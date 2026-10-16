#include "alife/simulation/PopulationManager.hpp"
#include "alife/ai/Sensors.hpp"
#include "alife/data/EventLogger.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace alife::sim {

PopulationManager::PopulationManager(const core::AppConfig& config)
    : config_(config),
      orgGrid_(config.worldWidth, config.worldHeight, 100.0f) {
    organisms_.reserve(config.maxPopulation);
    offspringQueue_.reserve(config.maxPopulation);
}

void PopulationManager::initialize(core::Rng& rng, double currentTime) {
    organisms_.clear();
    offspringQueue_.clear();
    orgGrid_.clear();
    lineageTracker_.reset();
    nextEntityId_ = 1;
    totalDeaths_ = 0;
    totalSpawned_ = 0;
    totalBirths_ = 0;
    totalPredations_ = 0;
    totalPredationEnergyGained_ = 0.0f;
    deathsStarvation_ = 0;
    deathsPredation_ = 0;
    maxGeneration_ = 0;
    speciesDeaths_.fill(0);
    speciesBirths_.fill(0);
    speciesSpawned_.fill(0);
    speciesStarvationDeaths_.fill(0);
    speciesPredationDeaths_.fill(0);
    extinctionTimes_.fill(-1.0);

    float margin = config_.organismRadius * 3.0f;
    float minX = margin;
    float maxX = config_.worldWidth - margin;
    float minY = margin;
    float maxY = config_.worldHeight - margin;

    bool useMultiSpecies = config_.enableMultiSpecies &&
        (config_.initialHerbivoreCount + config_.initialPredatorCount == config_.initialPopulation);

    if (useMultiSpecies) {
        // 1. Spawn Initial Herbivores
        for (uint32_t i = 0; i < config_.initialHerbivoreCount; ++i) {
            float x = rng.randomFloat(minX, maxX);
            float y = rng.randomFloat(minY, maxY);
            float angle = rng.randomFloat(0.0f, 6.2831853f);
            core::Vector2f dir{std::cos(angle), std::sin(angle)};

            biology::Traits traits;
            traits.speed = rng.randomFloat(config_.minSpeed, config_.maxSpeed);
            traits.basalMetabolismCost = config_.basalMetabolismCost;
            traits.foodInteractionRadius = config_.foodInteractionRadius;

            ai::NeuralNetwork brain(config_.nnInputs, config_.nnHidden, config_.nnOutputs);
            brain.randomize(rng, config_.nnWeightInitRange);

            spawnOrganism({x, y}, dir, traits, config_.initialEnergy, config_.maxEnergy, 0, 0, currentTime, std::move(brain), biology::SpeciesId::Herbivore);
        }

        // 2. Spawn Initial Predators
        for (uint32_t i = 0; i < config_.initialPredatorCount; ++i) {
            float x = rng.randomFloat(minX, maxX);
            float y = rng.randomFloat(minY, maxY);
            float angle = rng.randomFloat(0.0f, 6.2831853f);
            core::Vector2f dir{std::cos(angle), std::sin(angle)};

            biology::Traits traits;
            traits.speed = rng.randomFloat(config_.predatorMinSpeed, config_.predatorMaxSpeed);
            traits.basalMetabolismCost = config_.predatorBasalMetabolismCost;
            traits.foodInteractionRadius = config_.predatorCaptureRadius;

            ai::NeuralNetwork brain(config_.nnInputs, config_.nnHidden, config_.nnOutputs);
            brain.randomize(rng, config_.nnWeightInitRange);

            spawnOrganism({x, y}, dir, traits, config_.initialEnergy, config_.maxEnergy, 0, 0, currentTime, std::move(brain), biology::SpeciesId::Predator);
        }
    } else {
        // Single-species baseline fallback
        for (uint32_t i = 0; i < config_.initialPopulation; ++i) {
            float x = rng.randomFloat(minX, maxX);
            float y = rng.randomFloat(minY, maxY);
            float angle = rng.randomFloat(0.0f, 6.2831853f);
            core::Vector2f dir{std::cos(angle), std::sin(angle)};

            biology::Traits traits;
            traits.speed = rng.randomFloat(config_.minSpeed, config_.maxSpeed);
            traits.basalMetabolismCost = config_.basalMetabolismCost;
            traits.foodInteractionRadius = config_.foodInteractionRadius;

            ai::NeuralNetwork brain(config_.nnInputs, config_.nnHidden, config_.nnOutputs);
            brain.randomize(rng, config_.nnWeightInitRange);

            spawnOrganism({x, y}, dir, traits, config_.initialEnergy, config_.maxEnergy, 0, 0, currentTime, std::move(brain), biology::SpeciesId::Herbivore);
        }
    }
}

void PopulationManager::reset(core::Rng& rng, double currentTime) {
    initialize(rng, currentTime);
}

entities::EntityId PopulationManager::spawnOrganism(const core::Vector2f& pos,
                                                   const core::Vector2f& dir,
                                                   const biology::Traits& traits,
                                                   float energy,
                                                   float maxEnergy,
                                                   entities::EntityId parentId,
                                                   uint32_t generation,
                                                   double birthTime,
                                                   ai::NeuralNetwork brain,
                                                   biology::SpeciesId speciesId) {
    entities::EntityId id = nextEntityId_++;
    organisms_.emplace_back(id, pos, dir, traits, energy, maxEnergy, parentId, generation, std::move(brain), speciesId);
    orgGrid_.insert(static_cast<uint32_t>(organisms_.size() - 1), pos.x, pos.y);
    lineageTracker_.recordBirth(id, parentId, generation, birthTime, traits);
    ++totalSpawned_;
    size_t sIdx = static_cast<size_t>(speciesId);
    ++speciesBirths_[sIdx];
    ++speciesSpawned_[sIdx];
    extinctionTimes_[sIdx] = -1.0; // Mark active / recovered

    if (generation > maxGeneration_) {
        maxGeneration_ = generation;
    }

    if (eventLogger_) {
        eventLogger_->logEvent(birthTime, 0, data::EventType::Birth, id, parentId, speciesId,
                               "Generation: " + std::to_string(generation));
    }

    return id;
}

void PopulationManager::rebuildGrid() {
    orgGrid_.clear();
    for (uint32_t i = 0; i < static_cast<uint32_t>(organisms_.size()); ++i) {
        if (organisms_[i].isAlive()) {
            orgGrid_.insert(i, organisms_[i].getPosition().x, organisms_[i].getPosition().y);
        }
    }
}

void PopulationManager::restoreState(std::vector<entities::Organism> organisms,
                                     entities::EntityId nextEntityId,
                                     uint64_t deaths,
                                     uint64_t spawned,
                                     uint64_t births,
                                     uint64_t predations,
                                     float predationEnergy) {
    organisms_ = std::move(organisms);
    offspringQueue_.clear();
    nextEntityId_ = nextEntityId;
    totalDeaths_ = deaths;
    totalSpawned_ = spawned;
    totalBirths_ = births;
    totalPredations_ = predations;
    totalPredationEnergyGained_ = predationEnergy;

    maxGeneration_ = 0;
    speciesSpawned_.fill(0);
    speciesBirths_.fill(0);
    for (const auto& org : organisms_) {
        if (org.getGeneration() > maxGeneration_) {
            maxGeneration_ = org.getGeneration();
        }
        size_t sIdx = static_cast<size_t>(org.getSpeciesId());
        if (sIdx < speciesSpawned_.size()) {
            ++speciesSpawned_[sIdx];
        }
    }
    rebuildGrid();
}

void PopulationManager::update(float dt, float worldWidth, float worldHeight, double currentTime) {
    static const std::vector<entities::Food> emptyFoods;
    update(dt, worldWidth, worldHeight, emptyFoods, 0.0f, currentTime, nullptr);
}

void PopulationManager::update(float dt,
                               float worldWidth,
                               float worldHeight,
                               const std::vector<entities::Food>& foods,
                               double currentTime) {
    update(dt, worldWidth, worldHeight, foods, 0.0f, currentTime, nullptr);
}

void PopulationManager::update(float dt,
                               float worldWidth,
                               float worldHeight,
                               const std::vector<entities::Food>& foods,
                               float seasonFactor,
                               double currentTime,
                               const sim::SpatialGrid* foodGrid) {
    ai::SensorConfig sensorConfig{
        config_.organismPerceptionRadius,
        worldWidth,
        worldHeight
    };

    // Rebuild SpatialGrid for current living organism positions (zero allocation, clears indices only)
    rebuildGrid();

    // 1. Update all alive organisms (ecological sensor computation, neural network inference, movement, boundaries, metabolism)
    for (auto& org : organisms_) {
        if (!org.isAlive()) continue;

        float bodyRadius = org.isPredator() ? config_.predatorBodyRadius : config_.organismRadius;

        if (!foods.empty() || organisms_.size() > 1) {
            ai::computeEcologicalSensors(org,
                                         foods,
                                         organisms_,
                                         seasonFactor,
                                         sensorConfig,
                                         sensorBuffer_,
                                         foodGrid,
                                         &orgGrid_);
            org.update(dt,
                       config_.movementMetabolismCost,
                       worldWidth,
                       worldHeight,
                       bodyRadius,
                       sensorBuffer_);
        } else {
            org.update(dt,
                       config_.movementMetabolismCost,
                       worldWidth,
                       worldHeight,
                       bodyRadius);
        }
    }

    // 2. Safe removal of dead organisms via std::erase_if (C++20)
    // Record deaths in the lineage tracker and species death metrics before erasing
    for (const auto& org : organisms_) {
        if (!org.isAlive()) {
            auto cause = org.getDeathCause();
            lineageTracker_.recordDeath(org.getId(), currentTime, cause);
            size_t sIdx = static_cast<size_t>(org.getSpeciesId());
            ++speciesDeaths_[sIdx];

            if (cause == entities::DeathCause::Starvation) {
                ++deathsStarvation_;
                ++speciesStarvationDeaths_[sIdx];
            } else if (cause == entities::DeathCause::Predation) {
                ++deathsPredation_;
                ++speciesPredationDeaths_[sIdx];
            }

            if (eventLogger_) {
                eventLogger_->logEvent(currentTime, 0, data::EventType::Death,
                                       org.getId(), 0, org.getSpeciesId(),
                                       entities::deathCauseToString(cause));
            }
        }
    }

    size_t initialCount = organisms_.size();
    std::erase_if(organisms_, [](const entities::Organism& org) {
        return !org.isAlive();
    });

    size_t removed = initialCount - organisms_.size();
    totalDeaths_ += removed;

    // Post-movement / post-removal grid update for subsequent consumption and predation passes
    rebuildGrid();

    // 3. Extinction event tracking
    for (size_t s = 0; s < static_cast<size_t>(biology::SpeciesId::Count); ++s) {
        biology::SpeciesId sp = static_cast<biology::SpeciesId>(s);
        if (speciesSpawned_[s] > 0 && getCountBySpecies(sp) == 0 && extinctionTimes_[s] < 0.0) {
            extinctionTimes_[s] = currentTime;
            if (eventLogger_) {
                eventLogger_->logEvent(currentTime, 0, data::EventType::Extinction,
                                       0, 0, sp, "Extinction event");
            }
        }
    }
}

void PopulationManager::resolvePredation() {
    if (organisms_.empty()) return;

    if (orgGrid_.empty()) {
        rebuildGrid();
    }

    // Iterate through active predators to resolve capture of eligible prey using accelerated SpatialGrid query
    for (auto& predator : organisms_) {
        if (!predator.isAlive() || !predator.isPredator()) continue;

        float captureRadius = predator.getTraits().foodInteractionRadius;
        float captureRadiusSq = captureRadius * captureRadius;

        entities::Organism* closestPrey = nullptr;
        float minPreyDistSq = std::numeric_limits<float>::max();

        orgGrid_.queryNearby(predator.getPosition().x, predator.getPosition().y, captureRadius, [&](uint32_t cIdx) {
            if (cIdx >= organisms_.size()) return;
            auto& candidate = organisms_[cIdx];
            if (!candidate.isAlive() || candidate.getId() == predator.getId()) return;

            // Cannibalism check: predators only hunt herbivores unless cannibalism is explicitly enabled
            if (!config_.allowCannibalism && candidate.isPredator()) return;

            float dx = candidate.getPosition().x - predator.getPosition().x;
            float dy = candidate.getPosition().y - predator.getPosition().y;
            float distSq = dx * dx + dy * dy;

            if (distSq <= captureRadiusSq) {
                if (distSq < minPreyDistSq || (distSq == minPreyDistSq && (closestPrey == nullptr || candidate.getId() < closestPrey->getId()))) {
                    minPreyDistSq = distSq;
                    closestPrey = &candidate;
                }
            }
        });

        if (closestPrey != nullptr) {
            // Predator successfully captures prey: prey is killed with Predation cause
            closestPrey->kill(entities::DeathCause::Predation);
            predator.gainEnergy(config_.predatorEnergyGain);
            ++totalPredations_;
            totalPredationEnergyGained_ += config_.predatorEnergyGain;

            if (eventLogger_) {
                eventLogger_->logEvent(0.0, 0, data::EventType::Predation,
                                       predator.getId(), closestPrey->getId(), predator.getSpeciesId(),
                                       "EnergyGained: " + std::to_string(config_.predatorEnergyGain));
            }
        }
    }
}

void PopulationManager::updateReproduction(core::Rng& rng, float worldWidth, float worldHeight, double currentTime) {
    offspringQueue_.clear();

    // Iterate over active living organisms to check reproduction eligibility
    for (auto& parent : organisms_) {
        if (!parent.isAlive()) continue;

        // Check if global population cap would be exceeded
        if (organisms_.size() + offspringQueue_.size() >= config_.maxPopulation) {
            break;
        }

        float threshold = parent.isPredator() ? config_.predatorReproductionThreshold : config_.reproductionEnergyThreshold;
        float cost = parent.isPredator() ? config_.predatorReproductionCost : config_.reproductionEnergyCost;
        float offspringEnergy = parent.isPredator() ? config_.predatorOffspringEnergy : config_.offspringInitialEnergy;
        float cooldown = config_.reproductionCooldown;
        float bodyRadius = parent.isPredator() ? config_.predatorBodyRadius : config_.organismRadius;

        if (parent.canReproduce(threshold)) {
            // Deduct reproduction energy cost from parent
            parent.consumeEnergy(cost);
            parent.resetReproductionCooldown(cooldown);

            // Inherit traits and apply mutation
            biology::Traits offspringTraits = parent.getTraits().mutate(rng,
                                                                        config_.mutationProbability,
                                                                        config_.mutationMagnitude);

            // Inherit brain and apply mutation to neural network weights/biases
            ai::NeuralNetwork offspringBrain = parent.getBrain().mutate(
                rng,
                config_.nnMutationProbability,
                config_.nnMutationMagnitude,
                config_.nnMaxWeight
            );

            // Calculate offspring initial position near parent with random angular offset
            float angle = rng.randomFloat(0.0f, 6.2831853f);
            float offsetDist = bodyRadius * 2.5f;
            float offX = parent.getPosition().x + std::cos(angle) * offsetDist;
            float offY = parent.getPosition().y + std::sin(angle) * offsetDist;

            // Clamp offspring position within world bounds
            float margin = bodyRadius * 1.5f;
            offX = std::clamp(offX, margin, worldWidth - margin);
            offY = std::clamp(offY, margin, worldHeight - margin);

            // Random initial direction for offspring
            float dirAngle = rng.randomFloat(0.0f, 6.2831853f);
            core::Vector2f dir{std::cos(dirAngle), std::sin(dirAngle)};

            entities::EntityId childId = nextEntityId_++;
            uint32_t childGen = parent.getGeneration() + 1;
            biology::SpeciesId childSpecies = parent.getSpeciesId();

            offspringQueue_.emplace_back(childId,
                                         core::Vector2f{offX, offY},
                                         dir,
                                         offspringTraits,
                                         offspringEnergy,
                                         config_.maxEnergy,
                                         parent.getId(),
                                         childGen,
                                         std::move(offspringBrain),
                                         childSpecies);

            lineageTracker_.recordBirth(childId, parent.getId(), childGen, currentTime, offspringTraits);

            ++totalBirths_;
            ++totalSpawned_;
            size_t cIdx = static_cast<size_t>(childSpecies);
            ++speciesBirths_[cIdx];
            ++speciesSpawned_[cIdx];
            extinctionTimes_[cIdx] = -1.0; // Population recovered

            if (childGen > maxGeneration_) {
                maxGeneration_ = childGen;
            }

            if (eventLogger_) {
                eventLogger_->logEvent(currentTime, 0, data::EventType::Reproduction,
                                       parent.getId(), childId, parent.getSpeciesId(),
                                       "ParentOffspring");
                eventLogger_->logEvent(currentTime, 0, data::EventType::Birth,
                                       childId, parent.getId(), childSpecies,
                                       "Generation: " + std::to_string(childGen));
            }
        }
    }

    // Safely append deferred offspring to active population
    for (auto& child : offspringQueue_) {
        if (organisms_.size() < config_.maxPopulation) {
            organisms_.push_back(std::move(child));
        }
    }
    offspringQueue_.clear();
}

biology::EvolutionSnapshot PopulationManager::computeEvolutionSnapshot(double timestamp, uint64_t stepCount) const {
    biology::EvolutionSnapshot snapshot;
    snapshot.timestamp = timestamp;
    snapshot.stepCount = stepCount;
    snapshot.populationSize = organisms_.size();
    snapshot.totalBirths = totalBirths_;
    snapshot.totalDeaths = totalDeaths_;
    snapshot.maxGeneration = maxGeneration_;
    snapshot.minGeneration = getMinGeneration();
    snapshot.meanGeneration = getMeanGeneration();
    snapshot.deathsStarvation = deathsStarvation_;
    snapshot.deathsPredation = deathsPredation_;
    snapshot.totalPredations = totalPredations_;
    snapshot.totalPredationEnergyGained = totalPredationEnergyGained_;

    snapshot.herbivorePopulation = getHerbivoreCount();
    snapshot.predatorPopulation = getPredatorCount();
    snapshot.herbivoreExtinct = isExtinct(biology::SpeciesId::Herbivore);
    snapshot.predatorExtinct = isExtinct(biology::SpeciesId::Predator);

    snapshot.averageLifespan = lineageTracker_.getAverageLifespan();
    snapshot.meanOffspringCount = lineageTracker_.getMeanOffspringCount();
    snapshot.maxOffspringCount = lineageTracker_.getMaxOffspringCount();
    snapshot.reproductiveFraction = lineageTracker_.getReproductiveFraction();

    // 1. Population-wide trait summaries
    if (!organisms_.empty()) {
        std::vector<float> speedVals;
        std::vector<float> basalVals;
        std::vector<float> radiusVals;
        speedVals.reserve(organisms_.size());
        basalVals.reserve(organisms_.size());
        radiusVals.reserve(organisms_.size());

        for (const auto& org : organisms_) {
            const auto& traits = org.getTraits();
            speedVals.push_back(traits.speed);
            basalVals.push_back(traits.basalMetabolismCost);
            radiusVals.push_back(traits.foodInteractionRadius);
        }

        snapshot.speed = biology::computeTraitSummary(std::move(speedVals));
        snapshot.basalMetabolism = biology::computeTraitSummary(std::move(basalVals));
        snapshot.foodInteractionRadius = biology::computeTraitSummary(std::move(radiusVals));

        float cvSpeed = (snapshot.speed.mean > 1e-4f) ? (snapshot.speed.stdDev / snapshot.speed.mean) : 0.0f;
        float cvBasal = (snapshot.basalMetabolism.mean > 1e-4f) ? (snapshot.basalMetabolism.stdDev / snapshot.basalMetabolism.mean) : 0.0f;
        float cvRadius = (snapshot.foodInteractionRadius.mean > 1e-4f) ? (snapshot.foodInteractionRadius.stdDev / snapshot.foodInteractionRadius.mean) : 0.0f;
        snapshot.traitDiversity = (cvSpeed + cvBasal + cvRadius) / 3.0f;
    }

    // 2. Per-Species breakdown
    for (size_t s = 0; s < static_cast<size_t>(biology::SpeciesId::Count); ++s) {
        biology::SpeciesId sp = static_cast<biology::SpeciesId>(s);
        auto& sm = snapshot.speciesMetrics[s];
        sm.speciesId = sp;
        sm.population = getCountBySpecies(sp);
        sm.cumulativeBirths = speciesBirths_[s];
        sm.cumulativeDeaths = speciesDeaths_[s];
        sm.deathsStarvation = speciesStarvationDeaths_[s];
        sm.deathsPredation = speciesPredationDeaths_[s];
        sm.isExtinct = isExtinct(sp);
        sm.extinctionTime = extinctionTimes_[s];

        std::vector<float> spSpeeds;
        std::vector<float> spBasal;
        std::vector<float> spRadius;
        float spTotalEnergy = 0.0f;
        float spTotalSpeed = 0.0f;

        for (const auto& org : organisms_) {
            if (org.getSpeciesId() == sp) {
                const auto& tr = org.getTraits();
                spSpeeds.push_back(tr.speed);
                spBasal.push_back(tr.basalMetabolismCost);
                spRadius.push_back(tr.foodInteractionRadius);
                spTotalEnergy += org.getEnergy();
                spTotalSpeed += org.getSpeed();
            }
        }

        if (!spSpeeds.empty()) {
            sm.avgEnergy = spTotalEnergy / static_cast<float>(spSpeeds.size());
            sm.avgSpeed = spTotalSpeed / static_cast<float>(spSpeeds.size());
            sm.speed = biology::computeTraitSummary(std::move(spSpeeds));
            sm.basalMetabolism = biology::computeTraitSummary(std::move(spBasal));
            sm.foodInteractionRadius = biology::computeTraitSummary(std::move(spRadius));

            float cvS = (sm.speed.mean > 1e-4f) ? (sm.speed.stdDev / sm.speed.mean) : 0.0f;
            float cvB = (sm.basalMetabolism.mean > 1e-4f) ? (sm.basalMetabolism.stdDev / sm.basalMetabolism.mean) : 0.0f;
            float cvR = (sm.foodInteractionRadius.mean > 1e-4f) ? (sm.foodInteractionRadius.stdDev / sm.foodInteractionRadius.mean) : 0.0f;
            sm.traitDiversity = (cvS + cvB + cvR) / 3.0f;
        }
    }

    return snapshot;
}

size_t PopulationManager::getHerbivoreCount() const noexcept {
    return getCountBySpecies(biology::SpeciesId::Herbivore);
}

size_t PopulationManager::getPredatorCount() const noexcept {
    return getCountBySpecies(biology::SpeciesId::Predator);
}

size_t PopulationManager::getCountBySpecies(biology::SpeciesId id) const noexcept {
    size_t count = 0;
    for (const auto& org : organisms_) {
        if (org.isAlive() && org.getSpeciesId() == id) {
            ++count;
        }
    }
    return count;
}

bool PopulationManager::isExtinct(biology::SpeciesId id) const noexcept {
    return getCountBySpecies(id) == 0 && (totalSpawned_ > 0);
}

uint64_t PopulationManager::getDeathsBySpecies(biology::SpeciesId id) const noexcept {
    size_t idx = static_cast<size_t>(id);
    return idx < speciesDeaths_.size() ? speciesDeaths_[idx] : 0;
}

uint64_t PopulationManager::getBirthsBySpecies(biology::SpeciesId id) const noexcept {
    size_t idx = static_cast<size_t>(id);
    return idx < speciesBirths_.size() ? speciesBirths_[idx] : 0;
}

uint64_t PopulationManager::getStarvationDeathsBySpecies(biology::SpeciesId id) const noexcept {
    size_t idx = static_cast<size_t>(id);
    return idx < speciesStarvationDeaths_.size() ? speciesStarvationDeaths_[idx] : 0;
}

uint64_t PopulationManager::getPredationDeathsBySpecies(biology::SpeciesId id) const noexcept {
    size_t idx = static_cast<size_t>(id);
    return idx < speciesPredationDeaths_.size() ? speciesPredationDeaths_[idx] : 0;
}

double PopulationManager::getExtinctionTime(biology::SpeciesId id) const noexcept {
    size_t idx = static_cast<size_t>(id);
    return idx < extinctionTimes_.size() ? extinctionTimes_[idx] : -1.0;
}

uint32_t PopulationManager::getMinGeneration() const noexcept {
    if (organisms_.empty()) return 0;
    uint32_t minGen = std::numeric_limits<uint32_t>::max();
    for (const auto& org : organisms_) {
        if (org.getGeneration() < minGen) {
            minGen = org.getGeneration();
        }
    }
    return minGen;
}

float PopulationManager::getMeanGeneration() const noexcept {
    if (organisms_.empty()) return 0.0f;
    uint64_t totalGen = 0;
    for (const auto& org : organisms_) {
        totalGen += org.getGeneration();
    }
    return static_cast<float>(totalGen) / static_cast<float>(organisms_.size());
}

float PopulationManager::getAverageEnergy() const noexcept {
    if (organisms_.empty()) return 0.0f;
    float total = 0.0f;
    for (const auto& org : organisms_) {
        total += org.getEnergy();
    }
    return total / static_cast<float>(organisms_.size());
}

float PopulationManager::getAverageSpeed() const noexcept {
    if (organisms_.empty()) return 0.0f;
    float total = 0.0f;
    for (const auto& org : organisms_) {
        total += org.getSpeed();
    }
    return total / static_cast<float>(organisms_.size());
}

} // namespace alife::sim
