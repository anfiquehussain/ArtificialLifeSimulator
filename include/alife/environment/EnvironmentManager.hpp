#pragma once

#include "alife/core/Config.hpp"
#include <string>

namespace alife::environment {

/**
 * @brief Manages environmental dynamics, seasonal resource cycles, and climate fluctuations.
 * Driven strictly by simulation time.
 */
class EnvironmentManager {
public:
    explicit EnvironmentManager(const core::AppConfig& config);

    // Updates environmental cycle state for current simulation time
    void update(double simulationTime) noexcept;

    // Resets environmental state
    void reset() noexcept;

    // Queries
    [[nodiscard]] float getSeasonFactor() const noexcept { return seasonFactor_; }
    [[nodiscard]] float getFoodRegenMultiplier() const noexcept { return foodRegenMultiplier_; }
    [[nodiscard]] std::string getSeasonName() const;
    [[nodiscard]] double getCurrentSimulationTime() const noexcept { return currentSimTime_; }

private:
    core::AppConfig config_;
    double currentSimTime_{0.0};
    float seasonFactor_{0.0f};          // In [-1.0, 1.0]: +1 = peak abundance, -1 = peak drought
    float foodRegenMultiplier_{1.0f};   // Modulation multiplier applied to food regeneration
};

} // namespace alife::environment
