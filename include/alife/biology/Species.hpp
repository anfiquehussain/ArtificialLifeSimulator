#pragma once

#include <cstdint>
#include <string>

namespace alife::biology {

/**
 * @brief Identifies the ecological role and species of an organism.
 */
enum class SpeciesId : uint8_t {
    Herbivore = 0,
    Predator = 1,
    Count = 2
};

/**
 * @brief High-level ecological classification defining foraging and trophic behavior.
 */
enum class EcologicalRole : uint8_t {
    ResourceConsumer, // Forages on plant/food resources (Herbivore)
    Predator          // Hunts and consumes other organisms (Carnivore)
};

/**
 * @brief Configuration defining species-level rules, physiological parameters, and dietary compatibility.
 */
struct SpeciesConfig {
    SpeciesId id{SpeciesId::Herbivore};
    std::string name{"Herbivore"};
    EcologicalRole role{EcologicalRole::ResourceConsumer};

    // Initial trait distributions and population bounds
    uint32_t initialPopulation{45};
    uint32_t maxPopulation{120};

    float minSpeed{30.0f};
    float maxSpeed{80.0f};
    float defaultSpeed{50.0f};
    float basalMetabolismCost{1.5f};
    float bodyRadius{6.0f};

    // Sensory perception and interaction distances
    float perceptionRadius{120.0f};
    float interactionRadius{10.0f}; // Eating or capture radius

    // Energy limits
    float initialEnergy{80.0f};
    float maxEnergy{100.0f};

    // Reproduction parameters
    float reproductionEnergyThreshold{80.0f};
    float reproductionEnergyCost{45.0f};
    float offspringInitialEnergy{35.0f};
    float reproductionCooldown{3.0f};

    // Dietary & Predation compatibility rules
    bool canEatFoodPellets{true};    // Can consume environmental food
    bool canPreyOnHerbivores{false};  // Can hunt and eat herbivores
    bool canPreyOnPredators{false};   // Can hunt other predators (cannibalism)
    float predationEnergyGain{50.0f}; // Energy acquired per prey captured
};

[[nodiscard]] constexpr const char* speciesName(SpeciesId id) noexcept {
    switch (id) {
        case SpeciesId::Herbivore: return "Herbivore";
        case SpeciesId::Predator: return "Predator";
        default: return "Unknown";
    }
}

} // namespace alife::biology
