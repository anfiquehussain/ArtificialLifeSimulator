#include "alife/environment/FoodManager.hpp"
#include <algorithm>

namespace alife::environment {

FoodManager::FoodManager(const core::AppConfig& config)
    : config_(config),
      foodGrid_(config.worldWidth, config.worldHeight, 100.0f) {
    foodItems_.reserve(config.maxFoodCapacity);
}

void FoodManager::initialize(core::Rng& rng) {
    foodItems_.clear();
    foodGrid_.clear();
    nextFoodId_ = 1;
    regenerationTimer_ = 0.0f;
    totalFoodConsumed_ = 0;
    totalFoodEnergyGained_ = 0.0f;
    totalFoodSpawned_ = 0;

    float margin = config_.foodRadius * 4.0f;
    float minX = margin;
    float maxX = config_.worldWidth - margin;
    float minY = margin;
    float maxY = config_.worldHeight - margin;

    for (uint32_t i = 0; i < config_.initialFoodCount; ++i) {
        float x = rng.randomFloat(minX, maxX);
        float y = rng.randomFloat(minY, maxY);
        spawnFood({x, y}, config_.foodEnergyValue);
    }
    rebuildGrid();
}

void FoodManager::reset(core::Rng& rng) {
    initialize(rng);
}

entities::FoodId FoodManager::spawnFood(const core::Vector2f& pos, float energy) {
    if (foodItems_.size() >= config_.maxFoodCapacity) {
        return 0; // Capacity limit reached
    }

    entities::FoodId id = nextFoodId_++;
    foodItems_.emplace_back(id, pos, energy);
    foodGrid_.insert(static_cast<uint32_t>(foodItems_.size() - 1), pos.x, pos.y);
    ++totalFoodSpawned_;
    return id;
}

void FoodManager::updateRegeneration(float dt, core::Rng& rng, float regenMultiplier) {
    if (foodItems_.size() >= config_.maxFoodCapacity || config_.foodRegenerationInterval <= 0.0f) {
        return;
    }

    // Modulate interval: higher multiplier means faster regeneration
    float effectiveMultiplier = std::max(0.05f, regenMultiplier);
    regenerationTimer_ += dt * effectiveMultiplier;

    bool spawnedAny = false;
    while (regenerationTimer_ >= config_.foodRegenerationInterval) {
        regenerationTimer_ -= config_.foodRegenerationInterval;

        float margin = config_.foodRadius * 4.0f;
        float minX = margin;
        float maxX = config_.worldWidth - margin;
        float minY = margin;
        float maxY = config_.worldHeight - margin;

        for (uint32_t i = 0; i < config_.foodSpawnPerInterval; ++i) {
            if (foodItems_.size() >= config_.maxFoodCapacity) {
                break;
            }
            float x = rng.randomFloat(minX, maxX);
            float y = rng.randomFloat(minY, maxY);
            spawnFood({x, y}, config_.foodEnergyValue);
            spawnedAny = true;
        }
    }
    if (spawnedAny) {
        rebuildGrid();
    }
}

void FoodManager::rebuildGrid() {
    foodGrid_.clear();
    for (uint32_t i = 0; i < static_cast<uint32_t>(foodItems_.size()); ++i) {
        if (!foodItems_[i].isConsumed()) {
            foodGrid_.insert(i, foodItems_[i].getPosition().x, foodItems_[i].getPosition().y);
        }
    }
}

void FoodManager::restoreState(std::vector<entities::Food> foodItems,
                               entities::FoodId nextId,
                               uint64_t totalConsumed,
                               float totalEnergyGained) {
    foodItems_ = std::move(foodItems);
    nextFoodId_ = nextId;
    totalFoodConsumed_ = totalConsumed;
    totalFoodEnergyGained_ = totalEnergyGained;
    regenerationTimer_ = 0.0f;
    rebuildGrid();
}

void FoodManager::resolveConsumption(sim::PopulationManager& population) {
    if (foodItems_.empty()) return;

    if (population.getOrganismGrid().empty() && population.getActiveCount() > 0) {
        population.rebuildGrid();
    }

    auto& organisms = population.getOrganisms();
    const auto& orgGrid = population.getOrganismGrid();

    // For each unconsumed food item, award it to the closest living eligible herbivore within range.
    // Accelerate search using SpatialGrid bounded by MaxInteractionRadius (25.0f).
    for (auto& food : foodItems_) {
        if (food.isConsumed()) continue;

        const auto& foodPos = food.getPosition();
        entities::Organism* closestConsumer = nullptr;
        float minDistanceSq = std::numeric_limits<float>::max();

        orgGrid.queryNearby(foodPos.x, foodPos.y, biology::Traits::MaxInteractionRadius, [&](uint32_t orgIdx) {
            if (orgIdx >= organisms.size()) return;
            auto& org = organisms[orgIdx];
            if (!org.isAlive()) return;
            // Predators do not eat plant food pellets
            if (org.isPredator()) return;

            float radius = org.getTraits().foodInteractionRadius;
            float radiusSq = radius * radius;

            float dx = org.getPosition().x - foodPos.x;
            float dy = org.getPosition().y - foodPos.y;
            float distSq = dx * dx + dy * dy;

            if (distSq <= radiusSq) {
                if (distSq < minDistanceSq || (distSq == minDistanceSq && (closestConsumer == nullptr || org.getId() < closestConsumer->getId()))) {
                    minDistanceSq = distSq;
                    closestConsumer = &org;
                }
            }
        });

        if (closestConsumer != nullptr) {
            float gainedEnergy = food.consume();
            if (gainedEnergy > 0.0f) {
                closestConsumer->gainEnergy(gainedEnergy);
                ++totalFoodConsumed_;
                totalFoodEnergyGained_ += gainedEnergy;
            }
        }
    }

    // Prune consumed food items from active list and rebuild grid
    pruneConsumedFood();
    rebuildGrid();
}

void FoodManager::pruneConsumedFood() {
    std::erase_if(foodItems_, [](const entities::Food& food) {
        return food.isConsumed();
    });
}

} // namespace alife::environment
