#include "alife/ai/Sensors.hpp"
#include "alife/entities/Organism.hpp"
#include "alife/simulation/SpatialGrid.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace alife::ai {

namespace {
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;

inline float normalizeAngleDiff(float angleA, float angleB) noexcept {
    float diff = angleA - angleB;
    while (diff > PI) diff -= TWO_PI;
    while (diff < -PI) diff += TWO_PI;
    float norm = diff / PI;
    if (std::isnan(norm) || std::isinf(norm)) return 0.0f;
    return std::clamp(norm, -1.0f, 1.0f);
}
}

std::vector<float> computeSensors(const core::Vector2f& orgPos,
                                  const core::Vector2f& orgDir,
                                  float orgEnergy,
                                  float orgMaxEnergy,
                                  const std::vector<entities::Food>& foods,
                                  const SensorConfig& config) {
    std::vector<float> buffer(6, 0.0f);
    computeSensors(orgPos, orgDir, orgEnergy, orgMaxEnergy, foods, config, buffer);
    return buffer;
}

void computeSensors(const core::Vector2f& orgPos,
                    const core::Vector2f& orgDir,
                    float orgEnergy,
                    float orgMaxEnergy,
                    const std::vector<entities::Food>& foods,
                    const SensorConfig& config,
                    std::vector<float>& outBuffer) {
    if (outBuffer.size() < 6) {
        outBuffer.resize(6, 0.0f);
    }

    // 0. Energy ratio in [0, 1]
    float energyRatio = (orgMaxEnergy > 0.0f) ? (orgEnergy / orgMaxEnergy) : 0.0f;
    if (std::isnan(energyRatio) || std::isinf(energyRatio)) energyRatio = 0.0f;
    outBuffer[0] = std::clamp(energyRatio, 0.0f, 1.0f);

    // 1-3. Find nearest unconsumed food within perception radius
    float perceptionRadius = (config.perceptionRadius > 0.0f) ? config.perceptionRadius : 120.0f;
    float maxDistSq = perceptionRadius * perceptionRadius;
    float minDistSq = std::numeric_limits<float>::max();
    core::Vector2f nearestFoodPos{0.0f, 0.0f};
    bool foodFound = false;

    for (const auto& food : foods) {
        if (food.isConsumed()) continue;

        const auto& fPos = food.getPosition();
        float dx = fPos.x - orgPos.x;
        float dy = fPos.y - orgPos.y;
        float distSq = dx * dx + dy * dy;

        if (distSq <= maxDistSq && distSq < minDistSq) {
            minDistSq = distSq;
            nearestFoodPos = fPos;
            foodFound = true;
        }
    }

    if (foodFound) {
        float dist = std::sqrt(minDistSq);
        float foodDistNorm = dist / perceptionRadius;
        if (std::isnan(foodDistNorm) || std::isinf(foodDistNorm)) foodDistNorm = 1.0f;
        outBuffer[2] = std::clamp(foodDistNorm, 0.0f, 1.0f);

        // Relative angle to food
        float dx = nearestFoodPos.x - orgPos.x;
        float dy = nearestFoodPos.y - orgPos.y;
        float foodAngle = std::atan2(dy, dx);
        float orgHeading = std::atan2(orgDir.y, orgDir.x);
        outBuffer[1] = normalizeAngleDiff(foodAngle, orgHeading);
        outBuffer[3] = 1.0f; // Food detected flag
    } else {
        outBuffer[1] = 0.0f;  // Relative food angle when no food detected
        outBuffer[2] = 1.0f;  // Food distance normalized (1.0 = at or beyond limit)
        outBuffer[3] = -1.0f; // Food detected flag (-1.0 = none)
    }

    // 4. Horizontal boundary normalized in [-1, 1]
    if (config.worldWidth > 0.0f) {
        float boundX = (2.0f * orgPos.x / config.worldWidth) - 1.0f;
        if (std::isnan(boundX) || std::isinf(boundX)) boundX = 0.0f;
        outBuffer[4] = std::clamp(boundX, -1.0f, 1.0f);
    } else {
        outBuffer[4] = 0.0f;
    }

    // 5. Vertical boundary normalized in [-1, 1]
    if (config.worldHeight > 0.0f) {
        float boundY = (2.0f * orgPos.y / config.worldHeight) - 1.0f;
        if (std::isnan(boundY) || std::isinf(boundY)) boundY = 0.0f;
        outBuffer[5] = std::clamp(boundY, -1.0f, 1.0f);
    } else {
        outBuffer[5] = 0.0f;
    }
}

void computeEcologicalSensors(const entities::Organism& org,
                              const std::vector<entities::Food>& foods,
                              const std::vector<entities::Organism>& organisms,
                              float seasonFactor,
                              const SensorConfig& config,
                              std::vector<float>& outBuffer,
                              const sim::SpatialGrid* foodGrid,
                              const sim::SpatialGrid* orgGrid) {
    if (outBuffer.size() < 8) {
        outBuffer.resize(8, 0.0f);
    }

    const auto& orgPos = org.getPosition();
    const auto& orgDir = org.getDirection();
    float orgHeading = std::atan2(orgDir.y, orgDir.x);

    // 0. Energy ratio
    float energyRatio = (org.getMaxEnergy() > 0.0f) ? (org.getEnergy() / org.getMaxEnergy()) : 0.0f;
    if (std::isnan(energyRatio) || std::isinf(energyRatio)) energyRatio = 0.0f;
    outBuffer[0] = std::clamp(energyRatio, 0.0f, 1.0f);

    float perceptionRadius = (config.perceptionRadius > 0.0f) ? config.perceptionRadius : 120.0f;
    float maxDistSq = perceptionRadius * perceptionRadius;

    // 1-3. Target search: Food for Herbivore, Prey (Herbivore) for Predator
    bool targetFound = false;
    float minTargetDistSq = std::numeric_limits<float>::max();
    core::Vector2f targetPos{0.0f, 0.0f};

    if (org.isHerbivore()) {
        if (foodGrid != nullptr) {
            foodGrid->queryNearby(orgPos.x, orgPos.y, perceptionRadius, [&](uint32_t fIdx) {
                if (fIdx >= foods.size()) return;
                const auto& food = foods[fIdx];
                if (food.isConsumed()) return;
                float dx = food.getPosition().x - orgPos.x;
                float dy = food.getPosition().y - orgPos.y;
                float distSq = dx * dx + dy * dy;
                if (distSq <= maxDistSq && distSq < minTargetDistSq) {
                    minTargetDistSq = distSq;
                    targetPos = food.getPosition();
                    targetFound = true;
                }
            });
        } else {
            for (const auto& food : foods) {
                if (food.isConsumed()) continue;
                float dx = food.getPosition().x - orgPos.x;
                float dy = food.getPosition().y - orgPos.y;
                float distSq = dx * dx + dy * dy;
                if (distSq <= maxDistSq && distSq < minTargetDistSq) {
                    minTargetDistSq = distSq;
                    targetPos = food.getPosition();
                    targetFound = true;
                }
            }
        }
    } else {
        // Predator target: nearest living Herbivore
        if (orgGrid != nullptr) {
            orgGrid->queryNearby(orgPos.x, orgPos.y, perceptionRadius, [&](uint32_t cIdx) {
                if (cIdx >= organisms.size()) return;
                const auto& candidate = organisms[cIdx];
                if (!candidate.isAlive() || candidate.getId() == org.getId()) return;
                if (candidate.isHerbivore()) {
                    float dx = candidate.getPosition().x - orgPos.x;
                    float dy = candidate.getPosition().y - orgPos.y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq <= maxDistSq && distSq < minTargetDistSq) {
                        minTargetDistSq = distSq;
                        targetPos = candidate.getPosition();
                        targetFound = true;
                    }
                }
            });
        } else {
            for (const auto& candidate : organisms) {
                if (!candidate.isAlive() || candidate.getId() == org.getId()) continue;
                if (candidate.isHerbivore()) {
                    float dx = candidate.getPosition().x - orgPos.x;
                    float dy = candidate.getPosition().y - orgPos.y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq <= maxDistSq && distSq < minTargetDistSq) {
                        minTargetDistSq = distSq;
                        targetPos = candidate.getPosition();
                        targetFound = true;
                    }
                }
            }
        }
    }

    if (targetFound) {
        float dist = std::sqrt(minTargetDistSq);
        float normDist = dist / perceptionRadius;
        if (std::isnan(normDist) || std::isinf(normDist)) normDist = 1.0f;
        outBuffer[2] = std::clamp(normDist, 0.0f, 1.0f);

        float targetAngle = std::atan2(targetPos.y - orgPos.y, targetPos.x - orgPos.x);
        outBuffer[1] = normalizeAngleDiff(targetAngle, orgHeading);
        outBuffer[3] = 1.0f;
    } else {
        outBuffer[1] = 0.0f;
        outBuffer[2] = 1.0f;
        outBuffer[3] = -1.0f;
    }

    // 4-6. Threat search: Predator for Herbivore, Rival Predator for Predator
    bool threatFound = false;
    float minThreatDistSq = std::numeric_limits<float>::max();
    core::Vector2f threatPos{0.0f, 0.0f};

    if (org.isHerbivore()) {
        if (orgGrid != nullptr) {
            orgGrid->queryNearby(orgPos.x, orgPos.y, perceptionRadius, [&](uint32_t cIdx) {
                if (cIdx >= organisms.size()) return;
                const auto& candidate = organisms[cIdx];
                if (!candidate.isAlive() || candidate.getId() == org.getId()) return;
                if (candidate.isPredator()) {
                    float dx = candidate.getPosition().x - orgPos.x;
                    float dy = candidate.getPosition().y - orgPos.y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq <= maxDistSq && distSq < minThreatDistSq) {
                        minThreatDistSq = distSq;
                        threatPos = candidate.getPosition();
                        threatFound = true;
                    }
                }
            });
        } else {
            for (const auto& candidate : organisms) {
                if (!candidate.isAlive() || candidate.getId() == org.getId()) continue;
                if (candidate.isPredator()) {
                    float dx = candidate.getPosition().x - orgPos.x;
                    float dy = candidate.getPosition().y - orgPos.y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq <= maxDistSq && distSq < minThreatDistSq) {
                        minThreatDistSq = distSq;
                        threatPos = candidate.getPosition();
                        threatFound = true;
                    }
                }
            }
        }
    } else {
        // For Predator: rival predator in perception range
        if (orgGrid != nullptr) {
            orgGrid->queryNearby(orgPos.x, orgPos.y, perceptionRadius, [&](uint32_t cIdx) {
                if (cIdx >= organisms.size()) return;
                const auto& candidate = organisms[cIdx];
                if (!candidate.isAlive() || candidate.getId() == org.getId()) return;
                if (candidate.isPredator()) {
                    float dx = candidate.getPosition().x - orgPos.x;
                    float dy = candidate.getPosition().y - orgPos.y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq <= maxDistSq && distSq < minThreatDistSq) {
                        minThreatDistSq = distSq;
                        threatPos = candidate.getPosition();
                        threatFound = true;
                    }
                }
            });
        } else {
            for (const auto& candidate : organisms) {
                if (!candidate.isAlive() || candidate.getId() == org.getId()) continue;
                if (candidate.isPredator()) {
                    float dx = candidate.getPosition().x - orgPos.x;
                    float dy = candidate.getPosition().y - orgPos.y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq <= maxDistSq && distSq < minThreatDistSq) {
                        minThreatDistSq = distSq;
                        threatPos = candidate.getPosition();
                        threatFound = true;
                    }
                }
            }
        }
    }

    if (threatFound) {
        float dist = std::sqrt(minThreatDistSq);
        float normDist = dist / perceptionRadius;
        if (std::isnan(normDist) || std::isinf(normDist)) normDist = 1.0f;
        outBuffer[5] = std::clamp(normDist, 0.0f, 1.0f);

        float threatAngle = std::atan2(threatPos.y - orgPos.y, threatPos.x - orgPos.x);
        outBuffer[4] = normalizeAngleDiff(threatAngle, orgHeading);
        outBuffer[6] = 1.0f;
    } else {
        outBuffer[4] = 0.0f;
        outBuffer[5] = 1.0f;
        outBuffer[6] = -1.0f;
    }

    // 7. Environmental season factor
    if (std::isnan(seasonFactor) || std::isinf(seasonFactor)) seasonFactor = 0.0f;
    outBuffer[7] = std::clamp(seasonFactor, -1.0f, 1.0f);
}

} // namespace alife::ai
