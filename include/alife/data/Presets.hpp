#pragma once

#include "alife/core/Config.hpp"
#include <string>
#include <vector>
#include <optional>

namespace alife::data {

struct ValidationResult {
    bool valid{true};
    std::string errorMessage;
};

/**
 * @brief Structured configuration preset representing an experimental scenario.
 */
struct ExperimentPreset {
    std::string id;
    std::string name;
    std::string description;
    core::AppConfig config;
};

/**
 * @brief Validates an AppConfig ensuring all biological and simulation bounds are sound.
 */
[[nodiscard]] ValidationResult validateConfig(const core::AppConfig& config);

/**
 * @brief Returns the complete list of standard experiment presets for Phase 10.
 */
[[nodiscard]] std::vector<ExperimentPreset> getAvailablePresets();

/**
 * @brief Looks up a preset by unique ID or name.
 */
[[nodiscard]] std::optional<ExperimentPreset> findPreset(const std::string& nameOrId);

} // namespace alife::data
