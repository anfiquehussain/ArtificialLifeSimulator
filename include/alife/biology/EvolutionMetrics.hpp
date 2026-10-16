#pragma once

#include "alife/biology/Species.hpp"
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdint>

namespace alife::biology {

/**
 * @brief Summary statistics for a single numeric trait across an active population.
 * Includes parametric and non-parametric estimators, quartiles, and frequency histogram bins.
 */
struct TraitSummary {
    float mean{0.0f};
    float median{0.0f};
    float min{0.0f};
    float max{0.0f};
    float variance{0.0f};
    float stdDev{0.0f};
    float q1{0.0f}; // 25th percentile
    float q3{0.0f}; // 75th percentile
    std::array<uint32_t, 5> histogramBins{0, 0, 0, 0, 0}; // 5 uniform frequency bins across [min, max]
};

/**
 * @brief Utility for computing descriptive statistics over a collection of float values.
 * Handles empty and single-element collections safely.
 */
inline TraitSummary computeTraitSummary(std::vector<float> values) {
    if (values.empty()) {
        return {};
    }

    std::sort(values.begin(), values.end());
    size_t n = values.size();

    float minVal = values.front();
    float maxVal = values.back();

    float medianVal = (n % 2 == 1)
        ? values[n / 2]
        : 0.5f * (values[n / 2 - 1] + values[n / 2]);

    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    float meanVal = static_cast<float>(sum / n);

    double varSum = 0.0;
    for (float v : values) {
        double diff = v - meanVal;
        varSum += diff * diff;
    }
    float varianceVal = static_cast<float>(varSum / n);
    float stdDevVal = std::sqrt(varianceVal);

    // Quartiles
    float q1Val = minVal;
    float q3Val = maxVal;
    if (n >= 4) {
        size_t idxQ1 = n / 4;
        size_t idxQ3 = (3 * n) / 4;
        q1Val = values[idxQ1];
        q3Val = values[idxQ3];
    } else {
        q1Val = minVal;
        q3Val = maxVal;
    }

    // 5-bin histogram across [minVal, maxVal]
    std::array<uint32_t, 5> bins{0, 0, 0, 0, 0};
    float range = maxVal - minVal;
    if (range > 1e-6f) {
        float binWidth = range / 5.0f;
        for (float v : values) {
            int b = static_cast<int>((v - minVal) / binWidth);
            if (b < 0) b = 0;
            if (b > 4) b = 4;
            ++bins[static_cast<size_t>(b)];
        }
    } else {
        bins[0] = static_cast<uint32_t>(n);
    }

    return {meanVal, medianVal, minVal, maxVal, varianceVal, stdDevVal, q1Val, q3Val, bins};
}

/**
 * @brief Detailed measurements for a specific species at a discrete sampling point.
 */
struct SpeciesMetrics {
    SpeciesId speciesId{SpeciesId::Herbivore};
    size_t population{0};
    uint64_t cumulativeBirths{0};
    uint64_t cumulativeDeaths{0};
    uint64_t intervalBirths{0};
    uint64_t intervalDeaths{0};
    uint64_t deathsStarvation{0};
    uint64_t deathsPredation{0};
    float avgEnergy{0.0f};
    float avgSpeed{0.0f};
    TraitSummary speed;
    TraitSummary basalMetabolism;
    TraitSummary foodInteractionRadius;
    float traitDiversity{0.0f};
    bool isExtinct{false};
    double extinctionTime{-1.0};
};

/**
 * @brief Comprehensive snapshot of evolutionary, population, ecosystem,
 * and demographic metrics at a discrete simulation-time sampling point.
 */
struct EvolutionSnapshot {
    double timestamp{0.0};
    uint64_t stepCount{0};
    size_t populationSize{0};
    uint64_t totalBirths{0};
    uint64_t totalDeaths{0};
    uint32_t maxGeneration{0};

    // Trait summaries (population-wide)
    TraitSummary speed;
    TraitSummary basalMetabolism;
    TraitSummary foodInteractionRadius;

    // Trait diversity metric: average coefficient of variation across traits (dimensionless)
    float traitDiversity{0.0f};

    // Phase 8: Population Demographics & Dynamics
    size_t herbivorePopulation{0};
    size_t predatorPopulation{0};
    int64_t populationGrowthRate{0}; // Change in living population since previous sample
    uint64_t intervalBirths{0};       // Births during this sampling interval
    uint64_t intervalDeaths{0};       // Deaths during this sampling interval
    uint64_t deathsStarvation{0};     // Total deaths caused by energy depletion
    uint64_t deathsPredation{0};      // Total deaths caused by predator capture
    uint64_t intervalStarvationDeaths{0};
    uint64_t intervalPredationDeaths{0};
    bool herbivoreExtinct{false};
    bool predatorExtinct{false};
    float meanGeneration{0.0f};
    uint32_t minGeneration{0};

    // Phase 8: Ecosystem & Resource Dynamics
    size_t foodCount{0};
    size_t maxFoodCapacity{0};
    uint64_t totalFoodConsumed{0};
    uint64_t intervalFoodConsumed{0};
    float totalFoodEnergyGained{0.0f};
    float intervalFoodEnergyGained{0.0f};
    uint64_t totalPredations{0};
    uint64_t intervalPredations{0};
    float totalPredationEnergyGained{0.0f};
    float intervalPredationEnergyGained{0.0f};
    float seasonFactor{0.0f};
    float foodRegenMultiplier{1.0f};
    std::string seasonName{"Summer"};

    // Phase 8: Lineage & Reproductive Success
    double averageLifespan{0.0};
    float meanOffspringCount{0.0f};
    uint32_t maxOffspringCount{0};
    float reproductiveFraction{0.0f};

    // Species breakdown
    std::array<SpeciesMetrics, static_cast<size_t>(SpeciesId::Count)> speciesMetrics;
};

using ScientificSnapshot = EvolutionSnapshot;

} // namespace alife::biology
