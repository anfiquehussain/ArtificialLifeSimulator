#include "TestUtils.hpp"
#include "alife/entities/Food.hpp"
#include "alife/environment/FoodManager.hpp"
#include "alife/simulation/PopulationManager.hpp"
#include "alife/simulation/World.hpp"
#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include <cmath>

int main() {
    using namespace alife;

    std::cout << "--- Running test_environment (Food & Resource Systems) ---\n";

    core::AppConfig config;
    config.worldWidth = 1000.0f;
    config.worldHeight = 600.0f;
    config.initialFoodCount = 20;
    config.maxFoodCapacity = 30;
    config.foodEnergyValue = 25.0f;
    config.foodRadius = 4.0f;
    config.foodInteractionRadius = 10.0f;
    config.foodRegenerationInterval = 1.0f; // 1s
    config.foodSpawnPerInterval = 5;

    // 1. Food entity creation & consumption state
    entities::Food food1(101, {200.0f, 200.0f}, 25.0f);
    ALIFE_TEST_ASSERT(food1.getId() == 101, "Food ID is 101");
    ALIFE_TEST_ASSERT(!food1.isConsumed(), "Food starts unconsumed");
    ALIFE_TEST_ASSERT(food1.getEnergyValue() == 25.0f, "Food energy value is 25");
    float consumedEnergy = food1.consume();
    ALIFE_TEST_ASSERT(consumedEnergy == 25.0f, "First consume returns energy value");
    ALIFE_TEST_ASSERT(food1.isConsumed(), "Food is marked consumed");
    float secondConsume = food1.consume();
    ALIFE_TEST_ASSERT(secondConsume == 0.0f, "Duplicate consumption returns 0 energy");

    // 2. FoodManager initial placement & boundaries
    environment::FoodManager foodMgr(config);
    core::Rng rng(42);
    foodMgr.initialize(rng);

    ALIFE_TEST_ASSERT(foodMgr.getFoodCount() == 20, "Initial food count matches config (20)");
    ALIFE_TEST_ASSERT(foodMgr.getTotalFoodConsumed() == 0, "Initial consumed count is 0");

    bool withinBounds = true;
    for (const auto& f : foodMgr.getFoodItems()) {
        if (f.getPosition().x < 0.0f || f.getPosition().x > config.worldWidth ||
            f.getPosition().y < 0.0f || f.getPosition().y > config.worldHeight) {
            withinBounds = false;
        }
    }
    ALIFE_TEST_ASSERT(withinBounds, "All initial food items placed within world boundaries");

    // 3. Determinism with identical seeds
    environment::FoodManager foodMgrA(config);
    environment::FoodManager foodMgrB(config);
    core::Rng rngA(1337);
    core::Rng rngB(1337);
    foodMgrA.initialize(rngA);
    foodMgrB.initialize(rngB);

    bool identicalFood = true;
    for (size_t i = 0; i < foodMgrA.getFoodCount(); ++i) {
        if (foodMgrA.getFoodItems()[i].getPosition().x != foodMgrB.getFoodItems()[i].getPosition().x ||
            foodMgrA.getFoodItems()[i].getPosition().y != foodMgrB.getFoodItems()[i].getPosition().y) {
            identicalFood = false;
            break;
        }
    }
    ALIFE_TEST_ASSERT(identicalFood, "Identical RNG seeds place food at identical coordinates");

    // 4. Food consumption within interaction radius
    sim::PopulationManager popMgr(config);
    // Organism at (100, 100) with 50 energy, max 100
    biology::Traits defaultTraits;
    defaultTraits.speed = 0.0f;
    defaultTraits.foodInteractionRadius = 10.0f;
    popMgr.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, defaultTraits, 50.0f, 100.0f);

    environment::FoodManager customFood(config);
    // Spawn food close at (105, 100) -> distance = 5 <= 10 (radius)
    customFood.spawnFood({105.0f, 100.0f}, 30.0f);
    // Spawn food far away at (200, 200) -> distance ~ 141 > 10
    customFood.spawnFood({200.0f, 200.0f}, 30.0f);

    customFood.resolveConsumption(popMgr);
    ALIFE_TEST_ASSERT(customFood.getFoodCount() == 1, "Nearby food was consumed and pruned; distant food remains");
    ALIFE_TEST_ASSERT(customFood.getTotalFoodConsumed() == 1, "Total food consumed counter incremented to 1");
    ALIFE_TEST_ASSERT(std::abs(popMgr.getOrganisms()[0].getEnergy() - 80.0f) < 1e-4f,
                      "Organism energy increased by food energy value (50 + 30 = 80)");

    // 5. Maximum energy clamping
    // Another food item nearby with 40 energy: 80 + 40 = 120 -> clamp to 100 max
    customFood.spawnFood({102.0f, 100.0f}, 40.0f);
    customFood.resolveConsumption(popMgr);
    ALIFE_TEST_ASSERT(popMgr.getOrganisms()[0].getEnergy() == 100.0f, "Organism energy clamped at maxEnergy (100)");

    // 6. Dead organism cannot recover energy
    popMgr.getOrganisms()[0].kill();
    customFood.spawnFood({100.0f, 100.0f}, 30.0f);
    customFood.resolveConsumption(popMgr);
    ALIFE_TEST_ASSERT(popMgr.getOrganisms()[0].getEnergy() == 0.0f, "Dead organism does not recover energy");
    ALIFE_TEST_ASSERT(customFood.getFoodCount() == 2, "Food was not consumed by dead organism");

    // 7. Simultaneous collision arbitration (multiple organisms on 1 food item)
    sim::PopulationManager popContest(config);
    // Two organisms both at (300, 300) with 20 energy
    popContest.spawnOrganism({300.0f, 300.0f}, {1.0f, 0.0f}, defaultTraits, 20.0f, 100.0f);
    popContest.spawnOrganism({300.0f, 300.0f}, {-1.0f, 0.0f}, defaultTraits, 20.0f, 100.0f);

    environment::FoodManager contestFood(config);
    contestFood.spawnFood({300.0f, 300.0f}, 30.0f); // 1 food item at exact location
    contestFood.resolveConsumption(popContest);

    // Exactly one organism should consume it (the first resolved), the other gains 0
    bool oneGained = (popContest.getOrganisms()[0].getEnergy() == 50.0f && popContest.getOrganisms()[1].getEnergy() == 20.0f) ||
                     (popContest.getOrganisms()[0].getEnergy() == 20.0f && popContest.getOrganisms()[1].getEnergy() == 50.0f);
    ALIFE_TEST_ASSERT(oneGained, "Food consumed at most once under simultaneous collision contention");
    ALIFE_TEST_ASSERT(contestFood.getFoodCount() == 0, "Contested food removed after single consumption");

    // 8. Food regeneration over simulation time
    environment::FoodManager regenMgr(config);
    regenMgr.initialize(rng); // 20 food items
    // dt = 0.5s (less than 1.0s interval) -> no spawn
    regenMgr.updateRegeneration(0.5f, rng);
    ALIFE_TEST_ASSERT(regenMgr.getFoodCount() == 20, "No food spawned before interval elapses");

    // dt = 0.6s (total 1.1s >= 1.0s) -> 1 trigger spawns 5 items -> 25 total
    regenMgr.updateRegeneration(0.6f, rng);
    ALIFE_TEST_ASSERT(regenMgr.getFoodCount() == 25, "5 food items spawned when interval elapsed");

    // 9. Food capacity enforcement
    // Another 2.0s -> 2 triggers of 5 would spawn 10 items (25 + 10 = 35), but maxFoodCapacity is 30
    regenMgr.updateRegeneration(2.0f, rng);
    ALIFE_TEST_ASSERT(regenMgr.getFoodCount() == 30, "Food count strictly capped at maxFoodCapacity (30)");

    // 10. Resource reset restores initial configuration
    regenMgr.reset(rng);
    ALIFE_TEST_ASSERT(regenMgr.getFoodCount() == 20, "Reset restores initial food count (20)");
    ALIFE_TEST_ASSERT(regenMgr.getTotalFoodConsumed() == 0, "Reset clears food consumed counter");

    alife::test::reportSummary();
    return alife::test::failedTests > 0 ? 1 : 0;
}
