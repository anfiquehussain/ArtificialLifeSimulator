#pragma once

#include "alife/biology/EvolutionMetrics.hpp"
#include "alife/core/Config.hpp"
#include "alife/data/EventLogger.hpp"
#include "alife/data/ExperimentRecord.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstdint>

namespace alife::data {

/**
 * @brief Records periodic evolutionary and ecological snapshots, manages bounded
 * in-memory history, logs discrete ecosystem events, and exports structured scientific CSVs.
 */
class HistoryRecorder {
public:
    explicit HistoryRecorder(size_t maxSnapshots = 5000, size_t maxEvents = 10000)
        : maxSnapshots_(maxSnapshots),
          eventLogger_(maxEvents) {
        snapshots_.reserve(std::min(maxSnapshots, size_t(2000)));
    }

    void recordSnapshot(biology::EvolutionSnapshot snapshot) {
        // Compute delta growth rate and interval metrics relative to previous snapshot
        if (!snapshots_.empty()) {
            const auto& prev = snapshots_.back();
            snapshot.populationGrowthRate = static_cast<int64_t>(snapshot.populationSize) - static_cast<int64_t>(prev.populationSize);
            snapshot.intervalBirths = (snapshot.totalBirths >= prev.totalBirths) ? (snapshot.totalBirths - prev.totalBirths) : 0;
            snapshot.intervalDeaths = (snapshot.totalDeaths >= prev.totalDeaths) ? (snapshot.totalDeaths - prev.totalDeaths) : 0;
            snapshot.intervalFoodConsumed = (snapshot.totalFoodConsumed >= prev.totalFoodConsumed) ? (snapshot.totalFoodConsumed - prev.totalFoodConsumed) : 0;
            snapshot.intervalFoodEnergyGained = (snapshot.totalFoodEnergyGained >= prev.totalFoodEnergyGained) ? (snapshot.totalFoodEnergyGained - prev.totalFoodEnergyGained) : 0.0f;
            snapshot.intervalPredations = (snapshot.totalPredations >= prev.totalPredations) ? (snapshot.totalPredations - prev.totalPredations) : 0;
            snapshot.intervalPredationEnergyGained = (snapshot.totalPredationEnergyGained >= prev.totalPredationEnergyGained) ? (snapshot.totalPredationEnergyGained - prev.totalPredationEnergyGained) : 0.0f;
            snapshot.intervalStarvationDeaths = (snapshot.deathsStarvation >= prev.deathsStarvation) ? (snapshot.deathsStarvation - prev.deathsStarvation) : 0;
            snapshot.intervalPredationDeaths = (snapshot.deathsPredation >= prev.deathsPredation) ? (snapshot.deathsPredation - prev.deathsPredation) : 0;
        } else {
            snapshot.populationGrowthRate = 0;
            snapshot.intervalBirths = snapshot.totalBirths;
            snapshot.intervalDeaths = snapshot.totalDeaths;
            snapshot.intervalFoodConsumed = snapshot.totalFoodConsumed;
            snapshot.intervalFoodEnergyGained = snapshot.totalFoodEnergyGained;
            snapshot.intervalPredations = snapshot.totalPredations;
            snapshot.intervalPredationEnergyGained = snapshot.totalPredationEnergyGained;
            snapshot.intervalStarvationDeaths = snapshot.deathsStarvation;
            snapshot.intervalPredationDeaths = snapshot.deathsPredation;
        }

        if (snapshots_.size() >= maxSnapshots_) {
            snapshots_.erase(snapshots_.begin());
        }
        snapshots_.push_back(std::move(snapshot));
    }

    void reset() noexcept {
        snapshots_.clear();
        eventLogger_.reset();
    }

    [[nodiscard]] const std::vector<biology::EvolutionSnapshot>& getSnapshots() const noexcept {
        return snapshots_;
    }

    [[nodiscard]] size_t getSnapshotCount() const noexcept {
        return snapshots_.size();
    }

    [[nodiscard]] EventLogger& getEventLogger() noexcept {
        return eventLogger_;
    }

    [[nodiscard]] const EventLogger& getEventLogger() const noexcept {
        return eventLogger_;
    }

    // 1. Backward-compatible Phase 5 CSV Export (evolution_telemetry.csv)
    bool exportToCsv(const std::string& filepath, const core::AppConfig& config) const {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        file << "# Experiment: " << config.experimentId << "\n";
        file << "# Random Seed: " << config.randomSeed << "\n";
        file << "# Initial Population: " << config.initialPopulation << "\n";
        file << "# Max Population: " << config.maxPopulation << "\n";
        file << "# Mutation Probability: " << config.mutationProbability << "\n";
        file << "# Mutation Magnitude: " << config.mutationMagnitude << "\n";
        file << "# Food Capacity: " << config.maxFoodCapacity << "\n";

        file << "timestamp,step,population,births,deaths,max_gen,"
             << "speed_mean,speed_median,speed_min,speed_max,speed_stddev,"
             << "basal_mean,basal_median,basal_min,basal_max,basal_stddev,"
             << "radius_mean,radius_median,radius_min,radius_max,radius_stddev,"
             << "trait_diversity\n";

        for (const auto& s : snapshots_) {
            file << std::fixed << std::setprecision(2) << s.timestamp << ","
                 << s.stepCount << ","
                 << s.populationSize << ","
                 << s.totalBirths << ","
                 << s.totalDeaths << ","
                 << s.maxGeneration << ","
                 << std::setprecision(3)
                 << s.speed.mean << "," << s.speed.median << "," << s.speed.min << "," << s.speed.max << "," << s.speed.stdDev << ","
                 << s.basalMetabolism.mean << "," << s.basalMetabolism.median << "," << s.basalMetabolism.min << "," << s.basalMetabolism.max << "," << s.basalMetabolism.stdDev << ","
                 << s.foodInteractionRadius.mean << "," << s.foodInteractionRadius.median << "," << s.foodInteractionRadius.min << "," << s.foodInteractionRadius.max << "," << s.foodInteractionRadius.stdDev << ","
                 << s.traitDiversity << "\n";
        }

        return true;
    }

    // 2. Population Demographics CSV Export
    bool exportPopulationHistory(const std::string& filepath, const core::AppConfig& config) const {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;

        file << "# Experiment: " << config.experimentId << "\n";
        file << "# Seed: " << config.randomSeed << "\n";
        file << "timestamp,step,population,herbivore_pop,predator_pop,interval_births,interval_deaths,"
             << "cumulative_births,cumulative_deaths,growth_rate,deaths_starvation,deaths_predation,"
             << "interval_starvation,interval_predation,mean_generation,min_generation,max_generation,"
             << "herbivore_extinct,predator_extinct\n";

        for (const auto& s : snapshots_) {
            file << std::fixed << std::setprecision(2) << s.timestamp << ","
                 << s.stepCount << ","
                 << s.populationSize << ","
                 << s.herbivorePopulation << ","
                 << s.predatorPopulation << ","
                 << s.intervalBirths << ","
                 << s.intervalDeaths << ","
                 << s.totalBirths << ","
                 << s.totalDeaths << ","
                 << s.populationGrowthRate << ","
                 << s.deathsStarvation << ","
                 << s.deathsPredation << ","
                 << s.intervalStarvationDeaths << ","
                 << s.intervalPredationDeaths << ","
                 << std::setprecision(2)
                 << s.meanGeneration << ","
                 << s.minGeneration << ","
                 << s.maxGeneration << ","
                 << (s.herbivoreExtinct ? "true" : "false") << ","
                 << (s.predatorExtinct ? "true" : "false") << "\n";
        }
        return true;
    }

    // 3. Species-Level Statistics CSV Export
    bool exportSpeciesStatistics(const std::string& filepath, const core::AppConfig& config) const {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;

        file << "# Experiment: " << config.experimentId << "\n";
        file << "timestamp,step,species,population,interval_births,interval_deaths,cumulative_births,"
             << "cumulative_deaths,deaths_starvation,deaths_predation,avg_energy,avg_speed,"
             << "speed_mean,speed_stddev,basal_mean,basal_stddev,radius_mean,radius_stddev,"
             << "trait_diversity,is_extinct,extinction_time\n";

        for (const auto& s : snapshots_) {
            for (const auto& sm : s.speciesMetrics) {
                file << std::fixed << std::setprecision(2) << s.timestamp << ","
                     << s.stepCount << ","
                     << biology::speciesName(sm.speciesId) << ","
                     << sm.population << ","
                     << sm.intervalBirths << ","
                     << sm.intervalDeaths << ","
                     << sm.cumulativeBirths << ","
                     << sm.cumulativeDeaths << ","
                     << sm.deathsStarvation << ","
                     << sm.deathsPredation << ","
                     << std::setprecision(3)
                     << sm.avgEnergy << ","
                     << sm.avgSpeed << ","
                     << sm.speed.mean << ","
                     << sm.speed.stdDev << ","
                     << sm.basalMetabolism.mean << ","
                     << sm.basalMetabolism.stdDev << ","
                     << sm.foodInteractionRadius.mean << ","
                     << sm.foodInteractionRadius.stdDev << ","
                     << sm.traitDiversity << ","
                     << (sm.isExtinct ? "true" : "false") << ","
                     << std::setprecision(2) << sm.extinctionTime << "\n";
            }
        }
        return true;
    }

    // 4. Trait Statistics with Quantiles and Histogram Distribution Bins
    bool exportTraitStatistics(const std::string& filepath, const core::AppConfig& config) const {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;

        file << "# Experiment: " << config.experimentId << "\n";
        file << "timestamp,step,trait,mean,median,min,max,variance,stddev,q1,q3,bin0,bin1,bin2,bin3,bin4\n";

        auto writeTrait = [&](double t, uint64_t step, const char* name, const biology::TraitSummary& tr) {
            file << std::fixed << std::setprecision(2) << t << ","
                 << step << ","
                 << name << ","
                 << std::setprecision(3)
                 << tr.mean << ","
                 << tr.median << ","
                 << tr.min << ","
                 << tr.max << ","
                 << tr.variance << ","
                 << tr.stdDev << ","
                 << tr.q1 << ","
                 << tr.q3 << ","
                 << tr.histogramBins[0] << ","
                 << tr.histogramBins[1] << ","
                 << tr.histogramBins[2] << ","
                 << tr.histogramBins[3] << ","
                 << tr.histogramBins[4] << "\n";
        };

        for (const auto& s : snapshots_) {
            writeTrait(s.timestamp, s.stepCount, "speed", s.speed);
            writeTrait(s.timestamp, s.stepCount, "basal_metabolism", s.basalMetabolism);
            writeTrait(s.timestamp, s.stepCount, "food_interaction_radius", s.foodInteractionRadius);
        }
        return true;
    }

    // 5. Ecosystem & Trophic History CSV Export
    bool exportEcosystemHistory(const std::string& filepath, const core::AppConfig& config) const {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;

        file << "# Experiment: " << config.experimentId << "\n";
        file << "timestamp,step,food_count,food_capacity,interval_food_consumed,total_food_consumed,"
             << "interval_food_energy,total_food_energy,interval_predations,total_predations,"
             << "interval_predation_energy,total_predation_energy,season_factor,food_regen_multiplier,season_name\n";

        for (const auto& s : snapshots_) {
            file << std::fixed << std::setprecision(2) << s.timestamp << ","
                 << s.stepCount << ","
                 << s.foodCount << ","
                 << s.maxFoodCapacity << ","
                 << s.intervalFoodConsumed << ","
                 << s.totalFoodConsumed << ","
                 << std::setprecision(2)
                 << s.intervalFoodEnergyGained << ","
                 << s.totalFoodEnergyGained << ","
                 << s.intervalPredations << ","
                 << s.totalPredations << ","
                 << s.intervalPredationEnergyGained << ","
                 << s.totalPredationEnergyGained << ","
                 << std::setprecision(3)
                 << s.seasonFactor << ","
                 << s.foodRegenMultiplier << ",\""
                 << s.seasonName << "\"\n";
        }
        return true;
    }

    // 6. Export all scientific observation tables into directory
    bool exportAll(const std::string& directory,
                   const core::AppConfig& config,
                   const ExperimentMetadata& metadata) const {
        try {
            std::filesystem::create_directories(directory);
        } catch (...) {
            return false;
        }

        std::filesystem::path dir(directory);
        bool ok = true;
        ok &= exportToCsv((dir / "evolution_telemetry.csv").string(), config);
        ok &= exportPopulationHistory((dir / "population_history.csv").string(), config);
        ok &= exportSpeciesStatistics((dir / "species_statistics.csv").string(), config);
        ok &= exportTraitStatistics((dir / "trait_statistics.csv").string(), config);
        ok &= exportEcosystemHistory((dir / "ecosystem_history.csv").string(), config);
        ok &= eventLogger_.exportToCsv((dir / "events.csv").string());
        ok &= exportMetadataToCsv((dir / "experiment_metadata.csv").string(), metadata);

        return ok;
    }

private:
    size_t maxSnapshots_{5000};
    std::vector<biology::EvolutionSnapshot> snapshots_;
    EventLogger eventLogger_;
};

} // namespace alife::data
