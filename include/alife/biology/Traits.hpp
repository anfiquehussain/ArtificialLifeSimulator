#pragma once

#include "alife/core/Rng.hpp"
#include <algorithm>

namespace alife::biology {

/**
 * @brief Heritable traits passed from parent to offspring.
 * Encapsulates mutation logic and bounds enforcement.
 */
struct Traits {
    float speed{50.0f};                  // Movement speed (pixels / sec)
    float basalMetabolismCost{1.5f};     // Basal energy burn rate (/ sec)
    float foodInteractionRadius{10.0f};  // Food detection / eat distance (pixels)

    // Mutation bounds
    static constexpr float MinSpeed = 15.0f;
    static constexpr float MaxSpeed = 150.0f;
    static constexpr float MinBasalCost = 0.5f;
    static constexpr float MaxBasalCost = 5.0f;
    static constexpr float MinInteractionRadius = 5.0f;
    static constexpr float MaxInteractionRadius = 25.0f;

    // Mutates traits with given probability and relative magnitude
    [[nodiscard]] Traits mutate(core::Rng& rng, float mutationProb, float mutationMagnitude) const {
        Traits result = *this;

        // Speed mutation
        if (rng.randomFloat(0.0f, 1.0f) < mutationProb) {
            float delta = rng.randomFloat(-mutationMagnitude, mutationMagnitude) * result.speed;
            result.speed = std::clamp(result.speed + delta, MinSpeed, MaxSpeed);
        }

        // Basal cost mutation
        if (rng.randomFloat(0.0f, 1.0f) < mutationProb) {
            float delta = rng.randomFloat(-mutationMagnitude, mutationMagnitude) * result.basalMetabolismCost;
            result.basalMetabolismCost = std::clamp(result.basalMetabolismCost + delta, MinBasalCost, MaxBasalCost);
        }

        // Interaction radius mutation
        if (rng.randomFloat(0.0f, 1.0f) < mutationProb) {
            float delta = rng.randomFloat(-mutationMagnitude, mutationMagnitude) * result.foodInteractionRadius;
            result.foodInteractionRadius = std::clamp(result.foodInteractionRadius + delta, MinInteractionRadius, MaxInteractionRadius);
        }

        return result;
    }
};

} // namespace alife::biology
