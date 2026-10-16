#include "alife/biology/Species.hpp"
#include "alife/ai/NeuralNetwork.hpp"
#include "alife/ai/Sensors.hpp"
#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include "alife/entities/Organism.hpp"
#include "alife/entities/Food.hpp"
#include "alife/environment/EnvironmentManager.hpp"
#include "alife/environment/FoodManager.hpp"
#include "alife/simulation/PopulationManager.hpp"
#include "alife/simulation/World.hpp"
#include "alife/simulation/SimulationEngine.hpp"
#include <chrono>
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

// 1. Species initialization and valid identifiers
void testSpeciesInitialization() {
    biology::Traits hTraits{50.0f, 1.5f, 10.0f};
    entities::Organism herbivore(1, {100.0f, 100.0f}, {1.0f, 0.0f}, hTraits, 80.0f, 100.0f, 0, 0, {}, biology::SpeciesId::Herbivore);
    TEST_ASSERT(herbivore.getSpeciesId() == biology::SpeciesId::Herbivore);
    TEST_ASSERT(herbivore.isHerbivore());
    TEST_ASSERT(!herbivore.isPredator());

    biology::Traits pTraits{70.0f, 2.0f, 12.0f};
    entities::Organism predator(2, {200.0f, 200.0f}, {0.0f, 1.0f}, pTraits, 80.0f, 100.0f, 0, 0, {}, biology::SpeciesId::Predator);
    TEST_ASSERT(predator.getSpeciesId() == biology::SpeciesId::Predator);
    TEST_ASSERT(predator.isPredator());
    TEST_ASSERT(!predator.isHerbivore());
    std::cout << "Test 1 Passed: Species initialization and valid identifiers.\n";
}

// 2. Species-specific configuration
void testSpeciesConfig() {
    core::AppConfig config;
    TEST_ASSERT(config.enableMultiSpecies);
    TEST_ASSERT(config.initialHerbivoreCount == 45);
    TEST_ASSERT(config.initialPredatorCount == 15);
    TEST_ASSERT(config.predatorCaptureRadius > 0.0f);
    TEST_ASSERT(config.predatorEnergyGain > 0.0f);
    TEST_ASSERT(config.predatorMinSpeed < config.predatorMaxSpeed);
    std::cout << "Test 2 Passed: Species-specific configuration.\n";
}

// 3. Predator-prey compatibility rules
void testPredatorPreyCompatibility() {
    core::AppConfig config;
    config.allowCannibalism = false;
    sim::PopulationManager pop(config);

    biology::Traits pTraits{70.0f, 2.0f, 12.0f};
    pop.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, pTraits, 50.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Predator);
    pop.spawnOrganism({105.0f, 100.0f}, {1.0f, 0.0f}, pTraits, 50.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Predator);

    // Two predators near each other: should NOT prey on each other
    pop.resolvePredation();
    TEST_ASSERT(pop.getOrganisms()[0].isAlive());
    TEST_ASSERT(pop.getOrganisms()[1].isAlive());
    TEST_ASSERT(pop.getTotalPredations() == 0);
    std::cout << "Test 3 Passed: Predator-prey compatibility rules.\n";
}

// 4. Detection and capture of eligible prey
void testPreyDetectionAndCapture() {
    core::AppConfig config;
    sim::PopulationManager pop(config);

    biology::Traits pTraits{70.0f, 2.0f, 12.0f};
    biology::Traits hTraits{50.0f, 1.5f, 10.0f};

    // Predator at (100, 100), Prey (Herbivore) at (105, 100) -> distance 5 < capture radius 12
    pop.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, pTraits, 40.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Predator);
    pop.spawnOrganism({105.0f, 100.0f}, {1.0f, 0.0f}, hTraits, 80.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Herbivore);

    pop.resolvePredation();
    TEST_ASSERT(pop.getTotalPredations() == 1);
    TEST_ASSERT(!pop.getOrganisms()[1].isAlive()); // Prey killed
    std::cout << "Test 4 Passed: Detection and capture of eligible prey.\n";
}

// 5. Prevention of duplicate prey consumption
void testPreventDuplicatePreyConsumption() {
    core::AppConfig config;
    sim::PopulationManager pop(config);

    biology::Traits pTraits{70.0f, 2.0f, 12.0f};
    biology::Traits hTraits{50.0f, 1.5f, 10.0f};

    // 2 Predators near 1 Herbivore
    pop.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, pTraits, 40.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Predator);
    pop.spawnOrganism({101.0f, 100.0f}, {1.0f, 0.0f}, pTraits, 40.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Predator);
    pop.spawnOrganism({105.0f, 100.0f}, {1.0f, 0.0f}, hTraits, 80.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Herbivore);

    pop.resolvePredation();
    // Exactly 1 predation event should occur, consuming the prey once
    TEST_ASSERT(pop.getTotalPredations() == 1);
    std::cout << "Test 5 Passed: Prevention of duplicate prey consumption.\n";
}

// 6. Correct predator energy gain
void testPredatorEnergyGain() {
    core::AppConfig config;
    config.predatorEnergyGain = 35.0f;
    sim::PopulationManager pop(config);

    biology::Traits pTraits{70.0f, 2.0f, 12.0f};
    biology::Traits hTraits{50.0f, 1.5f, 10.0f};

    pop.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, pTraits, 30.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Predator);
    pop.spawnOrganism({105.0f, 100.0f}, {1.0f, 0.0f}, hTraits, 80.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Herbivore);

    pop.resolvePredation();
    // Predator energy was 30 + 35 = 65
    TEST_ASSERT(std::abs(pop.getOrganisms()[0].getEnergy() - 65.0f) < 1e-4f);
    std::cout << "Test 6 Passed: Correct predator energy gain.\n";
}

// 7. Death and removal of consumed organisms
void testDeathAndRemovalOfConsumedOrganisms() {
    core::AppConfig config;
    sim::PopulationManager pop(config);

    biology::Traits pTraits{70.0f, 2.0f, 12.0f};
    biology::Traits hTraits{50.0f, 1.5f, 10.0f};

    pop.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, pTraits, 50.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Predator);
    pop.spawnOrganism({105.0f, 100.0f}, {1.0f, 0.0f}, hTraits, 50.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Herbivore);

    pop.resolvePredation();
    TEST_ASSERT(pop.getActiveCount() == 2); // 1 alive, 1 dead waiting purge
    pop.update(0.016f, 1000.0f, 1000.0f, 0.0);
    // Dead herbivore purged
    TEST_ASSERT(pop.getActiveCount() == 1);
    TEST_ASSERT(pop.getOrganisms()[0].isPredator());
    std::cout << "Test 7 Passed: Death and removal of consumed organisms.\n";
}

// 8. Resource competition and duplicate-consumption prevention
void testResourceCompetition() {
    core::AppConfig config;
    environment::FoodManager foodMgr(config);
    sim::PopulationManager pop(config);

    // Spawn 1 food pellet at (500, 500)
    foodMgr.spawnFood({500.0f, 500.0f}, 30.0f);

    // 2 Herbivores within reach: H1 at (502, 500) [dist 2], H2 at (505, 500) [dist 5]
    biology::Traits hTraits{50.0f, 1.5f, 10.0f};
    pop.spawnOrganism({502.0f, 500.0f}, {1.0f, 0.0f}, hTraits, 50.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Herbivore);
    pop.spawnOrganism({505.0f, 500.0f}, {1.0f, 0.0f}, hTraits, 50.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Herbivore);

    foodMgr.resolveConsumption(pop);
    // Food eaten exactly once
    TEST_ASSERT(foodMgr.getTotalFoodConsumed() == 1);
    // Closest herbivore (H1) got the energy (50 + 30 = 80)
    TEST_ASSERT(std::abs(pop.getOrganisms()[0].getEnergy() - 80.0f) < 1e-4f);
    // H2 got nothing
    TEST_ASSERT(std::abs(pop.getOrganisms()[1].getEnergy() - 50.0f) < 1e-4f);
    std::cout << "Test 8 Passed: Resource competition and duplicate-consumption prevention.\n";
}

// 9. Population statistics by species
void testPopulationStatisticsBySpecies() {
    core::AppConfig config;
    core::Rng rng(42);
    sim::PopulationManager pop(config);
    pop.initialize(rng);

    TEST_ASSERT(pop.getHerbivoreCount() == config.initialHerbivoreCount);
    TEST_ASSERT(pop.getPredatorCount() == config.initialPredatorCount);
    TEST_ASSERT(pop.getActiveCount() == config.initialHerbivoreCount + config.initialPredatorCount);
    std::cout << "Test 9 Passed: Population statistics by species.\n";
}

// 10. Extinction and zero-population handling
void testExtinctionAndZeroPopulation() {
    core::AppConfig config;
    sim::PopulationManager pop(config);

    biology::Traits pTraits{70.0f, 2.0f, 12.0f};
    pop.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, pTraits, 50.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Predator);

    TEST_ASSERT(pop.isExtinct(biology::SpeciesId::Herbivore)); // No herbivores ever spawned
    TEST_ASSERT(!pop.isExtinct(biology::SpeciesId::Predator));

    // Kill predator
    pop.getOrganisms()[0].kill();
    pop.update(0.016f, 1000.0f, 1000.0f, 0.0);
    TEST_ASSERT(pop.isExtinct(biology::SpeciesId::Predator));
    TEST_ASSERT(pop.getActiveCount() == 0);
    // Simulation can continue without crashing
    pop.update(0.016f, 1000.0f, 1000.0f, 1.0);
    std::cout << "Test 10 Passed: Extinction and zero-population handling.\n";
}

// 11. Reproduction after environmental interactions
void testReproductionAfterInteractions() {
    core::AppConfig config;
    core::Rng rng(101);
    sim::PopulationManager pop(config);

    biology::Traits hTraits{50.0f, 1.5f, 10.0f};
    // Herbivore eligible to reproduce (energy 90 >= 80)
    pop.spawnOrganism({500.0f, 500.0f}, {1.0f, 0.0f}, hTraits, 90.0f, 100.0f, 0, 0, 0.0, {}, biology::SpeciesId::Herbivore);

    pop.updateReproduction(rng, 1000.0f, 1000.0f, 0.0);
    TEST_ASSERT(pop.getActiveCount() == 2);
    // Offspring must also be a Herbivore
    TEST_ASSERT(pop.getOrganisms()[1].isHerbivore());
    TEST_ASSERT(pop.getOrganisms()[1].getParentId() == pop.getOrganisms()[0].getId());
    std::cout << "Test 11 Passed: Reproduction after environmental interactions.\n";
}

// 12. Environmental changes based on simulation time
void testEnvironmentalSeasonalChanges() {
    core::AppConfig config;
    config.enableSeasons = true;
    config.seasonCyclePeriod = 20.0f;
    config.seasonIntensity = 0.5f;

    environment::EnvironmentManager env(config);
    // t = 0 -> sin(0) = 0 -> seasonFactor = 0, regenMult = 1.0
    env.update(0.0);
    TEST_ASSERT(std::abs(env.getSeasonFactor()) < 1e-4f);
    TEST_ASSERT(std::abs(env.getFoodRegenMultiplier() - 1.0f) < 1e-4f);

    // t = 5 (quarter cycle) -> sin(pi/2) = 1 -> peak abundance
    env.update(5.0);
    TEST_ASSERT(std::abs(env.getSeasonFactor() - 1.0f) < 1e-3f);
    TEST_ASSERT(std::abs(env.getFoodRegenMultiplier() - 1.5f) < 1e-3f);

    // t = 15 (three quarter cycle) -> sin(3pi/2) = -1 -> peak drought
    env.update(15.0);
    TEST_ASSERT(std::abs(env.getSeasonFactor() - (-1.0f)) < 1e-3f);
    TEST_ASSERT(std::abs(env.getFoodRegenMultiplier() - 0.5f) < 1e-3f);
    std::cout << "Test 12 Passed: Environmental changes based on simulation time.\n";
}

// 13. Pause and reset behaviour
void testPauseAndResetBehaviour() {
    core::AppConfig config;
    sim::SimulationEngine engine(config);

    // Tick for 5 seconds
    for (int i = 0; i < 300; ++i) {
        engine.update(std::chrono::duration<double>(1.0 / 60.0));
    }
    float factorBeforePause = engine.getWorld().getEnvironment().getSeasonFactor();
    engine.pause();

    // Updating while paused should not advance environmental time or season
    for (int i = 0; i < 60; ++i) {
        engine.update(std::chrono::duration<double>(1.0 / 60.0));
    }
    float factorDuringPause = engine.getWorld().getEnvironment().getSeasonFactor();
    TEST_ASSERT(factorBeforePause == factorDuringPause);

    // Reset should return simulation to t=0
    engine.reset();
    TEST_ASSERT(engine.getClock().getSimulationTime() == 0.0);
    TEST_ASSERT(std::abs(engine.getWorld().getEnvironment().getSeasonFactor()) < 1e-4f);
    std::cout << "Test 13 Passed: Pause and reset behaviour.\n";
}

// 14. Reproducibility with controlled random seeds
void testSeedReproducibility() {
    core::AppConfig config;
    core::Rng rng1(1337);
    core::Rng rng2(1337);

    sim::World world1(config);
    sim::World world2(config);

    world1.reset(rng1);
    world2.reset(rng2);

    for (int i = 0; i < 100; ++i) {
        world1.update(1.0 / 60.0, rng1, i * (1.0 / 60.0));
        world2.update(1.0 / 60.0, rng2, i * (1.0 / 60.0));
    }

    TEST_ASSERT(world1.getPopulation().getHerbivoreCount() == world2.getPopulation().getHerbivoreCount());
    TEST_ASSERT(world1.getPopulation().getPredatorCount() == world2.getPopulation().getPredatorCount());
    TEST_ASSERT(world1.getPopulation().getTotalPredations() == world2.getPopulation().getTotalPredations());
    TEST_ASSERT(world1.getFoodManager().getFoodCount() == world2.getFoodManager().getFoodCount());
    std::cout << "Test 14 Passed: Reproducibility with controlled random seeds.\n";
}

// 15. Neural-network sensor integration
void testNeuralNetworkSensorIntegration() {
    ai::SensorConfig config{100.0f, 1000.0f, 1000.0f};
    std::vector<entities::Food> foods;
    foods.emplace_back(1, core::Vector2f{500.0f, 500.0f}, 20.0f);

    std::vector<entities::Organism> orgs;
    biology::Traits hTraits{50.0f, 1.5f, 10.0f};
    biology::Traits pTraits{70.0f, 2.0f, 12.0f};

    // Herbivore at (450, 500) heading right towards food
    orgs.emplace_back(1, core::Vector2f{450.0f, 500.0f}, core::Vector2f{1.0f, 0.0f}, hTraits, 80.0f, 100.0f, 0, 0, ai::NeuralNetwork{}, biology::SpeciesId::Herbivore);
    // Predator at (400, 500) heading right towards herbivore
    orgs.emplace_back(2, core::Vector2f{400.0f, 500.0f}, core::Vector2f{1.0f, 0.0f}, pTraits, 80.0f, 100.0f, 0, 0, ai::NeuralNetwork{}, biology::SpeciesId::Predator);

    std::vector<float> hBuffer(8, 0.0f);
    ai::computeEcologicalSensors(orgs[0], foods, orgs, 0.5f, config, hBuffer);
    // Herbivore target: food at 50 px distance -> normDist = 0.5, target angle = 0.0
    TEST_ASSERT(std::abs(hBuffer[1]) < 1e-4f);
    TEST_ASSERT(std::abs(hBuffer[2] - 0.5f) < 1e-4f);
    TEST_ASSERT(hBuffer[3] == 1.0f);
    // Herbivore threat: predator at (400, 500) -> 50 px directly behind -> angle = +/- 1.0
    TEST_ASSERT(std::abs(std::abs(hBuffer[4]) - 1.0f) < 1e-3f);
    TEST_ASSERT(std::abs(hBuffer[5] - 0.5f) < 1e-4f);
    TEST_ASSERT(hBuffer[6] == 1.0f);
    // Season factor
    TEST_ASSERT(std::abs(hBuffer[7] - 0.5f) < 1e-4f);

    std::vector<float> pBuffer(8, 0.0f);
    ai::computeEcologicalSensors(orgs[1], foods, orgs, 0.5f, config, pBuffer);
    // Predator target: herbivore at 50 px ahead -> angle = 0.0, dist = 0.5
    TEST_ASSERT(std::abs(pBuffer[1]) < 1e-4f);
    TEST_ASSERT(std::abs(pBuffer[2] - 0.5f) < 1e-4f);
    TEST_ASSERT(pBuffer[3] == 1.0f);
    std::cout << "Test 15 Passed: Neural-network sensor integration.\n";
}

// 16. Preservation of existing Phases 1–6 behaviour
void testPreservationPhases1to6() {
    core::AppConfig config;
    core::Rng rng(777);
    sim::World world(config);
    world.reset(rng);

    TEST_ASSERT(world.getUpdateCount() == 0);
    for (int i = 0; i < 60; ++i) {
        world.update(1.0 / 60.0, rng, i * (1.0 / 60.0));
    }
    TEST_ASSERT(world.getUpdateCount() == 60);
    TEST_ASSERT(world.getPopulation().getActiveCount() > 0);
    TEST_ASSERT(world.getFoodManager().getFoodCount() > 0);
    std::cout << "Test 16 Passed: Preservation of existing Phases 1–6 behaviour.\n";
}

int main() {
    std::cout << "Running Phase 7 Complex Ecosystem & Multi-Species Unit Tests...\n";
    testSpeciesInitialization();
    testSpeciesConfig();
    testPredatorPreyCompatibility();
    testPreyDetectionAndCapture();
    testPreventDuplicatePreyConsumption();
    testPredatorEnergyGain();
    testDeathAndRemovalOfConsumedOrganisms();
    testResourceCompetition();
    testPopulationStatisticsBySpecies();
    testExtinctionAndZeroPopulation();
    testReproductionAfterInteractions();
    testEnvironmentalSeasonalChanges();
    testPauseAndResetBehaviour();
    testSeedReproducibility();
    testNeuralNetworkSensorIntegration();
    testPreservationPhases1to6();
    std::cout << "ALL 16 PHASE 7 TESTS PASSED SUCCESSFULLY!\n";
    return 0;
}
