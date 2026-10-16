#pragma once

#include "alife/core/Vector2.hpp"
#include "alife/entities/Food.hpp"
#include <vector>

namespace alife::entities {
class Organism;
}

namespace alife::sim {
class SpatialGrid;
}

namespace alife::ai {

struct SensorConfig {
    float perceptionRadius{120.0f};
    float worldWidth{1200.0f};
    float worldHeight{800.0f};
};

/**
 * @brief Computes normalized sensory inputs for an organism (Phase 6 baseline).
 * Inputs (vector of 6 floats):
 * [0] Normalized energy: E / maxE in [0.0, 1.0]
 * [1] Relative angle to nearest food pellet within perception radius in [-1.0, 1.0]
 * [2] Normalized distance to nearest food pellet in [0.0, 1.0]
 * [3] Food presence flag: 1.0 if in range, -1.0 if none detected
 * [4] Horizontal boundary position: [-1.0, 1.0]
 * [5] Vertical boundary position: [-1.0, 1.0]
 */
std::vector<float> computeSensors(const core::Vector2f& orgPos,
                                  const core::Vector2f& orgDir,
                                  float orgEnergy,
                                  float orgMaxEnergy,
                                  const std::vector<entities::Food>& foods,
                                  const SensorConfig& config);

void computeSensors(const core::Vector2f& orgPos,
                    const core::Vector2f& orgDir,
                    float orgEnergy,
                    float orgMaxEnergy,
                    const std::vector<entities::Food>& foods,
                    const SensorConfig& config,
                    std::vector<float>& outBuffer);

/**
 * @brief Phase 7 Multi-Species Ecological sensory perception with Phase 9 SpatialGrid acceleration.
 * Inputs (vector of 8 floats):
 * [0] Normalized energy: [0.0, 1.0]
 * [1] Target relative angle (Food for herbivore, Prey for predator): [-1.0, 1.0]
 * [2] Target normalized distance: [0.0, 1.0]
 * [3] Target detected flag: +1.0 if in perception range, -1.0 if none
 * [4] Threat relative angle (Predator for herbivore, Rival for predator): [-1.0, 1.0]
 * [5] Threat normalized distance: [0.0, 1.0]
 * [6] Threat detected flag: +1.0 if in perception range, -1.0 if none
 * [7] Environmental season factor: [-1.0, 1.0] (+1 = abundance, -1 = drought)
 */
void computeEcologicalSensors(const entities::Organism& org,
                              const std::vector<entities::Food>& foods,
                              const std::vector<entities::Organism>& organisms,
                              float seasonFactor,
                              const SensorConfig& config,
                              std::vector<float>& outBuffer,
                              const sim::SpatialGrid* foodGrid = nullptr,
                              const sim::SpatialGrid* orgGrid = nullptr);

} // namespace alife::ai
