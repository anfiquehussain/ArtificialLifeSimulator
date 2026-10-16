#pragma once

#include "alife/core/Vector2.hpp"
#include <cstdint>

namespace alife::entities {

using FoodId = uint64_t;

/**
 * @brief Food item entity representing an edible nutrient resource.
 * Completely decoupled from SFML and rendering.
 */
class Food {
public:
    Food(FoodId id, const core::Vector2f& position, float energyValue);

    [[nodiscard]] FoodId getId() const noexcept { return id_; }
    [[nodiscard]] const core::Vector2f& getPosition() const noexcept { return position_; }
    [[nodiscard]] float getEnergyValue() const noexcept { return energyValue_; }
    [[nodiscard]] bool isConsumed() const noexcept { return isConsumed_; }

    // Consumes the food item, returns available energy. Returns 0 if already consumed.
    float consume() noexcept;

private:
    FoodId id_{0};
    core::Vector2f position_{0.0f, 0.0f};
    float energyValue_{25.0f};
    bool isConsumed_{false};
};

} // namespace alife::entities
