#include "TestUtils.hpp"
#include "alife/biology/EvolutionMetrics.hpp"
#include "alife/biology/LineageTracker.hpp"
#include "alife/data/HistoryRecorder.hpp"
#include "alife/simulation/PopulationManager.hpp"
#include "alife/simulation/SimulationEngine.hpp"
#include <cmath>
#include <fstream>
#include <filesystem>

int main() {
    using namespace alife;

    std::cout << "--- Running test_evolution (Natural Selection & Dynamics) ---\n";

    // 1. Descriptive trait statistics computation
    std::vector<float> sampleSpeeds = {20.0f, 40.0f, 60.0f, 80.0f, 100.0f};
    auto summary = biology::computeTraitSummary(sampleSpeeds);
    ALIFE_TEST_ASSERT(std::abs(summary.mean - 60.0f) < 1e-4f, "Mean computed correctly (60.0)");
    ALIFE_TEST_ASSERT(std::abs(summary.median - 60.0f) < 1e-4f, "Median computed correctly for odd sample (60.0)");
    ALIFE_TEST_ASSERT(summary.min == 20.0f, "Min value matches front (20.0)");
    ALIFE_TEST_ASSERT(summary.max == 100.0f, "Max value matches back (100.0)");
    // Variance = ((40^2 + 20^2 + 0 + 20^2 + 40^2) / 5) = (1600+400+0+400+1600)/5 = 4000/5 = 800.0
    ALIFE_TEST_ASSERT(std::abs(summary.variance - 800.0f) < 1e-2f, "Variance computed correctly (800.0)");
    ALIFE_TEST_ASSERT(std::abs(summary.stdDev - std::sqrt(800.0f)) < 1e-2f, "StdDev matches sqrt(variance)");

    // Even sample median check
    std::vector<float> evenSample = {10.0f, 20.0f, 30.0f, 40.0f};
    auto evenSummary = biology::computeTraitSummary(evenSample);
    ALIFE_TEST_ASSERT(std::abs(evenSummary.median - 25.0f) < 1e-4f, "Median computed correctly for even sample (25.0)");

    // Empty collection handling
    auto emptySummary = biology::computeTraitSummary({});
    ALIFE_TEST_ASSERT(emptySummary.mean == 0.0f && emptySummary.median == 0.0f, "Empty sample handled safely without crash");

    // 2. Lineage tracking and offspring accounting
    biology::LineageTracker lineage;
    biology::Traits t1; t1.speed = 45.0f;
    biology::Traits t2; t2.speed = 55.0f;

    lineage.recordBirth(1, 0, 1, 0.0, t1); // Ancestor
    lineage.recordBirth(2, 1, 2, 5.0, t2); // Child of 1
    lineage.recordBirth(3, 1, 2, 8.0, t2); // Second child of 1

    ALIFE_TEST_ASSERT(lineage.getTotalRecorded() == 3, "Total recorded individuals is 3");
    ALIFE_TEST_ASSERT(lineage.getRecords().at(1).offspringCount == 2, "Parent 1 accurately records 2 offspring");
    ALIFE_TEST_ASSERT(lineage.getRecords().at(2).offspringCount == 0, "Child 2 starts with 0 offspring");

    // Death & Lifespan accounting
    lineage.recordDeath(1, 20.0); // lived 20s
    lineage.recordDeath(2, 15.0); // lived 10s (15 - 5)
    // Avg lifespan of dead organisms = (20 + 10)/2 = 15.0s
    ALIFE_TEST_ASSERT(std::abs(lineage.getAverageLifespan() - 15.0) < 1e-4, "Average lifespan calculated accurately across deaths");

    // 3. Trait consequences on actual simulation behaviour
    // Higher speed organisms must burn more energy than lower speed organisms over the exact same time
    core::AppConfig config;
    config.movementMetabolismCost = 0.05f;

    biology::Traits slowTraits; slowTraits.speed = 30.0f; slowTraits.basalMetabolismCost = 1.0f;
    biology::Traits fastTraits; fastTraits.speed = 100.0f; fastTraits.basalMetabolismCost = 1.0f;

    entities::Organism slowOrg(10, {100.0f, 100.0f}, {1.0f, 0.0f}, slowTraits, 100.0f, 100.0f);
    entities::Organism fastOrg(11, {100.0f, 100.0f}, {1.0f, 0.0f}, fastTraits, 100.0f, 100.0f);

    slowOrg.update(2.0f, config.movementMetabolismCost, 1000.0f, 1000.0f, 5.0f);
    fastOrg.update(2.0f, config.movementMetabolismCost, 1000.0f, 1000.0f, 5.0f);

    // Slow energy: 100 - (1.0 + 0.05*30)*2 = 100 - (2.5)*2 = 95.0
    // Fast energy: 100 - (1.0 + 0.05*100)*2 = 100 - (6.0)*2 = 88.0
    ALIFE_TEST_ASSERT(fastOrg.getEnergy() < slowOrg.getEnergy(),
                      "Higher speed trait causes greater metabolic energy expenditure as intended");
    ALIFE_TEST_ASSERT(fastOrg.getPosition().x > slowOrg.getPosition().x,
                      "Higher speed trait covers greater distance over same time interval");

    // 4. Evolutionary Snapshot generation & diversity
    sim::PopulationManager popMgr(config);
    core::Rng rng(42);
    popMgr.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, slowTraits, 90.0f, 100.0f);
    popMgr.spawnOrganism({200.0f, 200.0f}, {1.0f, 0.0f}, fastTraits, 90.0f, 100.0f);

    auto snap = popMgr.computeEvolutionSnapshot(12.5, 750);
    ALIFE_TEST_ASSERT(snap.timestamp == 12.5, "Snapshot timestamp preserved");
    ALIFE_TEST_ASSERT(snap.populationSize == 2, "Snapshot population size is 2");
    ALIFE_TEST_ASSERT(std::abs(snap.speed.mean - 65.0f) < 1e-4f, "Speed mean computed correctly (65.0)");
    ALIFE_TEST_ASSERT(snap.traitDiversity > 0.0f, "Trait diversity metric CV > 0 for diverse population");

    // 5. HistoryRecorder and CSV Export
    data::HistoryRecorder recorder(100);
    recorder.recordSnapshot(snap);
    ALIFE_TEST_ASSERT(recorder.getSnapshotCount() == 1, "Snapshot recorded in history");

    std::string testCsvPath = "test_telemetry.csv";
    bool exported = recorder.exportToCsv(testCsvPath, config);
    ALIFE_TEST_ASSERT(exported, "CSV export succeeded");
    ALIFE_TEST_ASSERT(std::filesystem::exists(testCsvPath), "CSV file created on disk");

    // Verify CSV content
    std::ifstream csvFile(testCsvPath);
    std::string firstLine;
    std::getline(csvFile, firstLine);
    ALIFE_TEST_ASSERT(firstLine.find("# Experiment:") != std::string::npos, "CSV contains experiment metadata header");
    csvFile.close();
    std::filesystem::remove(testCsvPath);

    // 6. SimulationEngine periodic sampling and determinism
    config.statsSamplingInterval = 0.5f; // sample every 0.5s
    config.randomSeed = 8888;
    sim::SimulationEngine engineA(config);
    sim::SimulationEngine engineB(config);

    // Advance both engines by 1.2s in frame increments (since maxAccumulator = 0.25s)
    // 60 frames * (1/50s = 0.02s) = 1.2s simulation time
    for (int f = 0; f < 60; ++f) {
        engineA.update(core::SimulationClock::Duration(0.02));
        engineB.update(core::SimulationClock::Duration(0.02));
    }

    ALIFE_TEST_ASSERT(engineA.getHistoryRecorder().getSnapshotCount() >= 3,
                      "Periodic telemetry sampled at configured simulation time interval");
    ALIFE_TEST_ASSERT(engineA.getHistoryRecorder().getSnapshotCount() == engineB.getHistoryRecorder().getSnapshotCount(),
                      "Identical sampling count across identical configurations");

    const auto& snapA = engineA.getHistoryRecorder().getSnapshots().back();
    const auto& snapB = engineB.getHistoryRecorder().getSnapshots().back();
    ALIFE_TEST_ASSERT(snapA.speed.mean == snapB.speed.mean, "Deterministic trait statistics produced with identical seeds");

    alife::test::reportSummary();
    return alife::test::failedTests > 0 ? 1 : 0;
}
