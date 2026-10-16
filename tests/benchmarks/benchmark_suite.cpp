#include "alife/simulation/SimulationEngine.hpp"
#include "alife/core/Config.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

void runWorkloadBenchmark(const std::string& name,
                          uint32_t popSize,
                          uint32_t foodCount,
                          int steps = 300,
                          uint64_t seed = 42) {
    alife::core::AppConfig config;
    config.randomSeed = seed;
    config.initialPopulation = popSize;
    config.initialHerbivoreCount = (popSize * 3) / 4;
    config.initialPredatorCount = popSize - config.initialHerbivoreCount;
    config.maxPopulation = popSize * 2;
    config.initialFoodCount = foodCount;
    config.maxFoodCapacity = foodCount * 2;
    config.enableEventLogging = false; // Isolate simulation throughput
    config.enableMultiSpecies = true;

    alife::sim::SimulationEngine engine(config);

    // Warm-up 10 steps to populate caches
    for (int i = 0; i < 10; ++i) {
        engine.update(alife::core::SimulationClock::Duration(0.0166666));
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < steps; ++i) {
        engine.update(alife::core::SimulationClock::Duration(0.0166666));
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    double totalSec = elapsed.count();
    double sps = static_cast<double>(steps) / totalSec;
    double msPerStep = (totalSec / static_cast<double>(steps)) * 1000.0;

    std::cout << std::left << std::setw(19) << name
              << " | Pop: " << std::setw(5) << popSize
              << " | Food: " << std::setw(5) << foodCount
              << " | Steps: " << std::setw(5) << steps
              << " | Total: " << std::fixed << std::setprecision(3) << std::setw(7) << totalSec << "s"
              << " | " << std::setw(8) << std::setprecision(1) << sps << " steps/s"
              << " | " << std::setw(6) << std::setprecision(3) << msPerStep << " ms/step\n";
}

void runDetailedProfiling(uint32_t popSize, uint32_t foodCount, int steps = 300) {
    alife::core::AppConfig config;
    config.randomSeed = 42;
    config.initialPopulation = popSize;
    config.initialHerbivoreCount = (popSize * 3) / 4;
    config.initialPredatorCount = popSize - config.initialHerbivoreCount;
    config.maxPopulation = popSize * 2;
    config.initialFoodCount = foodCount;
    config.maxFoodCapacity = foodCount * 2;
    config.enableEventLogging = false;
    config.enableMultiSpecies = true;

    alife::sim::World world(config);
    alife::core::Rng rng(config.randomSeed);
    world.reset(rng, 0.0);

    double totalMovementTime = 0.0;
    double totalConsumptionTime = 0.0;
    double totalPredationTime = 0.0;
    double totalReproductionTime = 0.0;

    float dt = static_cast<float>(config.fixedTimeStep);

    for (int step = 0; step < steps; ++step) {
        double t = step * config.fixedTimeStep;
        world.getEnvironment().update(t);
        world.getFoodManager().updateRegeneration(dt, rng, world.getEnvironment().getFoodRegenMultiplier());

        // Measure Sensor & Movement (inside population_.update)
        auto t0 = std::chrono::high_resolution_clock::now();
        world.getPopulation().update(dt, world.getWidth(), world.getHeight(),
                                     world.getFoodManager().getFoodItems(),
                                     world.getEnvironment().getSeasonFactor(), t);
        auto t1 = std::chrono::high_resolution_clock::now();

        // Measure Food Consumption
        world.getFoodManager().resolveConsumption(world.getPopulation());
        auto t2 = std::chrono::high_resolution_clock::now();

        // Measure Predation
        world.getPopulation().resolvePredation();
        auto t3 = std::chrono::high_resolution_clock::now();

        // Measure Reproduction
        world.getPopulation().updateReproduction(rng, world.getWidth(), world.getHeight(), t);
        auto t4 = std::chrono::high_resolution_clock::now();

        totalMovementTime += std::chrono::duration<double, std::milli>(t1 - t0).count();
        totalConsumptionTime += std::chrono::duration<double, std::milli>(t2 - t1).count();
        totalPredationTime += std::chrono::duration<double, std::milli>(t3 - t2).count();
        totalReproductionTime += std::chrono::duration<double, std::milli>(t4 - t3).count();
    }

    double totalTime = totalMovementTime + totalConsumptionTime + totalPredationTime + totalReproductionTime;

    std::cout << "\n--- Subsystem Profile Breakdown (Pop: " << popSize << ", Food: " << foodCount << ", " << steps << " steps) ---\n";
    std::cout << "1. Population Update (Sensors + NN + Move): " << std::fixed << std::setprecision(2)
              << totalMovementTime << " ms (" << (totalMovementTime / totalTime * 100.0) << "%)\n";
    std::cout << "2. Food Consumption Arbitration:           "
              << totalConsumptionTime << " ms (" << (totalConsumptionTime / totalTime * 100.0) << "%)\n";
    std::cout << "3. Predation Interaction Resolution:       "
              << totalPredationTime << " ms (" << (totalPredationTime / totalTime * 100.0) << "%)\n";
    std::cout << "4. Reproduction & Lifecycle:               "
              << totalReproductionTime << " ms (" << (totalReproductionTime / totalTime * 100.0) << "%)\n";
    std::cout << "Total measured core step time:             " << totalTime << " ms (" << (totalTime / steps) << " ms/step)\n";
}

int main() {
    std::cout << "========================================================================================\n";
    std::cout << "              ALIFE SIMULATOR - SYSTEM BENCHMARK SUITE (PHASE 9)                        \n";
    std::cout << "========================================================================================\n";
    std::cout << "Platform: Windows 11 (x64) | Compiler: MSVC 19.51 | Build: Release | SIMD: Default\n";
    std::cout << "Warm-up: 10 steps | Measurement: 300 steps per workload\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    runWorkloadBenchmark("Workload 1 (Small)",   60,   80, 300);
    runWorkloadBenchmark("Workload 2 (Medium)", 150,  150, 300);
    runWorkloadBenchmark("Workload 3 (Large)",  300,  250, 300);
    runWorkloadBenchmark("Workload 4 (Scaled)", 600,  400, 300);
    runWorkloadBenchmark("Workload 5 (Stress)", 1000, 500, 300);

    std::cout << "========================================================================================\n";

    // Detailed profiling for Stress Workload
    runDetailedProfiling(1000, 500, 300);

    return 0;
}
