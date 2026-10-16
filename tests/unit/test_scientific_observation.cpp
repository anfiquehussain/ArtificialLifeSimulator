#include "TestUtils.hpp"
#include "alife/biology/EvolutionMetrics.hpp"
#include "alife/biology/LineageTracker.hpp"
#include "alife/biology/Species.hpp"
#include "alife/data/EventLogger.hpp"
#include "alife/data/ExperimentRecord.hpp"
#include "alife/data/ExperimentComparator.hpp"
#include "alife/data/HistoryRecorder.hpp"
#include "alife/simulation/PopulationManager.hpp"
#include "alife/simulation/SimulationEngine.hpp"
#include <cmath>
#include <fstream>
#include <filesystem>
#include <string>

int main() {
    using namespace alife;

    std::cout << "--- Running test_scientific_observation (Phase 8 Observation & Data Analysis) ---\n";

    // 1. Parametric & Non-Parametric Trait Analysis, Quartiles, and Frequency Bins
    std::vector<float> sampleData = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f};
    auto summary = biology::computeTraitSummary(sampleData);

    ALIFE_TEST_ASSERT(std::abs(summary.mean - 45.0f) < 1e-4f, "Trait mean correctly calculated (45.0)");
    ALIFE_TEST_ASSERT(std::abs(summary.median - 45.0f) < 1e-4f, "Trait median correctly calculated (45.0)");
    ALIFE_TEST_ASSERT(summary.min == 10.0f && summary.max == 80.0f, "Min and max correctly recorded (10.0, 80.0)");
    ALIFE_TEST_ASSERT(summary.q1 == 30.0f, "Quartile Q1 correctly calculated (30.0)");
    ALIFE_TEST_ASSERT(summary.q3 == 70.0f, "Quartile Q3 correctly calculated (70.0)");

    // Total counts in 5 frequency bins must sum to sample size
    uint32_t totalBinCounts = 0;
    for (uint32_t b : summary.histogramBins) {
        totalBinCounts += b;
    }
    ALIFE_TEST_ASSERT(totalBinCounts == sampleData.size(), "Histogram bin counts sum to sample size (8)");
    ALIFE_TEST_ASSERT(summary.variance > 0.0f && summary.stdDev > 0.0f, "Variance and standard deviation calculated");

    // Empty dataset safety
    auto emptySum = biology::computeTraitSummary({});
    ALIFE_TEST_ASSERT(emptySum.mean == 0.0f && emptySum.histogramBins[0] == 0, "Empty dataset handled safely with zero division prevention");

    // 2. Demographic & Reproductive Success Tracking (LineageTracker)
    biology::LineageTracker lineage;
    biology::Traits tr;
    lineage.recordBirth(1, 0, 1, 0.0, tr); // Founder
    lineage.recordBirth(2, 1, 2, 2.0, tr); // Child 1 of Founder
    lineage.recordBirth(3, 1, 2, 4.0, tr); // Child 2 of Founder
    lineage.recordBirth(4, 2, 3, 6.0, tr); // Grandchild (Child of 2)

    lineage.recordDeath(1, 10.0, entities::DeathCause::Starvation); // Lived 10s
    lineage.recordDeath(3, 8.0, entities::DeathCause::Predation);   // Lived 4s (8 - 4)

    ALIFE_TEST_ASSERT(lineage.getTotalRecorded() == 4, "4 individuals recorded in lineage history");
    ALIFE_TEST_ASSERT(std::abs(lineage.getAverageLifespan() - 7.0) < 1e-4, "Average lifespan calculated accurately across deaths (7.0s)");
    // Individuals with offspring: 1 (2 children) and 2 (1 child). Breeders = 2/4 = 0.50
    ALIFE_TEST_ASSERT(std::abs(lineage.getReproductiveFraction() - 0.50f) < 1e-4f, "Reproductive fraction accurately computed (50%)");
    ALIFE_TEST_ASSERT(std::abs(lineage.getMeanOffspringCount() - 0.75f) < 1e-4f, "Mean offspring count accurately computed (3/4 = 0.75)");
    ALIFE_TEST_ASSERT(lineage.getMaxOffspringCount() == 2, "Max offspring count matches parent 1 (2)");
    ALIFE_TEST_ASSERT(lineage.getDeathsByCause(entities::DeathCause::Starvation) == 1, "Starvation deaths recorded accurately (1)");
    ALIFE_TEST_ASSERT(lineage.getDeathsByCause(entities::DeathCause::Predation) == 1, "Predation deaths recorded accurately (1)");

    // 3. Multi-Species Population Dynamics & Species-Level Metrics
    core::AppConfig config;
    config.initialHerbivoreCount = 10;
    config.initialPredatorCount = 5;
    config.initialPopulation = 15;
    config.enableMultiSpecies = true;

    sim::PopulationManager popMgr(config);
    core::Rng rng(42);
    popMgr.initialize(rng, 0.0);

    ALIFE_TEST_ASSERT(popMgr.getHerbivoreCount() == 10, "Initial herbivore count accurately recorded (10)");
    ALIFE_TEST_ASSERT(popMgr.getPredatorCount() == 5, "Initial predator count accurately recorded (5)");
    ALIFE_TEST_ASSERT(popMgr.getActiveCount() == 15, "Total living population matches sum of species (15)");

    auto initialSnap = popMgr.computeEvolutionSnapshot(0.0, 0);
    ALIFE_TEST_ASSERT(initialSnap.populationSize == 15, "Snapshot living population is 15");
    ALIFE_TEST_ASSERT(initialSnap.herbivorePopulation == 10, "Snapshot herbivore population is 10");
    ALIFE_TEST_ASSERT(initialSnap.predatorPopulation == 5, "Snapshot predator population is 5");
    ALIFE_TEST_ASSERT(!initialSnap.herbivoreExtinct && !initialSnap.predatorExtinct, "Neither species marked extinct at start");
    ALIFE_TEST_ASSERT(initialSnap.speciesMetrics[0].speciesId == biology::SpeciesId::Herbivore, "SpeciesMetrics[0] corresponds to Herbivore");
    ALIFE_TEST_ASSERT(initialSnap.speciesMetrics[1].speciesId == biology::SpeciesId::Predator, "SpeciesMetrics[1] corresponds to Predator");
    ALIFE_TEST_ASSERT(initialSnap.speciesMetrics[0].population == 10, "Herbivore species metrics population is 10");
    ALIFE_TEST_ASSERT(initialSnap.speciesMetrics[1].population == 5, "Predator species metrics population is 5");

    // 4. Explicit Death Cause Attribution (Starvation vs Predation)
    // Starvation test: reduce herbivore energy to zero
    auto& herbivore = popMgr.getOrganisms()[0];
    herbivore.consumeEnergy(500.0f); // Drain all energy
    ALIFE_TEST_ASSERT(!herbivore.isAlive(), "Herbivore energy depleted");
    ALIFE_TEST_ASSERT(herbivore.getDeathCause() == entities::DeathCause::Starvation, "Death cause explicitly marked as Starvation");

    popMgr.update(0.016f, 1000.0f, 1000.0f, 1.0); // Prunes dead organisms
    ALIFE_TEST_ASSERT(popMgr.getDeathsByStarvation() == 1, "Starvation deaths incremented to 1");
    ALIFE_TEST_ASSERT(popMgr.getStarvationDeathsBySpecies(biology::SpeciesId::Herbivore) == 1, "Herbivore starvation deaths is 1");

    // Predation test: spawn predator close to herbivore and resolve predation
    popMgr.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, {70.0f, 2.0f, 15.0f}, 50.0f, 100.0f, 0, 0, 2.0, {}, biology::SpeciesId::Predator);
    popMgr.spawnOrganism({105.0f, 100.0f}, {1.0f, 0.0f}, {50.0f, 1.5f, 10.0f}, 50.0f, 100.0f, 0, 0, 2.0, {}, biology::SpeciesId::Herbivore);

    popMgr.resolvePredation();
    ALIFE_TEST_ASSERT(popMgr.getTotalPredations() == 1, "Predation interaction successfully resolved");
    ALIFE_TEST_ASSERT(popMgr.getTotalPredationEnergyGained() == config.predatorEnergyGain, "Predator energy transfer accounted");

    popMgr.update(0.016f, 1000.0f, 1000.0f, 3.0); // Prunes killed prey
    ALIFE_TEST_ASSERT(popMgr.getDeathsByPredation() == 1, "Predation deaths incremented to 1");
    ALIFE_TEST_ASSERT(popMgr.getPredationDeathsBySpecies(biology::SpeciesId::Herbivore) == 1, "Herbivore predation death recorded");

    // 5. Extinction Detection and Recovery Tracking
    // Empty all predators
    for (auto& org : popMgr.getOrganisms()) {
        if (org.isPredator()) {
            org.kill(entities::DeathCause::Starvation);
        }
    }
    popMgr.update(0.016f, 1000.0f, 1000.0f, 5.0);
    ALIFE_TEST_ASSERT(popMgr.isExtinct(biology::SpeciesId::Predator), "Predator species detected as extinct");
    ALIFE_TEST_ASSERT(popMgr.getExtinctionTime(biology::SpeciesId::Predator) == 5.0, "Extinction timestamp accurately recorded (5.0s)");

    // Population recovery: spawn new predator
    popMgr.spawnOrganism({200.0f, 200.0f}, {1.0f, 0.0f}, {60.0f, 2.0f, 15.0f}, 60.0f, 100.0f, 0, 0, 6.0, {}, biology::SpeciesId::Predator);
    ALIFE_TEST_ASSERT(!popMgr.isExtinct(biology::SpeciesId::Predator), "Predator species recovered from extinction");
    ALIFE_TEST_ASSERT(popMgr.getExtinctionTime(biology::SpeciesId::Predator) < 0.0, "Extinction time reset to active state (-1.0)");

    // 6. Structured Event Logging & Memory Bounding
    data::EventLogger logger(5); // Bound capacity = 5
    logger.logEvent(1.0, 60, data::EventType::Birth, 10, 0, biology::SpeciesId::Herbivore, "Birth 1");
    logger.logEvent(2.0, 120, data::EventType::Death, 10, 0, biology::SpeciesId::Herbivore, "Starvation");
    logger.logEvent(3.0, 180, data::EventType::FoodConsumption, 11, 101, biology::SpeciesId::Herbivore, "Pellet");
    logger.logEvent(4.0, 240, data::EventType::Predation, 20, 11, biology::SpeciesId::Predator, "Kill");
    logger.logEvent(5.0, 300, data::EventType::Extinction, 0, 0, biology::SpeciesId::Predator, "Extinct");

    ALIFE_TEST_ASSERT(logger.getEventCount() == 5, "Event log contains 5 entries");

    // 6th event must discard oldest (timestamp 1.0)
    logger.logEvent(6.0, 360, data::EventType::Recovery, 0, 0, biology::SpeciesId::Predator, "Recovered");
    ALIFE_TEST_ASSERT(logger.getEventCount() == 5, "Event log bounded to maximum 5 entries without memory leak");
    ALIFE_TEST_ASSERT(logger.getEvents().front().timestamp == 2.0, "Oldest event discarded in FIFO order");
    ALIFE_TEST_ASSERT(logger.getEvents().back().timestamp == 6.0, "Newest event appended at back");

    // Export events CSV
    std::string testEventCsv = "test_events.csv";
    ALIFE_TEST_ASSERT(logger.exportToCsv(testEventCsv), "Exported events to CSV");
    ALIFE_TEST_ASSERT(std::filesystem::exists(testEventCsv), "Events CSV created on disk");
    std::filesystem::remove(testEventCsv);

    // 7. Historical Measurement Store, Delta Calculations & CSV Exports
    data::HistoryRecorder recorder(100);
    biology::EvolutionSnapshot s1;
    s1.timestamp = 1.0;
    s1.stepCount = 60;
    s1.populationSize = 20;
    s1.totalBirths = 5;
    s1.totalDeaths = 2;
    s1.totalFoodConsumed = 10;
    s1.totalPredations = 3;

    biology::EvolutionSnapshot s2;
    s2.timestamp = 2.0;
    s2.stepCount = 120;
    s2.populationSize = 25; // Growth = +5
    s2.totalBirths = 12;    // Interval births = 7
    s2.totalDeaths = 4;     // Interval deaths = 2
    s2.totalFoodConsumed = 18; // Interval food = 8
    s2.totalPredations = 5;    // Interval predations = 2

    recorder.recordSnapshot(s1);
    recorder.recordSnapshot(s2);

    const auto& recordedS2 = recorder.getSnapshots().back();
    ALIFE_TEST_ASSERT(recordedS2.populationGrowthRate == 5, "Population growth rate correctly computed (+5)");
    ALIFE_TEST_ASSERT(recordedS2.intervalBirths == 7, "Interval births correctly computed (7)");
    ALIFE_TEST_ASSERT(recordedS2.intervalDeaths == 2, "Interval deaths correctly computed (2)");
    ALIFE_TEST_ASSERT(recordedS2.intervalFoodConsumed == 8, "Interval food consumption correctly computed (8)");
    ALIFE_TEST_ASSERT(recordedS2.intervalPredations == 2, "Interval predations correctly computed (2)");

    // Full exportAll directory test
    std::string exportDir = "./test_phase8_exports";
    data::ExperimentMetadata meta;
    meta.experimentId = "Test_Experiment";
    meta.runId = "Test_Run_001";
    meta.randomSeed = 1337;

    bool exportSuccess = recorder.exportAll(exportDir, config, meta);
    ALIFE_TEST_ASSERT(exportSuccess, "exportAll created directory and all 6 scientific data tables");
    ALIFE_TEST_ASSERT(std::filesystem::exists(exportDir + "/population_history.csv"), "population_history.csv exists");
    ALIFE_TEST_ASSERT(std::filesystem::exists(exportDir + "/species_statistics.csv"), "species_statistics.csv exists");
    ALIFE_TEST_ASSERT(std::filesystem::exists(exportDir + "/trait_statistics.csv"), "trait_statistics.csv exists");
    ALIFE_TEST_ASSERT(std::filesystem::exists(exportDir + "/ecosystem_history.csv"), "ecosystem_history.csv exists");
    ALIFE_TEST_ASSERT(std::filesystem::exists(exportDir + "/events.csv"), "events.csv exists");
    ALIFE_TEST_ASSERT(std::filesystem::exists(exportDir + "/experiment_metadata.csv"), "experiment_metadata.csv exists");
    std::filesystem::remove_all(exportDir);

    // 8. Experiment Reproducibility & Statistical Comparison (ExperimentComparator)
    config.randomSeed = 4242;
    config.statsSamplingInterval = 0.2f;
    sim::SimulationEngine engineRunA(config);
    sim::SimulationEngine engineRunB(config); // Identical seed

    config.randomSeed = 9999; // Different seed
    sim::SimulationEngine engineRunC(config);

    // Advance all three engines by 1.0 second
    for (int step = 0; step < 60; ++step) {
        engineRunA.update(core::SimulationClock::Duration(0.0166666));
        engineRunB.update(core::SimulationClock::Duration(0.0166666));
        engineRunC.update(core::SimulationClock::Duration(0.0166666));
    }

    // Comparison A vs B (Identical Seeds)
    auto compIdentical = data::ExperimentComparator::compare(
        engineRunA.getHistoryRecorder().getSnapshots(),
        engineRunB.getHistoryRecorder().getSnapshots(),
        "Run_A", "Run_B"
    );
    ALIFE_TEST_ASSERT(compIdentical.populationRmse == 0.0, "Identical seed runs have zero population RMSE (perfect reproducibility)");
    ALIFE_TEST_ASSERT(compIdentical.maxPopulationDiff == 0, "Identical seed runs have zero population divergence");
    ALIFE_TEST_ASSERT(compIdentical.finalPopDiff == 0, "Identical seed runs have identical final populations");
    ALIFE_TEST_ASSERT(compIdentical.extinctionOutcomeIdentical, "Extinction outcome identical for reproducible seeds");

    // Comparison A vs C (Divergent Seeds)
    auto compDivergent = data::ExperimentComparator::compare(
        engineRunA.getHistoryRecorder().getSnapshots(),
        engineRunC.getHistoryRecorder().getSnapshots(),
        "Run_A", "Run_C"
    );
    ALIFE_TEST_ASSERT(compDivergent.samplesCompared > 0, "Divergent comparison successfully aligned time samples");
    std::string report = data::ExperimentComparator::formatReport(compDivergent);
    ALIFE_TEST_ASSERT(!report.empty(), "Generated formatted scientific comparison report");

    std::string compCsvPath = "test_run_comparison.csv";
    ALIFE_TEST_ASSERT(data::ExperimentComparator::exportToCsv(compCsvPath, compDivergent), "Exported comparison report to CSV");
    ALIFE_TEST_ASSERT(std::filesystem::exists(compCsvPath), "Comparison CSV created on disk");
    std::filesystem::remove(compCsvPath);

    // Export actual simulated dataset to ./data_exports for user inspection
    bool exportedSuite = engineRunA.exportAllScientificData("./data_exports");
    ALIFE_TEST_ASSERT(exportedSuite, "Successfully exported full scientific observation suite to ./data_exports");

    alife::test::reportSummary();
    return alife::test::failedTests > 0 ? 1 : 0;
}
