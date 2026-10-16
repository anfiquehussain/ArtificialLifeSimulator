#pragma once

#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include "alife/simulation/PopulationManager.hpp"
#include "alife/environment/FoodManager.hpp"
#include "alife/environment/EnvironmentManager.hpp"
#include <cstdint>

namespace alife::sim {

/**
 * @brief World representation for Phase 7.
 * Manages world dimensions, multi-species population, food resources,
 * and seasonal environmental cycles.
 */
class World {
public:
    explicit World(const core::AppConfig& config = core::AppConfig{});

    [[nodiscard]] float getWidth() const noexcept { return width_; }
    [[nodiscard]] float getHeight() const noexcept { return height_; }

    void setDimensions(float width, float height) noexcept;

    // Advance world: environment, food regeneration, entity movement, consumption, predation, reproduction
    void update(double fixedDeltaTime, core::Rng& rng, double currentTime = 0.0);

    // Reset world state, population, food resources, and environment
    void reset(core::Rng& rng, double currentTime = 0.0);

    [[nodiscard]] uint64_t getUpdateCount() const noexcept { return updateCount_; }
    void restoreState(uint64_t updateCount) noexcept { updateCount_ = updateCount; }
    [[nodiscard]] const PopulationManager& getPopulation() const noexcept { return population_; }
    [[nodiscard]] PopulationManager& getPopulation() noexcept { return population_; }
    [[nodiscard]] const environment::FoodManager& getFoodManager() const noexcept { return foodManager_; }
    [[nodiscard]] environment::FoodManager& getFoodManager() noexcept { return foodManager_; }
    [[nodiscard]] const environment::EnvironmentManager& getEnvironment() const noexcept { return environment_; }
    [[nodiscard]] environment::EnvironmentManager& getEnvironment() noexcept { return environment_; }

private:
    float width_{1200.0f};
    float height_{800.0f};
    uint64_t updateCount_{0};
    environment::EnvironmentManager environment_;
    PopulationManager population_;
    environment::FoodManager foodManager_;
};

} // namespace alife::sim
