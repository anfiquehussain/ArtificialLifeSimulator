#pragma once

#include <chrono>
#include <cstdint>
#include <algorithm>

namespace alife::core {

/**
 * @brief Simulation clock that isolates simulation time from wall-clock time.
 * Decoupled from any rendering framework.
 */
class SimulationClock {
public:
    using Duration = std::chrono::duration<double>;

    explicit SimulationClock(double fixedTimeStep = 1.0 / 60.0, double maxAccumulator = 0.25);

    // Timing control
    void update(Duration wallDeltaTime);
    void pause() noexcept;
    void resume() noexcept;
    void togglePause() noexcept;
    void reset() noexcept;

    // Time advancement check
    [[nodiscard]] bool checkStep();

    // Accessors
    [[nodiscard]] bool isPaused() const noexcept { return isPaused_; }
    [[nodiscard]] double getFixedTimeStep() const noexcept { return fixedTimeStep_; }
    [[nodiscard]] double getSimulationTime() const noexcept { return simulationTime_; }
    [[nodiscard]] uint64_t getStepCount() const noexcept { return stepCount_; }
    [[nodiscard]] double getAccumulator() const noexcept { return accumulator_; }

    // Phase 10: Speed multiplier and manual single-step
    void setSpeedMultiplier(double multiplier) noexcept {
        speedMultiplier_ = std::clamp(multiplier, 0.1, 50.0);
    }
    [[nodiscard]] double getSpeedMultiplier() const noexcept { return speedMultiplier_; }
    bool stepOnce() noexcept;

    // Snapshot restoration
    void setSimulationState(double simTime, uint64_t stepCount, bool isPaused = true) noexcept {
        simulationTime_ = simTime;
        stepCount_ = stepCount;
        accumulator_ = 0.0;
        isPaused_ = isPaused;
    }

    void setFixedTimeStep(double step) noexcept;

private:
    double fixedTimeStep_{1.0 / 60.0};
    double maxAccumulator_{0.25}; // Prevent spiral of death on long pauses or frame hitches
    double accumulator_{0.0};
    double simulationTime_{0.0};
    uint64_t stepCount_{0};
    double speedMultiplier_{1.0};
    bool isPaused_{false};
};

} // namespace alife::core
