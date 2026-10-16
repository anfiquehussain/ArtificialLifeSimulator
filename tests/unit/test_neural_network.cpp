#include "alife/ai/NeuralNetwork.hpp"
#include "alife/ai/Sensors.hpp"
#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include "alife/entities/Organism.hpp"
#include "alife/entities/Food.hpp"
#include "alife/simulation/PopulationManager.hpp"
#include "alife/simulation/World.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
#include <cstdlib>

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "Assertion failed: " #cond << " at line " << __LINE__ << std::endl; \
        std::abort(); \
    } \
} while(0)

using namespace alife;

// 1. Network construction and valid dimensions
void testNetworkConstruction() {
    ai::NeuralNetwork net(6, 8, 2);
    TEST_ASSERT(net.getNumInputs() == 6);
    TEST_ASSERT(net.getNumHidden() == 8);
    TEST_ASSERT(net.getNumOutputs() == 2);
    TEST_ASSERT(net.getWeightsInputHidden().size() == 6 * 8);
    TEST_ASSERT(net.getBiasesHidden().size() == 8);
    TEST_ASSERT(net.getWeightsHiddenOutput().size() == 8 * 2);
    TEST_ASSERT(net.getBiasesOutput().size() == 2);

    ai::NeuralNetwork defaultNet;
    TEST_ASSERT(defaultNet.getNumInputs() == 6);
    TEST_ASSERT(defaultNet.getNumHidden() == 8);
    TEST_ASSERT(defaultNet.getNumOutputs() == 2);
    std::cout << "Test 1 Passed: Network construction and valid dimensions.\n";
}

// 2. Forward propagation using known weights and inputs
void testForwardPropagationKnown() {
    // 2 inputs -> 2 hidden -> 1 output
    ai::NeuralNetwork net(2, 2, 1);
    // Weights:
    // in0 -> h0 = 1.0, in1 -> h0 = 0.0
    // in0 -> h1 = 0.0, in1 -> h1 = 1.0
    // h0 -> out0 = 1.0, h1 -> out0 = -1.0
    // Biases: 0.0
    std::vector<float> wIH = {1.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> bH = {0.0f, 0.0f};
    std::vector<float> wHO = {1.0f, -1.0f};
    std::vector<float> bO = {0.0f};
    net.setParameters(wIH, bH, wHO, bO);

    std::vector<float> inputs = {0.5f, 0.5f};
    const auto& out = net.evaluate(inputs);
    TEST_ASSERT(out.size() == 1);
    // h0 = tanh(0.5), h1 = tanh(0.5)
    // out = tanh(h0 * 1.0 + h1 * -1.0) = tanh(0) = 0.0
    TEST_ASSERT(std::abs(out[0]) < 1e-5f);

    std::vector<float> inputs2 = {1.0f, 0.0f};
    const auto& out2 = net.evaluate(inputs2);
    // h0 = tanh(1.0) ~ 0.76159, h1 = 0
    // out = tanh(tanh(1.0)) ~ 0.642
    float expected = std::tanh(std::tanh(1.0f));
    TEST_ASSERT(std::abs(out2[0] - expected) < 1e-4f);
    std::cout << "Test 2 Passed: Forward propagation using known weights and inputs.\n";
}

// 3. Activation-function behaviour (tanh bounded in [-1, 1], zero input -> zero output for zero biases)
void testActivationFunction() {
    ai::NeuralNetwork net(2, 2, 2);
    std::vector<float> wIH(4, 2.0f);
    std::vector<float> bH(2, 0.0f);
    std::vector<float> wHO(4, 2.0f);
    std::vector<float> bO(2, 0.0f);
    net.setParameters(wIH, bH, wHO, bO);

    std::vector<float> zeroInputs = {0.0f, 0.0f};
    const auto& out = net.evaluate(zeroInputs);
    TEST_ASSERT(std::abs(out[0]) < 1e-6f);
    TEST_ASSERT(std::abs(out[1]) < 1e-6f);

    std::vector<float> largeInputs = {100.0f, 100.0f};
    const auto& outLarge = net.evaluate(largeInputs);
    TEST_ASSERT(outLarge[0] > 0.99f && outLarge[0] <= 1.0f);
    TEST_ASSERT(outLarge[1] > 0.99f && outLarge[1] <= 1.0f);
    std::cout << "Test 3 Passed: Activation-function behaviour.\n";
}

// 4. Output range and numerical validity
void testOutputRangeAndNumericalValidity() {
    core::Rng rng(42);
    ai::NeuralNetwork net(6, 8, 2);
    net.randomize(rng, 3.0f);

    for (int i = 0; i < 100; ++i) {
        std::vector<float> in = {
            rng.randomFloat(-10.0f, 10.0f),
            rng.randomFloat(-10.0f, 10.0f),
            rng.randomFloat(-10.0f, 10.0f),
            rng.randomFloat(-10.0f, 10.0f),
            rng.randomFloat(-10.0f, 10.0f),
            rng.randomFloat(-10.0f, 10.0f)
        };
        const auto& out = net.evaluate(in);
        for (float v : out) {
            TEST_ASSERT(!std::isnan(v));
            TEST_ASSERT(!std::isinf(v));
            TEST_ASSERT(v >= -1.0f && v <= 1.0f);
        }
    }
    std::cout << "Test 4 Passed: Output range and numerical validity.\n";
}

// 5. Sensor normalization
void testSensorNormalization() {
    ai::SensorConfig config{100.0f, 1000.0f, 500.0f};
    std::vector<entities::Food> foods;
    foods.emplace_back(1, core::Vector2f{550.0f, 250.0f}, 10.0f); // 50 px to the right

    core::Vector2f orgPos{500.0f, 250.0f};
    core::Vector2f orgDir{1.0f, 0.0f}; // Heading right directly towards food
    float energy = 75.0f;
    float maxEnergy = 100.0f;

    auto sensors = ai::computeSensors(orgPos, orgDir, energy, maxEnergy, foods, config);
    TEST_ASSERT(sensors.size() == 6);
    // [0] energy ratio: 75/100 = 0.75
    TEST_ASSERT(std::abs(sensors[0] - 0.75f) < 1e-4f);
    // [1] food angle: directly ahead -> 0.0
    TEST_ASSERT(std::abs(sensors[1]) < 1e-4f);
    // [2] food distance: 50 / 100 = 0.5
    TEST_ASSERT(std::abs(sensors[2] - 0.5f) < 1e-4f);
    // [3] food detected: +1.0
    TEST_ASSERT(sensors[3] == 1.0f);
    // [4] boundary X: center 500 / 1000 -> 0.0
    TEST_ASSERT(std::abs(sensors[4]) < 1e-4f);
    // [5] boundary Y: center 250 / 500 -> 0.0
    TEST_ASSERT(std::abs(sensors[5]) < 1e-4f);
    std::cout << "Test 5 Passed: Sensor normalization.\n";
}

// 6. Behaviour when no food is detected
void testNoFoodDetectedSensors() {
    ai::SensorConfig config{100.0f, 1000.0f, 500.0f};
    std::vector<entities::Food> foods; // Empty foods list
    core::Vector2f orgPos{100.0f, 100.0f};
    core::Vector2f orgDir{0.0f, 1.0f};

    auto sensors = ai::computeSensors(orgPos, orgDir, 50.0f, 100.0f, foods, config);
    TEST_ASSERT(sensors[1] == 0.0f);  // Angle default
    TEST_ASSERT(sensors[2] == 1.0f);  // Max distance
    TEST_ASSERT(sensors[3] == -1.0f); // Food absent flag
    std::cout << "Test 6 Passed: Behaviour when no food is detected.\n";
}

// 7. Action mapping from network outputs
void testActionMapping() {
    biology::Traits traits{100.0f, 1.0f, 10.0f};
    entities::Organism org(1, {500.0f, 500.0f}, {1.0f, 0.0f}, traits, 100.0f, 100.0f);

    // Provide known network with turn rate +1.0 (turn left/counter-clockwise) and speed throttle +1.0 (sprint)
    ai::NeuralNetwork customBrain(6, 2, 2);
    // Set parameters to produce output [1.0, 1.0]
    std::vector<float> wIH(12, 0.0f);
    std::vector<float> bH = {5.0f, 5.0f};
    std::vector<float> wHO = {5.0f, 5.0f, 5.0f, 5.0f};
    std::vector<float> bO = {5.0f, 5.0f};
    customBrain.setParameters(wIH, bH, wHO, bO);
    org.setBrain(customBrain);

    std::vector<float> dummySensors(6, 0.0f);
    float dt = 0.5f; // half second
    org.update(dt, 0.02f, 1000.0f, 1000.0f, 6.0f, dummySensors);

    // Outputs should be ~1.0
    const auto& outputs = org.getLastOutputs();
    TEST_ASSERT(outputs[0] > 0.99f);
    TEST_ASSERT(outputs[1] > 0.99f);

    // Heading should have turned by ~ pi * dt = pi * 0.5 rad (90 degrees)
    // Starting from {1, 0}, angle 0 + pi/2 = pi/2 -> {0, 1}
    TEST_ASSERT(std::abs(org.getDirection().x) < 0.1f);
    TEST_ASSERT(org.getDirection().y > 0.9f);
    std::cout << "Test 7 Passed: Action mapping from network outputs.\n";
}

// 8. Independent controllers for different organisms
void testIndependentControllers() {
    core::Rng rng(1234);
    core::AppConfig config;
    sim::PopulationManager pop(config);
    pop.initialize(rng);

    auto& orgs = pop.getOrganisms();
    TEST_ASSERT(orgs.size() >= 2);
    auto& brain0 = orgs[0].getBrain();
    auto& brain1 = orgs[1].getBrain();

    // Check that different organisms have distinct parameter vectors
    bool differentWeights = false;
    const auto& w0 = brain0.getWeightsInputHidden();
    const auto& w1 = brain1.getWeightsInputHidden();
    for (size_t i = 0; i < w0.size(); ++i) {
        if (std::abs(w0[i] - w1[i]) > 1e-4f) {
            differentWeights = true;
            break;
        }
    }
    TEST_ASSERT(differentWeights);
    std::cout << "Test 8 Passed: Independent controllers for different organisms.\n";
}

// 9. Network parameter inheritance
void testNetworkParameterInheritance() {
    core::Rng rng(999);
    ai::NeuralNetwork parentBrain(6, 8, 2);
    parentBrain.randomize(rng, 1.5f);

    // Mutate with probability 0.0 -> exact clone
    ai::NeuralNetwork childBrain = parentBrain.mutate(rng, 0.0f, 0.2f);
    TEST_ASSERT(childBrain.getWeightsInputHidden() == parentBrain.getWeightsInputHidden());
    TEST_ASSERT(childBrain.getBiasesHidden() == parentBrain.getBiasesHidden());
    TEST_ASSERT(childBrain.getWeightsHiddenOutput() == parentBrain.getWeightsHiddenOutput());
    TEST_ASSERT(childBrain.getBiasesOutput() == parentBrain.getBiasesOutput());
    std::cout << "Test 9 Passed: Network parameter inheritance without mutation.\n";
}

// 10. Mutation probability and parameter bounds
void testMutationAndBounds() {
    core::Rng rng(777);
    ai::NeuralNetwork parentBrain(6, 8, 2);
    parentBrain.randomize(rng, 1.0f);

    float maxWeight = 2.5f;
    // Mutate with prob 1.0 and large magnitude
    ai::NeuralNetwork childBrain = parentBrain.mutate(rng, 1.0f, 5.0f, maxWeight);

    for (float w : childBrain.getWeightsInputHidden()) {
        TEST_ASSERT(std::abs(w) <= maxWeight);
    }
    for (float b : childBrain.getBiasesHidden()) {
        TEST_ASSERT(std::abs(b) <= maxWeight);
    }
    for (float w : childBrain.getWeightsHiddenOutput()) {
        TEST_ASSERT(std::abs(w) <= maxWeight);
    }
    for (float b : childBrain.getBiasesOutput()) {
        TEST_ASSERT(std::abs(b) <= maxWeight);
    }
    std::cout << "Test 10 Passed: Mutation probability and parameter bounds.\n";
}

// 11. Deterministic inference for identical inputs and parameters
void testDeterministicInference() {
    core::Rng rng(555);
    ai::NeuralNetwork net(6, 8, 2);
    net.randomize(rng, 1.0f);

    std::vector<float> in = {0.1f, -0.2f, 0.5f, 1.0f, -0.7f, 0.3f};
    auto out1 = net.evaluate(in);
    auto out2 = net.evaluate(in);
    TEST_ASSERT(out1 == out2);
    std::cout << "Test 11 Passed: Deterministic inference for identical inputs and parameters.\n";
}

// 12. Reproducibility under controlled random seeds
void testSeedReproducibility() {
    core::Rng rng1(4242);
    ai::NeuralNetwork net1(6, 8, 2);
    net1.randomize(rng1, 1.0f);

    core::Rng rng2(4242);
    ai::NeuralNetwork net2(6, 8, 2);
    net2.randomize(rng2, 1.0f);

    TEST_ASSERT(net1.getWeightsInputHidden() == net2.getWeightsInputHidden());
    TEST_ASSERT(net1.getBiasesHidden() == net2.getBiasesHidden());
    TEST_ASSERT(net1.getWeightsHiddenOutput() == net2.getWeightsHiddenOutput());
    TEST_ASSERT(net1.getBiasesOutput() == net2.getBiasesOutput());
    std::cout << "Test 12 Passed: Reproducibility under controlled random seeds.\n";
}

// 13. Integration with movement and energy systems
void testMovementAndEnergyIntegration() {
    biology::Traits traits{100.0f, 2.0f, 10.0f}; // speed 100, basal 2
    entities::Organism fastOrg(1, {500.0f, 500.0f}, {1.0f, 0.0f}, traits, 100.0f, 100.0f);
    entities::Organism slowOrg(2, {500.0f, 500.0f}, {1.0f, 0.0f}, traits, 100.0f, 100.0f);

    // Fast org: throttle +1.0 -> speedMod 1.5 -> speed 150
    ai::NeuralNetwork fastBrain(6, 2, 2);
    fastBrain.setParameters(std::vector<float>(12, 0.0f), {0.0f, 0.0f},
                            std::vector<float>(4, 0.0f), {0.0f, 5.0f}); // output[1] ~ +1.0
    fastOrg.setBrain(fastBrain);

    // Slow org: throttle -1.0 -> speedMod 0.5 -> speed 50
    ai::NeuralNetwork slowBrain(6, 2, 2);
    slowBrain.setParameters(std::vector<float>(12, 0.0f), {0.0f, 0.0f},
                            std::vector<float>(4, 0.0f), {0.0f, -5.0f}); // output[1] ~ -1.0
    slowOrg.setBrain(slowBrain);

    std::vector<float> sensors(6, 0.0f);
    float dt = 1.0f;
    float movementCost = 0.02f;
    fastOrg.update(dt, movementCost, 1000.0f, 1000.0f, 6.0f, sensors);
    slowOrg.update(dt, movementCost, 1000.0f, 1000.0f, 6.0f, sensors);

    // Fast org moved further than slow org
    TEST_ASSERT(fastOrg.getPosition().x > slowOrg.getPosition().x);
    // Fast org burned more energy than slow org (lower remaining energy)
    TEST_ASSERT(fastOrg.getEnergy() < slowOrg.getEnergy());
    std::cout << "Test 13 Passed: Integration with movement and energy systems.\n";
}

// 14. Safe handling of invalid numerical values
void testSafeHandlingInvalidValues() {
    ai::NeuralNetwork net(6, 8, 2);
    std::vector<float> nanInputs = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        0.0f, 0.0f, 0.0f
    };
    const auto& out = net.evaluate(nanInputs);
    for (float v : out) {
        TEST_ASSERT(!std::isnan(v));
        TEST_ASSERT(!std::isinf(v));
        TEST_ASSERT(v >= -1.0f && v <= 1.0f);
    }

    // Sensors safe with 0 max energy and NaN inputs
    ai::SensorConfig config{0.0f, 0.0f, 0.0f};
    auto s = ai::computeSensors({0, 0}, {0, 0}, 10.0f, 0.0f, {}, config);
    for (float val : s) {
        TEST_ASSERT(!std::isnan(val));
        TEST_ASSERT(!std::isinf(val));
    }
    std::cout << "Test 14 Passed: Safe handling of invalid numerical values.\n";
}

// 15. No regressions in Phases 1–5
void testNoRegressions() {
    core::AppConfig config;
    core::Rng rng(1337);
    sim::World world(config);
    world.reset(rng);

    TEST_ASSERT(world.getPopulation().getActiveCount() == config.initialPopulation);
    TEST_ASSERT(world.getFoodManager().getFoodCount() == config.initialFoodCount);

    // Advance 60 steps
    for (int i = 0; i < 60; ++i) {
        world.update(1.0 / 60.0, rng, i * (1.0 / 60.0));
    }
    TEST_ASSERT(world.getUpdateCount() == 60);
    TEST_ASSERT(world.getPopulation().getActiveCount() > 0);
    std::cout << "Test 15 Passed: No regressions in Phases 1–5.\n";
}

int main() {
    std::cout << "Running Phase 6 Neural Network Unit Tests...\n";
    testNetworkConstruction();
    testForwardPropagationKnown();
    testActivationFunction();
    testOutputRangeAndNumericalValidity();
    testSensorNormalization();
    testNoFoodDetectedSensors();
    testActionMapping();
    testIndependentControllers();
    testNetworkParameterInheritance();
    testMutationAndBounds();
    testDeterministicInference();
    testSeedReproducibility();
    testMovementAndEnergyIntegration();
    testSafeHandlingInvalidValues();
    testNoRegressions();
    std::cout << "ALL 15 PHASE 6 TESTS PASSED SUCCESSFULLY!\n";
    return 0;
}
