#include "TestUtils.hpp"
#include "alife/biology/Traits.hpp"
#include "alife/entities/Organism.hpp"
#include "alife/simulation/PopulationManager.hpp"
#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include <cmath>

int main() {
    using namespace alife;

    std::cout << "--- Running test_reproduction (Inheritance, Mutation & Reproduction) ---\n";

    core::AppConfig config;
    config.initialPopulation = 10;
    config.maxPopulation = 25;
    config.reproductionEnergyThreshold = 80.0f;
    config.reproductionEnergyCost = 40.0f;
    config.offspringInitialEnergy = 30.0f;
    config.reproductionCooldown = 2.0f;
    config.mutationProbability = 0.0f; // Start with no mutation to test pure inheritance
    config.mutationMagnitude = 0.1f;

    // 1. Reproduction eligibility & energy deduction
    biology::Traits parentTraits;
    parentTraits.speed = 60.0f;
    parentTraits.basalMetabolismCost = 1.8f;
    parentTraits.foodInteractionRadius = 12.0f;

    entities::Organism parentOrg(1, {200.0f, 200.0f}, {1.0f, 0.0f}, parentTraits, 70.0f, 100.0f);
    ALIFE_TEST_ASSERT(!parentOrg.canReproduce(config.reproductionEnergyThreshold),
                      "Organism with 70 energy cannot reproduce (threshold = 80)");

    parentOrg.gainEnergy(15.0f); // 85 energy >= 80
    ALIFE_TEST_ASSERT(parentOrg.canReproduce(config.reproductionEnergyThreshold),
                      "Organism with 85 energy is eligible to reproduce");

    // 2. Cooldown check
    parentOrg.resetReproductionCooldown(2.0f);
    ALIFE_TEST_ASSERT(!parentOrg.canReproduce(config.reproductionEnergyThreshold),
                      "Organism with active cooldown cannot reproduce despite sufficient energy");

    parentOrg.update(2.1f, 0.0f, 1000.0f, 1000.0f, 5.0f);
    ALIFE_TEST_ASSERT(parentOrg.getReproductionCooldown() == 0.0f, "Cooldown decremented to zero over time");

    // 3. Dead organism reproduction prohibition
    parentOrg.kill();
    ALIFE_TEST_ASSERT(!parentOrg.canReproduce(config.reproductionEnergyThreshold),
                      "Dead organism cannot reproduce");

    // 4. Trait Inheritance without mutation
    sim::PopulationManager popMgr(config);
    core::Rng rng(42);

    entities::EntityId pId = popMgr.spawnOrganism({400.0f, 400.0f}, {1.0f, 0.0f}, parentTraits, 90.0f, 100.0f, 0, 1);
    ALIFE_TEST_ASSERT(popMgr.getActiveCount() == 1, "Initial active count is 1");

    popMgr.updateReproduction(rng, config.worldWidth, config.worldHeight);
    ALIFE_TEST_ASSERT(popMgr.getActiveCount() == 2, "Offspring created, count is 2");
    ALIFE_TEST_ASSERT(popMgr.getTotalBirths() == 1, "Births counter incremented to 1");

    const auto& parentRef = popMgr.getOrganisms()[0];
    const auto& childRef = popMgr.getOrganisms()[1];

    // Verify parent energy deducted
    ALIFE_TEST_ASSERT(std::abs(parentRef.getEnergy() - 50.0f) < 1e-4f, "Parent energy deducted by reproduction cost (90 - 40 = 50)");
    ALIFE_TEST_ASSERT(parentRef.getReproductionCooldown() == config.reproductionCooldown, "Parent reproduction cooldown initiated");

    // Verify offspring lineage
    ALIFE_TEST_ASSERT(childRef.getParentId() == pId, "Child records correct parent ID");
    ALIFE_TEST_ASSERT(childRef.getGeneration() == 2, "Child generation is parent generation + 1");
    ALIFE_TEST_ASSERT(childRef.getId() != parentRef.getId(), "Child has unique organism ID");
    ALIFE_TEST_ASSERT(childRef.getEnergy() == config.offspringInitialEnergy, "Child receives configured initial energy (30)");

    // Verify pure trait inheritance (mutationProb = 0)
    ALIFE_TEST_ASSERT(childRef.getTraits().speed == parentTraits.speed, "Child inherited exact speed");
    ALIFE_TEST_ASSERT(childRef.getTraits().basalMetabolismCost == parentTraits.basalMetabolismCost, "Child inherited exact basal cost");
    ALIFE_TEST_ASSERT(childRef.getTraits().foodInteractionRadius == parentTraits.foodInteractionRadius, "Child inherited exact radius");

    // 5. Mutation logic and range enforcement
    core::Rng rngMut(999);
    biology::Traits baseTraits;
    baseTraits.speed = 100.0f;
    baseTraits.basalMetabolismCost = 2.0f;
    baseTraits.foodInteractionRadius = 15.0f;

    // Mutate with 100% probability and 20% magnitude
    biology::Traits mutated = baseTraits.mutate(rngMut, 1.0f, 0.20f);
    ALIFE_TEST_ASSERT(mutated.speed != baseTraits.speed, "Speed changed under 100% mutation probability");
    ALIFE_TEST_ASSERT(mutated.speed >= biology::Traits::MinSpeed && mutated.speed <= biology::Traits::MaxSpeed,
                      "Mutated speed remains within valid bounds");
    ALIFE_TEST_ASSERT(mutated.basalMetabolismCost >= biology::Traits::MinBasalCost && 
                      mutated.basalMetabolismCost <= biology::Traits::MaxBasalCost,
                      "Mutated basal cost remains within valid bounds");
    ALIFE_TEST_ASSERT(mutated.foodInteractionRadius >= biology::Traits::MinInteractionRadius && 
                      mutated.foodInteractionRadius <= biology::Traits::MaxInteractionRadius,
                      "Mutated interaction radius remains within valid bounds");

    // 6. Population Cap Enforcement
    config.maxPopulation = 3;
    sim::PopulationManager popCapped(config);
    popCapped.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, parentTraits, 90.0f, 100.0f);
    popCapped.spawnOrganism({200.0f, 200.0f}, {1.0f, 0.0f}, parentTraits, 90.0f, 100.0f);
    popCapped.spawnOrganism({300.0f, 300.0f}, {1.0f, 0.0f}, parentTraits, 90.0f, 100.0f);
    // Already at maxPopulation = 3, none should be added
    popCapped.updateReproduction(rng, config.worldWidth, config.worldHeight);
    ALIFE_TEST_ASSERT(popCapped.getActiveCount() == 3, "Population cap strictly enforced; no offspring added beyond cap");
    ALIFE_TEST_ASSERT(popCapped.getTotalBirths() == 0, "No births recorded when at population cap");

    // 7. Determinism with identical seeds
    core::Rng rngSeedA(777);
    core::Rng rngSeedB(777);
    config.maxPopulation = 100;
    config.mutationProbability = 0.5f;

    sim::PopulationManager popA(config);
    sim::PopulationManager popB(config);
    popA.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, parentTraits, 90.0f, 100.0f);
    popB.spawnOrganism({100.0f, 100.0f}, {1.0f, 0.0f}, parentTraits, 90.0f, 100.0f);

    popA.updateReproduction(rngSeedA, config.worldWidth, config.worldHeight);
    popB.updateReproduction(rngSeedB, config.worldWidth, config.worldHeight);

    ALIFE_TEST_ASSERT(popA.getOrganisms()[1].getTraits().speed == popB.getOrganisms()[1].getTraits().speed,
                      "Identical random seeds generate identical mutated traits in offspring");

    // 8. Safe Offspring Creation during active population update
    // Update call executes movement, food eating, and reproduction without crashing or invalidating memory
    popA.update(0.1f, config.worldWidth, config.worldHeight);
    ALIFE_TEST_ASSERT(popA.getActiveCount() == 2, "Active count stable after update cycle");

    alife::test::reportSummary();
    return alife::test::failedTests > 0 ? 1 : 0;
}
