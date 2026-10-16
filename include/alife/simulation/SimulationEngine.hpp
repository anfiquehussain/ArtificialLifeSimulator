#pragma once

#include "alife/core/Clock.hpp"
#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include "alife/data/HistoryRecorder.hpp"
#include "alife/simulation/World.hpp"

namespace alife::sim {

/**
 * @brief SimulationEngine orchestrates the simulation clock, PRNG, world state,
 * and evolutionary telemetry history recording.
 * Strictly decoupled from graphics / SFML.
 */
class SimulationEngine {
public:
    explicit SimulationEngine(const core::AppConfig& config = core::AppConfig{});

    // Advance clock by wall-clock time and tick world for each fixed step
    void update(core::SimulationClock::Duration wallDeltaTime);

    // Controls
    void pause() noexcept { clock_.pause(); }
    void resume() noexcept { clock_.resume(); }
    void togglePause() noexcept { clock_.togglePause(); }
    void reset() noexcept;
    void stepOnce();

    // Speed multiplier (0.5x, 1x, 2x, 5x, 10x, etc.)
    void setSpeedMultiplier(double mult) noexcept { clock_.setSpeedMultiplier(mult); }
    [[nodiscard]] double getSpeedMultiplier() const noexcept { return clock_.getSpeedMultiplier(); }

    // Reconfiguration and preset application
    void applyConfig(const core::AppConfig& config, bool resetWorld = false);

    // Snapshot restoration
    void restoreState(double simTime, uint64_t stepCount, bool isPaused) noexcept {
        clock_.setSimulationState(simTime, stepCount, isPaused);
        lastStatsSampleTime_ = simTime;
    }

    // Export history telemetry to CSV (backward compatible)
    bool exportHistoryToCsv(const std::string& filepath) const {
        return historyRecorder_.exportToCsv(filepath, config_);
    }

    // Export comprehensive Phase 8 scientific measurement datasets to target directory
    bool exportAllScientificData(const std::string& directory) const {
        return historyRecorder_.exportAll(directory, config_, getExperimentMetadata());
    }

    // Capture structured experiment record for exact reproducibility
    [[nodiscard]] data::ExperimentMetadata getExperimentMetadata() const;

    // Controls
    void setExperimentId(std::string id) { config_.experimentId = std::move(id); }
    void setRunId(std::string id) { config_.runId = std::move(id); }

    // State queries
    [[nodiscard]] const World& getWorld() const noexcept { return world_; }
    [[nodiscard]] World& getWorld() noexcept { return world_; }
    [[nodiscard]] const core::SimulationClock& getClock() const noexcept { return clock_; }
    [[nodiscard]] const data::HistoryRecorder& getHistoryRecorder() const noexcept { return historyRecorder_; }
    [[nodiscard]] data::HistoryRecorder& getHistoryRecorder() noexcept { return historyRecorder_; }
    [[nodiscard]] const data::EventLogger& getEventLogger() const noexcept { return historyRecorder_.getEventLogger(); }
    [[nodiscard]] data::EventLogger& getEventLogger() noexcept { return historyRecorder_.getEventLogger(); }
    [[nodiscard]] const core::AppConfig& getConfig() const noexcept { return config_; }
    [[nodiscard]] core::Rng& getRng() noexcept { return rng_; }
    [[nodiscard]] bool isPaused() const noexcept { return clock_.isPaused(); }

private:
    void sampleEvolutionaryMetrics();

    core::AppConfig config_;
    core::Rng rng_;
    World world_;
    core::SimulationClock clock_;
    data::HistoryRecorder historyRecorder_;
    double lastStatsSampleTime_{0.0};
};

} // namespace alife::sim
