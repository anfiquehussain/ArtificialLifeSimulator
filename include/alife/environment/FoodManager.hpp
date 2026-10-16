#pragma once

#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include "alife/entities/Food.hpp"
#include "alife/simulation/PopulationManager.hpp"
#include <vector>
#include <cstdint>

namespace alife::environment {

/**
 * @brief FoodManager manages the food resources in the world:
 * initial spawning, periodic regeneration up to capacity,
 * distance-based consumption arbitration, safe consumed food cleanup,
 * and deterministic reset.
 */
class FoodManager {
public:
    explicit FoodManager(const core::AppConfig& config);

    // Initial spawning and reset
    void initialize(core::Rng& rng);
    void reset(core::Rng& rng);

    // Periodic regeneration (optionally modulated by environmental/seasonal multiplier)
    void updateRegeneration(float dt, core::Rng& rng, float regenMultiplier = 1.0f);

    // Consume food reached by organisms
    // Resolves interactions consistently: each food item is consumed at most once
    void resolveConsumption(sim::PopulationManager& population);

    // Direct manual spawn (useful for testing)
    entities::FoodId spawnFood(const core::Vector2f& pos, float energy);

    // Clean consumed food items from the world
    void pruneConsumedFood();

    // Rebuild spatial grid of active food items
    void rebuildGrid();

    // Phase 10 Snapshot state restoration
    void restoreState(std::vector<entities::Food> foodItems,
                      entities::FoodId nextId,
                      uint64_t totalConsumed = 0,
                      float totalEnergyGained = 0.0f);

    // Queries
    [[nodiscard]] const std::vector<entities::Food>& getFoodItems() const noexcept {
        return foodItems_;
    }

    [[nodiscard]] const sim::SpatialGrid& getFoodGrid() const noexcept {
        return foodGrid_;
    }

    [[nodiscard]] size_t getFoodCount() const noexcept {
        return foodItems_.size();
    }

    [[nodiscard]] uint64_t getTotalFoodConsumed() const noexcept {
        return totalFoodConsumed_;
    }

    [[nodiscard]] float getTotalFoodEnergyGained() const noexcept {
        return totalFoodEnergyGained_;
    }

    [[nodiscard]] uint64_t getTotalFoodSpawned() const noexcept {
        return totalFoodSpawned_;
    }

private:
    core::AppConfig config_;
    std::vector<entities::Food> foodItems_;
    sim::SpatialGrid foodGrid_;
    entities::FoodId nextFoodId_{1};
    float regenerationTimer_{0.0f};
    uint64_t totalFoodConsumed_{0};
    float totalFoodEnergyGained_{0.0f};
    uint64_t totalFoodSpawned_{0};
};

} // namespace alife::environment
