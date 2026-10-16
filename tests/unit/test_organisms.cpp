#include "TestUtils.hpp"
#include "alife/entities/Organism.hpp"
#include "alife/simulation/PopulationManager.hpp"
#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include <cmath>

int main() {
    using namespace alife;

    std::cout << "--- Running test_organisms (Organism & PopulationManager) ---\n";

    // 1. Organism Initialization
    biology::Traits initialTraits;
    initialTraits.speed = 50.0f;
    initialTraits.basalMetabolismCost = 2.0f;
    initialTraits.foodInteractionRadius = 10.0f;

    entities::Organism org1(1, {100.0f, 100.0f}, {1.0f, 0.0f}, initialTraits, 100.0f, 100.0f);
    ALIFE_TEST_ASSERT(org1.getId() == 1, "Organism ID is 1");
    ALIFE_TEST_ASSERT(org1.isAlive(), "Organism starts alive");
    ALIFE_TEST_ASSERT(org1.getEnergy() == 100.0f, "Organism initial energy is 100");
    ALIFE_TEST_ASSERT(org1.getSpeed() == 50.0f, "Organism speed is 50");
    ALIFE_TEST_ASSERT(org1.getPosition().x == 100.0f && org1.getPosition().y == 100.0f, "Position initialized");

    // 2. Movement over known simulation interval (dt = 0.5s)
    // speed = 50 -> dx = 25.0
    org1.update(0.5f, 0.0f, 500.0f, 500.0f, 5.0f);
    ALIFE_TEST_ASSERT(std::abs(org1.getPosition().x - 125.0f) < 1e-4f, "Position moved by exactly speed * dt");
    ALIFE_TEST_ASSERT(std::abs(org1.getPosition().y - 100.0f) < 1e-4f, "Y position unchanged for horizontal direction");

    // 3. Movement independence from frame rate
    // Two steps of 0.25s must equal one step of 0.5s
    biology::Traits traitsA; traitsA.speed = 40.0f; traitsA.basalMetabolismCost = 0.0f;
    entities::Organism orgA(2, {0.0f, 0.0f}, {0.0f, 1.0f}, traitsA, 100.0f, 100.0f);
    entities::Organism orgB(3, {0.0f, 0.0f}, {0.0f, 1.0f}, traitsA, 100.0f, 100.0f);
    orgA.update(0.5f, 0.0f, 500.0f, 500.0f, 5.0f);
    orgB.update(0.25f, 0.0f, 500.0f, 500.0f, 5.0f);
    orgB.update(0.25f, 0.0f, 500.0f, 500.0f, 5.0f);
    ALIFE_TEST_ASSERT(std::abs(orgA.getPosition().y - orgB.getPosition().y) < 1e-4f,
                      "Subdivided steps yield identical distance regardless of frame subdivision");

    // 4. Energy consumption model
    // basalCost = 2.0/s, movementCost = 0.1/(s*px), speed = 50 -> cost/s = 2.0 + 5.0 = 7.0
    // dt = 2.0s -> energy consumed = 14.0 -> remaining = 86.0
    biology::Traits traitsEnergy; traitsEnergy.speed = 50.0f; traitsEnergy.basalMetabolismCost = 2.0f;
    entities::Organism orgEnergy(4, {50.0f, 50.0f}, {1.0f, 0.0f}, traitsEnergy, 100.0f, 100.0f);
    orgEnergy.update(2.0f, 0.1f, 500.0f, 500.0f, 5.0f);
    ALIFE_TEST_ASSERT(std::abs(orgEnergy.getEnergy() - 86.0f) < 1e-4f, "Energy consumed matches basal + movement formula");

    // 5. Death at zero energy and no negative energy
    orgEnergy.consumeEnergy(90.0f); // 86.0 - 90.0 -> clamps to 0.0
    ALIFE_TEST_ASSERT(orgEnergy.getEnergy() == 0.0f, "Energy never falls below zero");
    ALIFE_TEST_ASSERT(!orgEnergy.isAlive(), "Organism marked dead when energy reaches 0");

    // 6. Dead organisms do not move or consume energy
    core::Vector2f deadPos = orgEnergy.getPosition();
    orgEnergy.update(1.0f, 0.1f, 500.0f, 500.0f, 5.0f);
    ALIFE_TEST_ASSERT(orgEnergy.getPosition().x == deadPos.x && orgEnergy.getPosition().y == deadPos.y,
                      "Dead organism position does not advance");
    ALIFE_TEST_ASSERT(orgEnergy.getEnergy() == 0.0f, "Dead organism energy stays at 0");

    // 7. World boundary reflection
    biology::Traits traitsBounce; traitsBounce.speed = 20.0f; traitsBounce.basalMetabolismCost = 0.0f;
    entities::Organism orgBounce(5, {498.0f, 100.0f}, {1.0f, 0.0f}, traitsBounce, 100.0f, 100.0f);
    // At worldWidth = 500, radius = 5, right boundary is at 495.
    // Moving right pushes it past boundary, should clamp to 495 and reflect direction.x to negative.
    orgBounce.update(0.5f, 0.0f, 500.0f, 500.0f, 5.0f);
    ALIFE_TEST_ASSERT(orgBounce.getPosition().x <= 495.0f, "Position clamped within boundary");
    ALIFE_TEST_ASSERT(orgBounce.getDirection().x < 0.0f, "Direction reflected horizontally at boundary");

    // 8. Population Manager tests: unique IDs, safe removal, counts
    core::AppConfig config;
    config.initialPopulation = 20;
    config.initialEnergy = 10.0f;
    config.maxEnergy = 10.0f;
    config.basalMetabolismCost = 5.0f; // drains 5 energy / second
    config.movementMetabolismCost = 0.0f;

    sim::PopulationManager popMgr(config);
    core::Rng rng(999);
    popMgr.initialize(rng);

    ALIFE_TEST_ASSERT(popMgr.getActiveCount() == 20, "Population initialized to 20");
    ALIFE_TEST_ASSERT(popMgr.getTotalDeaths() == 0, "Initial deaths is 0");

    // Verify unique IDs
    bool uniqueIds = true;
    const auto& orgs = popMgr.getOrganisms();
    for (size_t i = 0; i < orgs.size(); ++i) {
        for (size_t j = i + 1; j < orgs.size(); ++j) {
            if (orgs[i].getId() == orgs[j].getId()) {
                uniqueIds = false;
            }
        }
    }
    ALIFE_TEST_ASSERT(uniqueIds, "All initialized organisms have unique IDs");

    // 9. Update population and trigger starvation deaths
    // dt = 1.0s -> each organism loses 5 energy -> 5 remaining
    popMgr.update(1.0f, config.worldWidth, config.worldHeight);
    ALIFE_TEST_ASSERT(popMgr.getActiveCount() == 20, "All alive after 1s (5 energy remaining)");

    // dt = 1.5s -> each organism loses 7.5 energy -> reaches 0 -> all die
    popMgr.update(1.5f, config.worldWidth, config.worldHeight);
    ALIFE_TEST_ASSERT(popMgr.getActiveCount() == 0, "All organisms removed safely on starvation");
    ALIFE_TEST_ASSERT(popMgr.getTotalDeaths() == 20, "Total deaths recorded correctly as 20");

    // 10. Empty population update safety
    popMgr.update(1.0f, config.worldWidth, config.worldHeight);
    ALIFE_TEST_ASSERT(popMgr.getActiveCount() == 0, "Empty population handled safely without exceptions");

    // 11. Population Reset restores initial count
    popMgr.reset(rng);
    ALIFE_TEST_ASSERT(popMgr.getActiveCount() == 20, "Reset restores active population to 20");
    ALIFE_TEST_ASSERT(popMgr.getTotalDeaths() == 0, "Reset clears total deaths");

    // 12. Determinism test with identical seeds
    core::Rng rngA(12345);
    core::Rng rngB(12345);
    sim::PopulationManager popA(config);
    sim::PopulationManager popB(config);
    popA.initialize(rngA);
    popB.initialize(rngB);

    bool identicalInit = true;
    for (size_t i = 0; i < popA.getActiveCount(); ++i) {
        if (popA.getOrganisms()[i].getPosition().x != popB.getOrganisms()[i].getPosition().x ||
            popA.getOrganisms()[i].getSpeed() != popB.getOrganisms()[i].getSpeed()) {
            identicalInit = false;
            break;
        }
    }
    ALIFE_TEST_ASSERT(identicalInit, "Identical RNG seeds produce identical initial population states");

    alife::test::reportSummary();
    return alife::test::failedTests > 0 ? 1 : 0;
}
