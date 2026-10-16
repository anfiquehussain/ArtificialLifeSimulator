#include "alife/core/Clock.hpp"
#include <algorithm>

namespace alife::core {

SimulationClock::SimulationClock(double fixedTimeStep, double maxAccumulator)
    : fixedTimeStep_(fixedTimeStep > 0.0 ? fixedTimeStep : 1.0 / 60.0),
      maxAccumulator_(maxAccumulator > 0.0 ? maxAccumulator : 0.25) {}

void SimulationClock::update(Duration wallDeltaTime) {
    if (isPaused_) {
        return;
    }

    double dt = wallDeltaTime.count() * speedMultiplier_;
    if (dt < 0.0) {
        dt = 0.0;
    }

    // Accumulate wall time clamped to avoid spiral of death
    accumulator_ += dt;
    if (accumulator_ > maxAccumulator_) {
        accumulator_ = maxAccumulator_;
    }
}

void SimulationClock::pause() noexcept {
    isPaused_ = true;
    accumulator_ = 0.0; // Clear accumulator on pause so unpausing doesn't jump
}

void SimulationClock::resume() noexcept {
    isPaused_ = false;
    accumulator_ = 0.0; // Clear accumulator on resume to prevent large catch-up
}

void SimulationClock::togglePause() noexcept {
    if (isPaused_) {
        resume();
    } else {
        pause();
    }
}

void SimulationClock::reset() noexcept {
    accumulator_ = 0.0;
    simulationTime_ = 0.0;
    stepCount_ = 0;
    // Note: keep isPaused_ as is or unpaused as initial state
    isPaused_ = false;
}

bool SimulationClock::checkStep() {
    if (isPaused_) {
        return false;
    }

    if (accumulator_ >= fixedTimeStep_) {
        accumulator_ -= fixedTimeStep_;
        simulationTime_ += fixedTimeStep_;
        ++stepCount_;
        return true;
    }

    return false;
}

bool SimulationClock::stepOnce() noexcept {
    simulationTime_ += fixedTimeStep_;
    ++stepCount_;
    accumulator_ = 0.0;
    return true;
}

void SimulationClock::setFixedTimeStep(double step) noexcept {
    if (step > 0.0) {
        fixedTimeStep_ = step;
    }
}

} // namespace alife::core
