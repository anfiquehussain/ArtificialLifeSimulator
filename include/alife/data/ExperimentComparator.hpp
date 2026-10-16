#pragma once

#include "alife/biology/EvolutionMetrics.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace alife::data {

/**
 * @brief Structured outcome of a statistical comparison between two experiment runs.
 */
struct ComparisonResult {
    std::string runA_id{"Run_A"};
    std::string runB_id{"Run_B"};
    size_t samplesCompared{0};

    // Population trajectory comparison
    double meanPopulationDiff{0.0};  // Mean (PopA - PopB) across aligned samples
    double populationRmse{0.0};      // Root Mean Square Error between trajectories
    int64_t maxPopulationDiff{0};    // Max absolute population difference
    size_t finalPopA{0};
    size_t finalPopB{0};
    int64_t finalPopDiff{0};         // finalPopA - finalPopB

    // Species breakdown comparison
    size_t finalHerbivoreA{0};
    size_t finalHerbivoreB{0};
    size_t finalPredatorA{0};
    size_t finalPredatorB{0};

    // Trait differences at final sample
    float finalMeanSpeedDiff{0.0f};      // SpeedMeanA - SpeedMeanB
    float finalTraitDiversityDiff{0.0f}; // DiversityA - DiversityB

    // Extinction event comparisons
    bool herbivoreExtinctA{false};
    bool herbivoreExtinctB{false};
    bool predatorExtinctA{false};
    bool predatorExtinctB{false};
    bool extinctionOutcomeIdentical{true};
};

/**
 * @brief Utility for comparing two recorded experiment runs.
 */
class ExperimentComparator {
public:
    /**
     * @brief Compare two time-series of scientific snapshots.
     * Aligns samples by index up to min(sizeA, sizeB).
     */
    static ComparisonResult compare(const std::vector<biology::EvolutionSnapshot>& runA,
                                    const std::vector<biology::EvolutionSnapshot>& runB,
                                    const std::string& nameA = "Run_A",
                                    const std::string& nameB = "Run_B") {
        ComparisonResult res;
        res.runA_id = nameA;
        res.runB_id = nameB;

        if (runA.empty() || runB.empty()) {
            return res;
        }

        size_t n = std::min(runA.size(), runB.size());
        res.samplesCompared = n;

        double sumDiff = 0.0;
        double sumSqDiff = 0.0;
        int64_t maxDiff = 0;

        for (size_t i = 0; i < n; ++i) {
            int64_t diff = static_cast<int64_t>(runA[i].populationSize) - static_cast<int64_t>(runB[i].populationSize);
            sumDiff += diff;
            sumSqDiff += static_cast<double>(diff * diff);
            int64_t absDiff = std::abs(diff);
            if (absDiff > maxDiff) {
                maxDiff = absDiff;
            }
        }

        res.meanPopulationDiff = sumDiff / static_cast<double>(n);
        res.populationRmse = std::sqrt(sumSqDiff / static_cast<double>(n));
        res.maxPopulationDiff = maxDiff;

        const auto& finalA = runA[n - 1];
        const auto& finalB = runB[n - 1];

        res.finalPopA = finalA.populationSize;
        res.finalPopB = finalB.populationSize;
        res.finalPopDiff = static_cast<int64_t>(res.finalPopA) - static_cast<int64_t>(res.finalPopB);

        res.finalHerbivoreA = finalA.herbivorePopulation;
        res.finalHerbivoreB = finalB.herbivorePopulation;
        res.finalPredatorA = finalA.predatorPopulation;
        res.finalPredatorB = finalB.predatorPopulation;

        res.finalMeanSpeedDiff = finalA.speed.mean - finalB.speed.mean;
        res.finalTraitDiversityDiff = finalA.traitDiversity - finalB.traitDiversity;

        res.herbivoreExtinctA = finalA.herbivoreExtinct;
        res.herbivoreExtinctB = finalB.herbivoreExtinct;
        res.predatorExtinctA = finalA.predatorExtinct;
        res.predatorExtinctB = finalB.predatorExtinct;

        res.extinctionOutcomeIdentical = (res.herbivoreExtinctA == res.herbivoreExtinctB) &&
                                        (res.predatorExtinctA == res.predatorExtinctB);

        return res;
    }

    /**
     * @brief Format comparison result into a clean text report.
     */
    static std::string formatReport(const ComparisonResult& res) {
        std::ostringstream ss;
        ss << "========================================================\n"
           << "       EXPERIMENT COMPARISON: " << res.runA_id << " vs " << res.runB_id << "\n"
           << "========================================================\n"
           << "Aligned Samples Compared:    " << res.samplesCompared << "\n"
           << "Mean Population Difference:  " << std::fixed << std::setprecision(2) << res.meanPopulationDiff << " organisms\n"
           << "Population Trajectory RMSE:  " << res.populationRmse << "\n"
           << "Max Population Divergence:   " << res.maxPopulationDiff << " organisms\n"
           << "Final Population:            " << res.runA_id << "=" << res.finalPopA << " | "
           << res.runB_id << "=" << res.finalPopB << " (Diff=" << res.finalPopDiff << ")\n"
           << "Final Herbivores:            " << res.runA_id << "=" << res.finalHerbivoreA << " | "
           << res.runB_id << "=" << res.finalHerbivoreB << "\n"
           << "Final Predators:             " << res.runA_id << "=" << res.finalPredatorA << " | "
           << res.runB_id << "=" << res.finalPredatorB << "\n"
           << "Final Mean Speed Delta:      " << std::setprecision(3) << res.finalMeanSpeedDiff << "\n"
           << "Final Trait Diversity Delta: " << res.finalTraitDiversityDiff << "\n"
           << "Extinction Agreement:        " << (res.extinctionOutcomeIdentical ? "Identical" : "Diverged") << "\n"
           << "========================================================\n";
        return ss.str();
    }

    /**
     * @brief Export comparison summary to CSV.
     */
    static bool exportToCsv(const std::string& filepath, const ComparisonResult& res) {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        file << "metric,value\n";
        file << "run_a," << res.runA_id << "\n";
        file << "run_b," << res.runB_id << "\n";
        file << "samples_compared," << res.samplesCompared << "\n";
        file << "mean_population_diff," << std::fixed << std::setprecision(3) << res.meanPopulationDiff << "\n";
        file << "population_rmse," << res.populationRmse << "\n";
        file << "max_population_diff," << res.maxPopulationDiff << "\n";
        file << "final_pop_a," << res.finalPopA << "\n";
        file << "final_pop_b," << res.finalPopB << "\n";
        file << "final_pop_diff," << res.finalPopDiff << "\n";
        file << "final_herbivore_a," << res.finalHerbivoreA << "\n";
        file << "final_herbivore_b," << res.finalHerbivoreB << "\n";
        file << "final_predator_a," << res.finalPredatorA << "\n";
        file << "final_predator_b," << res.finalPredatorB << "\n";
        file << "final_mean_speed_diff," << res.finalMeanSpeedDiff << "\n";
        file << "final_trait_diversity_diff," << res.finalTraitDiversityDiff << "\n";
        file << "extinctions_identical," << (res.extinctionOutcomeIdentical ? "true" : "false") << "\n";

        return true;
    }
};

} // namespace alife::data
