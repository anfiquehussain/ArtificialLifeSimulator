#include "TestUtils.hpp"
#include "alife/core/Clock.hpp"
#include "alife/core/Config.hpp"
#include "alife/data/Presets.hpp"
#include "alife/data/SnapshotManager.hpp"
#include "alife/simulation/SimulationEngine.hpp"
#include <filesystem>
#include <fstream>
#include <cmath>

int main() {
    using namespace alife;

    std::cout << "--- Running test_phase10_lab_integration ---\n";

    // =========================================================================
    // 1. Configuration Validation & Preset Management
    // =========================================================================
    {
        std::cout << "\n[Test 1: Configuration Validation & Presets]\n";
        core::AppConfig validConfig;
        auto valRes = data::validateConfig(validConfig);
        ALIFE_TEST_ASSERT(valRes.valid, "Default AppConfig is valid");

        // Invalid: zero population
        core::AppConfig badPopConfig = validConfig;
        badPopConfig.initialPopulation = 0;
        ALIFE_TEST_ASSERT(!data::validateConfig(badPopConfig).valid, "Rejects initialPopulation = 0");

        // Invalid: mutation probability > 1.0
        core::AppConfig badMutConfig = validConfig;
        badMutConfig.mutationProbability = 1.5f;
        ALIFE_TEST_ASSERT(!data::validateConfig(badMutConfig).valid, "Rejects mutationProbability > 1.0");

        // Invalid: negative speed
        core::AppConfig badSpeedConfig = validConfig;
        badSpeedConfig.minSpeed = -10.0f;
        ALIFE_TEST_ASSERT(!data::validateConfig(badSpeedConfig).valid, "Rejects negative organism speed");

        // Invalid: food capacity smaller than initial food
        core::AppConfig badFoodConfig = validConfig;
        badFoodConfig.maxFoodCapacity = 50;
        badFoodConfig.initialFoodCount = 100;
        ALIFE_TEST_ASSERT(!data::validateConfig(badFoodConfig).valid, "Rejects initialFood > maxFoodCapacity");

        // Check presets
        auto presets = data::getAvailablePresets();
        ALIFE_TEST_ASSERT(presets.size() == 7, "Provides exactly 7 standard experiment presets");

        bool allPresetsValid = true;
        for (const auto& preset : presets) {
            auto res = data::validateConfig(preset.config);
            if (!res.valid) {
                std::cerr << "Preset '" << preset.name << "' failed validation: " << res.errorMessage << "\n";
                allPresetsValid = false;
            }
        }
        ALIFE_TEST_ASSERT(allPresetsValid, "All 7 presets pass strict configuration validation");

        // Lookup presets
        auto pBasic = data::findPreset("basic_evolution");
        ALIFE_TEST_ASSERT(pBasic.has_value() && pBasic->name == "Basic Evolution", "Found preset by id 'basic_evolution'");

        auto pPred = data::findPreset("Predator-Prey Ecosystem");
        ALIFE_TEST_ASSERT(pPred.has_value() && pPred->config.enableMultiSpecies, "Found preset by name 'Predator-Prey Ecosystem'");
    }

    // =========================================================================
    // 2. Simulation Clock: Single-Step & Speed Multipliers
    // =========================================================================
    {
        std::cout << "\n[Test 2: Simulation Clock Single-Step & Speed Multipliers]\n";
        core::SimulationClock clock;
        clock.pause();
        ALIFE_TEST_ASSERT(clock.isPaused(), "Clock starts paused for single-step test");

        uint64_t initialSteps = clock.getStepCount();
        clock.stepOnce();
        ALIFE_TEST_ASSERT(clock.getStepCount() == initialSteps + 1, "stepOnce() advances stepCount by exactly 1 while paused");
        ALIFE_TEST_ASSERT(std::abs(clock.getSimulationTime() - (1.0 / 60.0)) < 1e-6, "stepOnce() advances simulation time by 1/60s");
        ALIFE_TEST_ASSERT(clock.isPaused(), "Clock remains paused after single-step");

        // Speed multiplier
        clock.resume();
        clock.setSpeedMultiplier(2.0);
        ALIFE_TEST_ASSERT(std::abs(clock.getSpeedMultiplier() - 2.0) < 1e-6, "Speed multiplier set to 2.0x");

        // Accumulate 10ms with 2.0x speed multiplier -> effectively 20ms accumulated
        clock.update(core::SimulationClock::Duration(0.010));
        ALIFE_TEST_ASSERT(clock.checkStep(), "Step triggered with 2.0x multiplier after 10ms wall time (>= 16.66ms)");

        // Clamping bounds
        clock.setSpeedMultiplier(0.01);
        ALIFE_TEST_ASSERT(clock.getSpeedMultiplier() >= 0.1, "Speed multiplier clamped to lower bound (0.1x)");
        clock.setSpeedMultiplier(100.0);
        ALIFE_TEST_ASSERT(clock.getSpeedMultiplier() <= 50.0, "Speed multiplier clamped to upper bound (50.0x)");
    }

    // =========================================================================
    // 3. Simulation Snapshot Round-Trip Save & Load
    // =========================================================================
    {
        std::cout << "\n[Test 3: Snapshot Save & Load Round-Trip]\n";
        core::AppConfig config;
        config.randomSeed = 42;
        config.initialHerbivoreCount = 15;
        config.initialPredatorCount = 5;
        config.initialPopulation = 20;
        config.enableMultiSpecies = true;

        sim::SimulationEngine engine(config);

        // Run simulation for 20 ticks
        for (int i = 0; i < 20; ++i) {
            engine.update(core::SimulationClock::Duration(1.0 / 60.0));
        }

        double origSimTime = engine.getClock().getSimulationTime();
        uint64_t origStepCount = engine.getClock().getStepCount();
        size_t origOrgCount = engine.getWorld().getPopulation().getOrganisms().size();
        size_t origFoodCount = engine.getWorld().getFoodManager().getFoodCount();

        const std::string snapshotFile = "test_snapshot_roundtrip.alife";

        // Save snapshot
        auto saveRes = data::SnapshotManager::saveSnapshot(engine, snapshotFile);
        ALIFE_TEST_ASSERT(saveRes.success, "Snapshot saved successfully");
        ALIFE_TEST_ASSERT(std::filesystem::exists(snapshotFile), "Snapshot file exists on disk");

        // Create a completely separate simulation engine with different initial state
        core::AppConfig emptyConfig;
        emptyConfig.initialHerbivoreCount = 2;
        emptyConfig.initialPredatorCount = 1;
        emptyConfig.initialPopulation = 3;
        emptyConfig.enableMultiSpecies = true;
        sim::SimulationEngine restoredEngine(emptyConfig);

        // Load snapshot into restored engine
        auto loadRes = data::SnapshotManager::loadSnapshot(restoredEngine, snapshotFile);
        if (!loadRes.success) {
            std::cout << "DEBUG loadSnapshot failed: " << loadRes.message << "\n";
        }
        ALIFE_TEST_ASSERT(loadRes.success, "Snapshot loaded successfully");
        ALIFE_TEST_ASSERT(loadRes.organismsLoaded == origOrgCount, "Loaded organism count matches original");
        ALIFE_TEST_ASSERT(std::abs(restoredEngine.getClock().getSimulationTime() - origSimTime) < 1e-6, "Restored simulation time matches original");
        ALIFE_TEST_ASSERT(restoredEngine.getClock().getStepCount() == origStepCount, "Restored step count matches original");
        ALIFE_TEST_ASSERT(restoredEngine.getWorld().getFoodManager().getFoodCount() == origFoodCount, "Restored food count matches original");

        // Verify individual organism properties match
        const auto& origOrgs = engine.getWorld().getPopulation().getOrganisms();
        const auto& loadedOrgs = restoredEngine.getWorld().getPopulation().getOrganisms();
        ALIFE_TEST_ASSERT(origOrgs.size() == loadedOrgs.size(), "Exact organism container size match");

        bool traitsMatch = true;
        for (size_t i = 0; i < origOrgs.size(); ++i) {
            if (origOrgs[i].getId() != loadedOrgs[i].getId() ||
                origOrgs[i].getSpeciesId() != loadedOrgs[i].getSpeciesId() ||
                std::abs(origOrgs[i].getEnergy() - loadedOrgs[i].getEnergy()) > 1e-4f ||
                std::abs(origOrgs[i].getPosition().x - loadedOrgs[i].getPosition().x) > 1e-4f ||
                std::abs(origOrgs[i].getPosition().y - loadedOrgs[i].getPosition().y) > 1e-4f) {
                traitsMatch = false;
                break;
            }
        }
        ALIFE_TEST_ASSERT(traitsMatch, "Organism IDs, species, positions, and energy exact match");

        // Verify simulation can continue seamlessly after restoration
        restoredEngine.update(core::SimulationClock::Duration(1.0 / 60.0));
        ALIFE_TEST_ASSERT(restoredEngine.getClock().getStepCount() == origStepCount + 1, "Restored engine continues stepping cleanly");

        // Clean up temporary file
        std::filesystem::remove(snapshotFile);
        std::filesystem::remove(snapshotFile + ".tmp");
    }

    // =========================================================================
    // 4. Rejection of Corrupted or Incompatible Save Files
    // =========================================================================
    {
        std::cout << "\n[Test 4: Rejection of Corrupted & Incompatible Save Files]\n";
        core::AppConfig config;
        sim::SimulationEngine engine(config);

        // 1. Missing file
        auto resMissing = data::SnapshotManager::loadSnapshot(engine, "non_existent_file.alife");
        ALIFE_TEST_ASSERT(!resMissing.success, "Rejects non-existent snapshot file");

        // 2. Incompatible header
        const std::string badHeaderFile = "test_bad_header.alife";
        {
            std::ofstream out(badHeaderFile);
            out << "CORRUPTED_HEADER_V99\n[METADATA]\n";
        }
        auto resBadHeader = data::SnapshotManager::loadSnapshot(engine, badHeaderFile);
        ALIFE_TEST_ASSERT(!resBadHeader.success, "Rejects incompatible format header");
        std::filesystem::remove(badHeaderFile);

        // 3. Truncated file without footer
        const std::string truncatedFile = "test_truncated.alife";
        {
            std::ofstream out(truncatedFile);
            out << "ALIFE_SNAPSHOT_V1\n[METADATA]\nexperimentId test\n[CLOCK]\nsimTime 10.0\n";
        }
        auto resTruncated = data::SnapshotManager::loadSnapshot(engine, truncatedFile);
        ALIFE_TEST_ASSERT(!resTruncated.success, "Rejects truncated file missing ALIFE_SNAPSHOT_END footer");
        std::filesystem::remove(truncatedFile);

        // Verify live engine was NOT corrupted by failed loads
        ALIFE_TEST_ASSERT(engine.getWorld().getPopulation().getOrganisms().size() == config.initialPopulation,
                          "Live world population intact and untouched after load rejections");
    }

    // =========================================================================
    // 5. Safe Organism Inspection Across Entity Removal/Death
    // =========================================================================
    {
        std::cout << "\n[Test 5: Safe Organism Selection Across Death & Removal]\n";
        core::AppConfig config;
        config.initialHerbivoreCount = 5;
        config.initialPredatorCount = 0;
        sim::SimulationEngine engine(config);

        auto& pop = engine.getWorld().getPopulation();
        ALIFE_TEST_ASSERT(!pop.getOrganisms().empty(), "Population has active organisms");

        entities::EntityId targetId = pop.getOrganisms().front().getId();

        // Direct query before death
        bool found = false;
        for (const auto& org : pop.getOrganisms()) {
            if (org.getId() == targetId && org.isAlive()) {
                found = true;
                break;
            }
        }
        ALIFE_TEST_ASSERT(found, "Target organism exists and is alive before death");

        // Deplete energy to force death
        for (auto& org : pop.getOrganisms()) {
            if (org.getId() == targetId) {
                org.consumeEnergy(org.getEnergy() + 10.0f);
            }
        }

        // Tick world so dead organisms are cleaned up
        engine.update(core::SimulationClock::Duration(1.0 / 60.0));

        // Safe query after death
        bool stillAlive = false;
        for (const auto& org : pop.getOrganisms()) {
            if (org.getId() == targetId && org.isAlive()) {
                stillAlive = true;
                break;
            }
        }
        ALIFE_TEST_ASSERT(!stillAlive, "Safely recognizes organism is deceased without dangling reference");
    }

    // =========================================================================
    // 6. Scientific Data Recording & CSV Export Workflow
    // =========================================================================
    {
        std::cout << "\n[Test 6: Scientific Data Recording & CSV Export]\n";
        core::AppConfig config;
        config.randomSeed = 101;
        config.initialHerbivoreCount = 10;
        config.initialPredatorCount = 2;
        config.enableMultiSpecies = true;
        sim::SimulationEngine engine(config);

        // Run simulation for enough steps to collect history
        for (int i = 0; i < 40; ++i) {
            engine.update(core::SimulationClock::Duration(1.0 / 60.0));
        }

        const auto& snapshots = engine.getHistoryRecorder().getSnapshots();
        ALIFE_TEST_ASSERT(!snapshots.empty(), "Historical snapshots recorded during simulation run");

        const std::string exportDir = "test_export_phase10";
        bool exportOk = engine.exportAllScientificData(exportDir);
        ALIFE_TEST_ASSERT(exportOk, "exportAllScientificData succeeded");
        ALIFE_TEST_ASSERT(std::filesystem::exists(exportDir + "/population_history.csv"), "population_history.csv created");
        ALIFE_TEST_ASSERT(std::filesystem::exists(exportDir + "/ecosystem_history.csv"), "ecosystem_history.csv created");
        ALIFE_TEST_ASSERT(std::filesystem::exists(exportDir + "/experiment_metadata.csv"), "experiment_metadata.csv created");

        // Clean up export directory
        std::filesystem::remove_all(exportDir);
    }

    alife::test::reportSummary();
    return alife::test::failedTests > 0 ? 1 : 0;
}
