#pragma once

#include "alife/core/Config.hpp"
#include "alife/core/Rng.hpp"
#include "alife/entities/Organism.hpp"
#include "alife/entities/Food.hpp"
#include "alife/biology/Species.hpp"
#include "alife/biology/EvolutionMetrics.hpp"
#include "alife/biology/LineageTracker.hpp"
#include "alife/simulation/SpatialGrid.hpp"
#include <vector>
#include <array>
#include <cstdint>

namespace alife::data {
class EventLogger;
}

namespace alife::sim {

/**
 * @brief PopulationManager handles the lifecycle of organisms,
 * multi-species populations (Herbivores and Predators), predation interactions,
 * lineage tracking, neural network inheritance & mutation, and evolutionary metrics.
 */
class PopulationManager {
public:
    explicit PopulationManager(const core::AppConfig& config);

    // Initialize or reset population (spawns both Herbivores and Predators if multi-species is enabled)
    void initialize(core::Rng& rng, double currentTime = 0.0);
    void reset(core::Rng& rng, double currentTime = 0.0);

    // Update active organisms for fixed timestep dt (backward-compatible, no food sensing)
    void update(float dt, float worldWidth, float worldHeight, double currentTime = 0.0);

    // Update active organisms with sensory awareness of food pellets
    void update(float dt,
                float worldWidth,
                float worldHeight,
                const std::vector<entities::Food>& foods,
                double currentTime = 0.0);

    // Update active organisms with full ecological sensory inputs (prey, predators, food, season)
    void update(float dt,
                float worldWidth,
                float worldHeight,
                const std::vector<entities::Food>& foods,
                float seasonFactor,
                double currentTime = 0.0,
                const sim::SpatialGrid* foodGrid = nullptr);

    // Predation resolution: living predators capture eligible living prey within interaction distance
    void resolvePredation();

    // Manual spawn for testing / controls
    entities::EntityId spawnOrganism(const core::Vector2f& pos,
                                     const core::Vector2f& dir,
                                     const biology::Traits& traits,
                                     float energy,
                                     float maxEnergy,
                                     entities::EntityId parentId = 0,
                                     uint32_t generation = 0,
                                     double birthTime = 0.0,
                                     ai::NeuralNetwork brain = ai::NeuralNetwork{},
                                     biology::SpeciesId speciesId = biology::SpeciesId::Herbivore);

    // Reproduction step: checks eligible parents across species and generates offspring
    void updateReproduction(core::Rng& rng, float worldWidth, float worldHeight, double currentTime = 0.0);

    // Evolutionary metrics computation
    [[nodiscard]] biology::EvolutionSnapshot computeEvolutionSnapshot(double timestamp, uint64_t stepCount) const;

    // Queries
    [[nodiscard]] const std::vector<entities::Organism>& getOrganisms() const noexcept {
        return organisms_;
    }

    [[nodiscard]] std::vector<entities::Organism>& getOrganisms() noexcept {
        return organisms_;
    }

    [[nodiscard]] size_t getActiveCount() const noexcept {
        return organisms_.size();
    }

    // Phase 9 SpatialGrid query acceleration
    void rebuildGrid();
    [[nodiscard]] const SpatialGrid& getOrganismGrid() const noexcept {
        return orgGrid_;
    }

    // Phase 10 Snapshot state restoration
    void restoreState(std::vector<entities::Organism> organisms,
                      entities::EntityId nextEntityId,
                      uint64_t deaths = 0,
                      uint64_t spawned = 0,
                      uint64_t births = 0,
                      uint64_t predations = 0,
                      float predationEnergy = 0.0f);

    // Species-specific population queries
    [[nodiscard]] size_t getHerbivoreCount() const noexcept;
    [[nodiscard]] size_t getPredatorCount() const noexcept;
    [[nodiscard]] size_t getCountBySpecies(biology::SpeciesId id) const noexcept;
    [[nodiscard]] bool isExtinct(biology::SpeciesId id) const noexcept;

    [[nodiscard]] uint64_t getTotalDeaths() const noexcept { return totalDeaths_; }
    [[nodiscard]] uint64_t getTotalSpawned() const noexcept { return totalSpawned_; }
    [[nodiscard]] uint64_t getTotalBirths() const noexcept { return totalBirths_; }
    [[nodiscard]] uint64_t getTotalPredations() const noexcept { return totalPredations_; }
    [[nodiscard]] uint64_t getDeathsBySpecies(biology::SpeciesId id) const noexcept;
    [[nodiscard]] uint64_t getBirthsBySpecies(biology::SpeciesId id) const noexcept;
    [[nodiscard]] uint64_t getDeathsByStarvation() const noexcept { return deathsStarvation_; }
    [[nodiscard]] uint64_t getDeathsByPredation() const noexcept { return deathsPredation_; }
    [[nodiscard]] uint64_t getStarvationDeathsBySpecies(biology::SpeciesId id) const noexcept;
    [[nodiscard]] uint64_t getPredationDeathsBySpecies(biology::SpeciesId id) const noexcept;
    [[nodiscard]] float getTotalPredationEnergyGained() const noexcept { return totalPredationEnergyGained_; }
    [[nodiscard]] double getExtinctionTime(biology::SpeciesId id) const noexcept;

    [[nodiscard]] uint32_t getMaxGeneration() const noexcept {
        return maxGeneration_;
    }
    [[nodiscard]] uint32_t getMinGeneration() const noexcept;
    [[nodiscard]] float getMeanGeneration() const noexcept;

    [[nodiscard]] const biology::LineageTracker& getLineageTracker() const noexcept {
        return lineageTracker_;
    }

    [[nodiscard]] float getAverageEnergy() const noexcept;
    [[nodiscard]] float getAverageSpeed() const noexcept;

    // Optional event logger connection for structured ecological event telemetry
    void setEventLogger(data::EventLogger* logger) noexcept { eventLogger_ = logger; }

private:
    core::AppConfig config_;
    std::vector<entities::Organism> organisms_;
    std::vector<entities::Organism> offspringQueue_; // Deferred queue to prevent iterator invalidation
    SpatialGrid orgGrid_;
    biology::LineageTracker lineageTracker_;
    mutable std::vector<float> sensorBuffer_{8, 0.0f}; // Reusable buffer for zero-allocation sensor evaluation
    data::EventLogger* eventLogger_{nullptr};

    entities::EntityId nextEntityId_{1};
    uint64_t totalDeaths_{0};
    uint64_t totalSpawned_{0};
    uint64_t totalBirths_{0};
    uint64_t totalPredations_{0};
    float totalPredationEnergyGained_{0.0f};
    uint64_t deathsStarvation_{0};
    uint64_t deathsPredation_{0};
    uint32_t maxGeneration_{0};

    // Species-level metrics
    std::array<uint64_t, static_cast<size_t>(biology::SpeciesId::Count)> speciesDeaths_{0, 0};
    std::array<uint64_t, static_cast<size_t>(biology::SpeciesId::Count)> speciesBirths_{0, 0};
    std::array<uint64_t, static_cast<size_t>(biology::SpeciesId::Count)> speciesSpawned_{0, 0};
    std::array<uint64_t, static_cast<size_t>(biology::SpeciesId::Count)> speciesStarvationDeaths_{0, 0};
    std::array<uint64_t, static_cast<size_t>(biology::SpeciesId::Count)> speciesPredationDeaths_{0, 0};
    std::array<double, static_cast<size_t>(biology::SpeciesId::Count)> extinctionTimes_{-1.0, -1.0};
};

} // namespace alife::sim
