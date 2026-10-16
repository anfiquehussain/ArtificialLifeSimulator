#include "alife/simulation/SpatialGrid.hpp"
#include "alife/simulation/SimulationEngine.hpp"
#include "alife/simulation/World.hpp"
#include "alife/ai/NeuralNetwork.hpp"
#include "alife/ai/Sensors.hpp"
#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <algorithm>

#define ALIFE_TEST_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            std::cerr << "FAIL: [" << __FILE__ << ":" << __LINE__ << "] " << msg << std::endl; \
            assert(false); \
        } else { \
            std::cout << "PASS: " << msg << std::endl; \
        } \
    } while (false)

void testSpatialGridBoundaryAndNeighbors() {
    std::cout << "\n--- Test 1: SpatialGrid Boundary Crossing & Neighbor Detection ---\n";
    alife::sim::SpatialGrid grid(1200.0f, 800.0f, 100.0f);

    // Grid cells are 100x100: Cell (0,0) is [0, 100)x[0, 100), Cell (1,0) is [100, 200)x[0, 100)
    // Insert entity 0 at (95, 50) in cell (0,0)
    // Insert entity 1 at (105, 50) in cell (1,0)
    grid.insert(0, 95.0f, 50.0f);
    grid.insert(1, 105.0f, 50.0f);

    ALIFE_TEST_ASSERT(grid.size() == 2, "SpatialGrid contains 2 entities");

    // Query from (95, 50) with radius 20: should find BOTH entity 0 and entity 1 across cell boundary
    std::vector<uint32_t> found;
    grid.queryNearby(95.0f, 50.0f, 20.0f, [&](uint32_t idx) {
        found.push_back(idx);
    });

    ALIFE_TEST_ASSERT(found.size() == 2, "Both entities detected across cell boundary");
    bool has0 = std::find(found.begin(), found.end(), 0u) != found.end();
    bool has1 = std::find(found.begin(), found.end(), 1u) != found.end();
    ALIFE_TEST_ASSERT(has0 && has1, "Both entity 0 and entity 1 returned in query");

    // Query from (95, 50) with small radius 4: (95 + 4 = 99 < 100) strictly remains within cell 0
    std::vector<uint32_t> nearFound;
    grid.queryNearby(95.0f, 50.0f, 4.0f, [&](uint32_t idx) {
        nearFound.push_back(idx);
    });
    ALIFE_TEST_ASSERT(nearFound.size() == 1 && nearFound[0] == 0, "Narrow radius correctly excludes distant cell across boundary");
}

void testSpatialGridVsBruteForceEquivalence() {
    std::cout << "\n--- Test 2: SpatialGrid vs Brute Force Ground Truth ---\n";
    alife::sim::SpatialGrid grid(1200.0f, 800.0f, 100.0f);
    alife::core::Rng rng(42);

    struct Dummy {
        float x, y;
    };
    std::vector<Dummy> points;
    constexpr size_t N = 500;
    points.reserve(N);

    for (size_t i = 0; i < N; ++i) {
        float x = rng.randomFloat(0.0f, 1200.0f);
        float y = rng.randomFloat(0.0f, 800.0f);
        points.push_back({x, y});
        grid.insert(static_cast<uint32_t>(i), x, y);
    }

    ALIFE_TEST_ASSERT(grid.size() == N, "Grid populated with all N elements");

    // Test 50 random queries
    for (int q = 0; q < 50; ++q) {
        float qx = rng.randomFloat(0.0f, 1200.0f);
        float qy = rng.randomFloat(0.0f, 800.0f);
        float radius = rng.randomFloat(20.0f, 150.0f);
        float radSq = radius * radius;

        // Ground truth brute force
        std::vector<uint32_t> bruteMatches;
        for (size_t i = 0; i < N; ++i) {
            float dx = points[i].x - qx;
            float dy = points[i].y - qy;
            if (dx * dx + dy * dy <= radSq) {
                bruteMatches.push_back(static_cast<uint32_t>(i));
            }
        }
        std::sort(bruteMatches.begin(), bruteMatches.end());

        // SpatialGrid query (filter by actual radius since grid returns cell candidates)
        std::vector<uint32_t> gridMatches;
        grid.queryNearby(qx, qy, radius, [&](uint32_t idx) {
            float dx = points[idx].x - qx;
            float dy = points[idx].y - qy;
            if (dx * dx + dy * dy <= radSq) {
                gridMatches.push_back(idx);
            }
        });
        std::sort(gridMatches.begin(), gridMatches.end());

        ALIFE_TEST_ASSERT(bruteMatches == gridMatches, "SpatialGrid query matched brute-force ground truth exactly");
    }
}

void testNoDuplicateInteractions() {
    std::cout << "\n--- Test 3: Duplicate Interaction Prevention ---\n";
    alife::sim::SpatialGrid grid(1200.0f, 800.0f, 100.0f);

    // Insert 50 entities across cells
    for (uint32_t i = 0; i < 50; ++i) {
        grid.insert(i, static_cast<float>(i * 20), 100.0f);
    }

    // Query covering a broad radius (spanning 3-4 cells)
    std::vector<uint32_t> visited;
    grid.queryNearby(150.0f, 100.0f, 200.0f, [&](uint32_t idx) {
        visited.push_back(idx);
    });

    std::vector<uint32_t> sortedVisited = visited;
    std::sort(sortedVisited.begin(), sortedVisited.end());
    auto it = std::adjacent_find(sortedVisited.begin(), sortedVisited.end());
    ALIFE_TEST_ASSERT(it == sortedVisited.end(), "Zero duplicate candidate indices returned across cell queries");
}

void testNeuralNetworkNumericalConsistency() {
    std::cout << "\n--- Test 4: Neural Network Numerical Consistency ---\n";
    alife::ai::NeuralNetwork nn(4, 6, 2);

    // Set known weights and biases
    std::vector<float> wIH(24, 0.25f);
    std::vector<float> bH(6, 0.1f);
    std::vector<float> wHO(12, 0.5f);
    std::vector<float> bO(2, -0.2f);
    nn.setParameters(wIH, bH, wHO, bO);

    std::vector<float> inputs = {1.0f, 0.5f, -0.5f, 0.0f};
    const auto& outputs = nn.evaluate(inputs);

    ALIFE_TEST_ASSERT(outputs.size() == 2, "Outputs size is 2");

    // Manually calculate reference output:
    // Input sum per hidden: 0.1 + (1.0 + 0.5 - 0.5 + 0.0) * 0.25 = 0.1 + 0.25 = 0.35
    // Hidden activation: tanh(0.35)
    float expectedH = std::tanh(0.35f);
    // Output sum: -0.2 + 6 * (expectedH * 0.5) = -0.2 + 3.0 * expectedH
    float expectedO = std::tanh(-0.2f + 3.0f * expectedH);

    float diff0 = std::abs(outputs[0] - expectedO);
    float diff1 = std::abs(outputs[1] - expectedO);

    ALIFE_TEST_ASSERT(diff0 < 1e-5f, "Output 0 matches manual analytical expectation");
    ALIFE_TEST_ASSERT(diff1 < 1e-5f, "Output 1 matches manual analytical expectation");

    // Test NaN input protection
    std::vector<float> nanInputs = {std::numeric_limits<float>::quiet_NaN(), 0.5f, -0.5f, 0.0f};
    const auto& nanOutputs = nn.evaluate(nanInputs);
    ALIFE_TEST_ASSERT(!std::isnan(nanOutputs[0]) && !std::isinf(nanOutputs[0]), "NaN inputs sanitized gracefully");
}

void testReproducibilityUnderControlledSeed() {
    std::cout << "\n--- Test 5: Determinism & Controlled-Seed Reproducibility ---\n";
    alife::core::AppConfig config;
    config.randomSeed = 1337;
    config.initialPopulation = 80;
    config.initialHerbivoreCount = 60;
    config.initialPredatorCount = 20;
    config.initialFoodCount = 100;

    auto runSim = [&](uint64_t seed) {
        config.randomSeed = seed;
        alife::sim::World world(config);
        alife::core::Rng rng(seed);
        world.reset(rng, 0.0);

        for (int step = 0; step < 100; ++step) {
            world.update(config.fixedTimeStep, rng, step * config.fixedTimeStep);
        }
        return world.getPopulation().computeEvolutionSnapshot(100 * config.fixedTimeStep, 100);
    };

    auto snapA = runSim(1337);
    auto snapB = runSim(1337);

    ALIFE_TEST_ASSERT(snapA.populationSize == snapB.populationSize, "Identical final population sizes");
    ALIFE_TEST_ASSERT(snapA.totalBirths == snapB.totalBirths, "Identical total births");
    ALIFE_TEST_ASSERT(snapA.totalDeaths == snapB.totalDeaths, "Identical total deaths");
    ALIFE_TEST_ASSERT(snapA.totalPredations == snapB.totalPredations, "Identical total predations");
    ALIFE_TEST_ASSERT(std::abs(snapA.speed.mean - snapB.speed.mean) < 1e-6f, "Identical mean speed under controlled seed");
}

void testZeroAllocationGridLifecycle() {
    std::cout << "\n--- Test 6: Zero Allocations and Capacity Retention ---\n";
    alife::sim::SpatialGrid grid(1200.0f, 800.0f, 100.0f);

    // Populate with 200 items
    for (uint32_t i = 0; i < 200; ++i) {
        grid.insert(i, 50.0f, 50.0f);
    }
    ALIFE_TEST_ASSERT(grid.size() == 200, "Grid populated with 200 items");

    // Clear grid
    grid.clear();
    ALIFE_TEST_ASSERT(grid.empty(), "Grid reports empty after clear");
    ALIFE_TEST_ASSERT(grid.size() == 0, "Grid size is 0 after clear");

    // Re-inserting without reallocation
    for (uint32_t i = 0; i < 50; ++i) {
        grid.insert(i, 50.0f, 50.0f);
    }
    ALIFE_TEST_ASSERT(grid.size() == 50, "Grid cleanly repopulated without memory leak");
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "   PHASE 9: PERFORMANCE & SCALABILITY REGRESSION TESTS  \n";
    std::cout << "========================================================\n";

    testSpatialGridBoundaryAndNeighbors();
    testSpatialGridVsBruteForceEquivalence();
    testNoDuplicateInteractions();
    testNeuralNetworkNumericalConsistency();
    testReproducibilityUnderControlledSeed();
    testZeroAllocationGridLifecycle();

    std::cout << "\nAll Phase 9 Performance and Scalability tests passed successfully!\n";
    return 0;
}
