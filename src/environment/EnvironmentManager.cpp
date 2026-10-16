#include "alife/environment/EnvironmentManager.hpp"
#include <cmath>
#include <algorithm>

namespace alife::environment {

namespace {
constexpr double TWO_PI = 6.28318530717958647692;
}

EnvironmentManager::EnvironmentManager(const core::AppConfig& config)
    : config_(config) {
    reset();
}

void EnvironmentManager::reset() noexcept {
    currentSimTime_ = 0.0;
    seasonFactor_ = 0.0f;
    foodRegenMultiplier_ = 1.0f;
}

void EnvironmentManager::update(double simulationTime) noexcept {
    currentSimTime_ = simulationTime;

    if (!config_.enableSeasons || config_.seasonCyclePeriod <= 0.0f) {
        seasonFactor_ = 0.0f;
        foodRegenMultiplier_ = 1.0f;
        return;
    }

    // Periodic sinusoidal seasonal oscillation based on simulation time
    double phase = (simulationTime / static_cast<double>(config_.seasonCyclePeriod)) * TWO_PI;
    seasonFactor_ = static_cast<float>(std::sin(phase));

    // Calculate regeneration multiplier: e.g. with intensity 0.5, ranges between 0.5x and 1.5x
    float intensity = std::clamp(config_.seasonIntensity, 0.0f, 0.95f);
    foodRegenMultiplier_ = 1.0f + intensity * seasonFactor_;
}

std::string EnvironmentManager::getSeasonName() const {
    if (!config_.enableSeasons) {
        return "Stable Environment";
    }
    if (seasonFactor_ > 0.35f) {
        return "Abundance (Wet Season)";
    }
    if (seasonFactor_ < -0.35f) {
        return "Scarcity (Drought Season)";
    }
    return "Temperate Season";
}

} // namespace alife::environment
