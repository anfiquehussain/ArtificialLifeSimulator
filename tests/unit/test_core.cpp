#include "TestUtils.hpp"
#include "alife/core/Clock.hpp"
#include <cmath>

int main() {
    using namespace alife::core;

    std::cout << "--- Running test_core (SimulationClock) ---\n";

    // 1. Initial state
    SimulationClock clock(1.0 / 60.0, 0.25);
    ALIFE_TEST_ASSERT(!clock.isPaused(), "Clock starts unpaused");
    ALIFE_TEST_ASSERT(clock.getSimulationTime() == 0.0, "Initial simulation time is 0");
    ALIFE_TEST_ASSERT(clock.getStepCount() == 0, "Initial step count is 0");
    ALIFE_TEST_ASSERT(std::abs(clock.getFixedTimeStep() - (1.0 / 60.0)) < 1e-9, "Default fixed step is 1/60s");

    // 2. Accumulation and step trigger
    clock.update(SimulationClock::Duration(0.010)); // 10ms: less than 16.66ms
    ALIFE_TEST_ASSERT(!clock.checkStep(), "No step triggered when accumulator < fixedTimeStep");
    ALIFE_TEST_ASSERT(clock.getStepCount() == 0, "Step count remains 0");

    clock.update(SimulationClock::Duration(0.010)); // +10ms: total 20ms >= 16.66ms
    ALIFE_TEST_ASSERT(clock.checkStep(), "Step triggered when accumulator >= fixedTimeStep");
    ALIFE_TEST_ASSERT(clock.getStepCount() == 1, "Step count incremented to 1");
    ALIFE_TEST_ASSERT(std::abs(clock.getSimulationTime() - (1.0 / 60.0)) < 1e-9, "Simulation time advanced by fixed step");
    ALIFE_TEST_ASSERT(!clock.checkStep(), "No second step if remainder < fixedTimeStep");

    // 3. Pause behavior
    clock.pause();
    ALIFE_TEST_ASSERT(clock.isPaused(), "Clock is paused");
    clock.update(SimulationClock::Duration(1.0)); // 1 second passes while paused
    ALIFE_TEST_ASSERT(!clock.checkStep(), "No step allowed while paused");
    ALIFE_TEST_ASSERT(clock.getStepCount() == 1, "Step count does not advance while paused");
    ALIFE_TEST_ASSERT(std::abs(clock.getSimulationTime() - (1.0 / 60.0)) < 1e-9, "Sim time unchanged while paused");

    // 4. Resume behavior (no catch-up burst)
    clock.resume();
    ALIFE_TEST_ASSERT(!clock.isPaused(), "Clock resumed");
    ALIFE_TEST_ASSERT(clock.getAccumulator() == 0.0, "Accumulator reset on resume, preventing burst");
    ALIFE_TEST_ASSERT(!clock.checkStep(), "No instant step on resume");

    // 5. Large elapsed-time interval clamping (max accumulator)
    clock.update(SimulationClock::Duration(10.0)); // 10 seconds sudden jump
    ALIFE_TEST_ASSERT(clock.getAccumulator() <= 0.25 + 1e-6, "Accumulator clamped to maxAccumulator (0.25s)");

    int drainedSteps = 0;
    while (clock.checkStep()) {
        ++drainedSteps;
    }
    // 0.25 / (1/60) = 15 steps max
    ALIFE_TEST_ASSERT(drainedSteps == 15, "Clamped accumulator drains exactly 15 steps instead of 600");

    // 6. Reset behavior
    clock.reset();
    ALIFE_TEST_ASSERT(clock.getSimulationTime() == 0.0, "Reset restores simulation time to 0");
    ALIFE_TEST_ASSERT(clock.getStepCount() == 0, "Reset restores step count to 0");
    ALIFE_TEST_ASSERT(clock.getAccumulator() == 0.0, "Reset clears accumulator");
    ALIFE_TEST_ASSERT(!clock.isPaused(), "Reset leaves clock unpaused");

    alife::test::reportSummary();
    return alife::test::failedTests > 0 ? 1 : 0;
}
