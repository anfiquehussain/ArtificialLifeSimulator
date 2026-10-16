#pragma once

#include "alife/core/Config.hpp"
#include <string>
#include <fstream>
#include <iomanip>
#include <cstdint>

namespace alife::data {

/**
 * @brief Structured record encapsulating experiment parameters, runtime environment,
 * random seed, and high-level outcomes required for exact reproducibility.
 */
struct ExperimentMetadata {
    std::string experimentId{"Experiment_Phase8"};
    std::string runId{"Run_001"};
    std::string appVersion{"0.1.0-phase8"};
    uint64_t randomSeed{1337};
    double simulationDuration{0.0};
    uint64_t totalSteps{0};
    std::string terminationReason{"Running"};

    // Effective configuration parameters
    uint32_t initialPopulation{60};
    uint32_t initialHerbivores{45};
    uint32_t initialPredators{15};
    uint32_t maxPopulation{150};
    uint32_t initialFoodCount{80};
    uint32_t maxFoodCapacity{150};
    float foodEnergyValue{30.0f};
    float mutationProbability{0.20f};
    float mutationMagnitude{0.15f};
    float statsSamplingInterval{1.0f};
    bool enableMultiSpecies{true};
    bool enableSeasons{true};

    // Summary of experiment outcomes
    size_t finalPopulation{0};
    size_t finalHerbivores{0};
    size_t finalPredators{0};
    uint64_t totalBirths{0};
    uint64_t totalDeaths{0};
    uint64_t totalPredations{0};
    uint64_t totalFoodConsumed{0};
    uint64_t starvationDeaths{0};
    uint64_t predationDeaths{0};
    bool herbivoreExtinct{false};
    bool predatorExtinct{false};
    double herbivoreExtinctionTime{-1.0};
    double predatorExtinctionTime{-1.0};
};

/**
 * @brief Exports experiment metadata to a clean key-value CSV file.
 */
inline bool exportMetadataToCsv(const std::string& filepath, const ExperimentMetadata& meta) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << "parameter,value\n";
    file << "experiment_id," << meta.experimentId << "\n";
    file << "run_id," << meta.runId << "\n";
    file << "app_version," << meta.appVersion << "\n";
    file << "random_seed," << meta.randomSeed << "\n";
    file << "simulation_duration," << std::fixed << std::setprecision(3) << meta.simulationDuration << "\n";
    file << "total_steps," << meta.totalSteps << "\n";
    file << "termination_reason," << meta.terminationReason << "\n";
    file << "initial_population," << meta.initialPopulation << "\n";
    file << "initial_herbivores," << meta.initialHerbivores << "\n";
    file << "initial_predators," << meta.initialPredators << "\n";
    file << "max_population," << meta.maxPopulation << "\n";
    file << "initial_food_count," << meta.initialFoodCount << "\n";
    file << "max_food_capacity," << meta.maxFoodCapacity << "\n";
    file << "food_energy_value," << meta.foodEnergyValue << "\n";
    file << "mutation_probability," << meta.mutationProbability << "\n";
    file << "mutation_magnitude," << meta.mutationMagnitude << "\n";
    file << "stats_sampling_interval," << meta.statsSamplingInterval << "\n";
    file << "enable_multi_species," << (meta.enableMultiSpecies ? "true" : "false") << "\n";
    file << "enable_seasons," << (meta.enableSeasons ? "true" : "false") << "\n";
    file << "final_population," << meta.finalPopulation << "\n";
    file << "final_herbivores," << meta.finalHerbivores << "\n";
    file << "final_predators," << meta.finalPredators << "\n";
    file << "total_births," << meta.totalBirths << "\n";
    file << "total_deaths," << meta.totalDeaths << "\n";
    file << "total_predations," << meta.totalPredations << "\n";
    file << "total_food_consumed," << meta.totalFoodConsumed << "\n";
    file << "starvation_deaths," << meta.starvationDeaths << "\n";
    file << "predation_deaths," << meta.predationDeaths << "\n";
    file << "herbivore_extinct," << (meta.herbivoreExtinct ? "true" : "false") << "\n";
    file << "predator_extinct," << (meta.predatorExtinct ? "true" : "false") << "\n";
    file << "herbivore_extinction_time," << meta.herbivoreExtinctionTime << "\n";
    file << "predator_extinction_time," << meta.predatorExtinctionTime << "\n";

    return true;
}

} // namespace alife::data
