#pragma once

#include "alife/simulation/SimulationEngine.hpp"
#include <string>

namespace alife::data {

struct SaveResult {
    bool success{false};
    std::string message;
    std::string filepath;
};

struct LoadResult {
    bool success{false};
    std::string message;
    uint32_t organismsLoaded{0};
    uint32_t foodLoaded{0};
    double simulationTime{0.0};
    uint64_t stepCount{0};
};

/**
 * @brief Manages saving and loading of complete simulation snapshots.
 * Implements versioned headers, atomic temporary-file writes, and pre-validation.
 */
class SnapshotManager {
public:
    static constexpr const char* FormatHeader = "ALIFE_SNAPSHOT_V1";
    static constexpr const char* FormatFooter = "ALIFE_SNAPSHOT_END";

    /**
     * @brief Atomically saves the complete world state to the given filepath.
     * Writes to a temporary file (.tmp) first and renames upon verified completion.
     */
    static SaveResult saveSnapshot(const sim::SimulationEngine& engine, const std::string& filepath);

    /**
     * @brief Validates and loads a snapshot from file into the simulation engine.
     * Performs strict integrity checks before applying; leaves live state untouched if loading fails.
     */
    static LoadResult loadSnapshot(sim::SimulationEngine& engine, const std::string& filepath);
};

} // namespace alife::data
