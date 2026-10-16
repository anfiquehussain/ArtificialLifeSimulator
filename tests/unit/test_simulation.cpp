#include "TestUtils.hpp"
#include "alife/simulation/SimulationEngine.hpp"

int main() {
    using namespace alife::sim;
    using namespace alife::core;

    std::cout << "--- Running test_simulation (SimulationEngine & World) ---\n";

    AppConfig config;
    config.worldWidth = 1000.0f;
    config.worldHeight = 600.0f;
    config.fixedTimeStep = 0.05; // 20 ticks / sec
    config.maxAccumulator = 0.20;

    SimulationEngine engine(config);

    // 1. Initial configuration
    ALIFE_TEST_ASSERT(engine.getWorld().getWidth() == 1000.0f, "World width initialized correctly");
    ALIFE_TEST_ASSERT(engine.getWorld().getHeight() == 600.0f, "World height initialized correctly");
    ALIFE_TEST_ASSERT(engine.getWorld().getUpdateCount() == 0, "Initial world update count is 0");
    ALIFE_TEST_ASSERT(!engine.isPaused(), "Engine starts unpaused");

    // 2. Stepping through engine update
    engine.update(SimulationClock::Duration(0.105)); // ~2 steps of 0.05
    ALIFE_TEST_ASSERT(engine.getWorld().getUpdateCount() == 2, "World stepped 2 times");
    ALIFE_TEST_ASSERT(engine.getClock().getStepCount() == 2, "Clock recorded 2 steps");

    // 3. Pause
    engine.pause();
    ALIFE_TEST_ASSERT(engine.isPaused(), "Engine is paused");
    engine.update(SimulationClock::Duration(1.0));
    ALIFE_TEST_ASSERT(engine.getWorld().getUpdateCount() == 2, "World did not update while paused");

    // 4. Resume
    engine.resume();
    ALIFE_TEST_ASSERT(!engine.isPaused(), "Engine resumed");
    engine.update(SimulationClock::Duration(0.06)); // 1 step
    ALIFE_TEST_ASSERT(engine.getWorld().getUpdateCount() == 3, "World updated after resume");

    // 5. Reset
    engine.reset();
    ALIFE_TEST_ASSERT(engine.getWorld().getUpdateCount() == 0, "Reset cleared world update count");
    ALIFE_TEST_ASSERT(engine.getClock().getStepCount() == 0, "Reset cleared clock step count");
    ALIFE_TEST_ASSERT(engine.getClock().getSimulationTime() == 0.0, "Reset cleared simulation time");

    alife::test::reportSummary();
    return alife::test::failedTests > 0 ? 1 : 0;
}
