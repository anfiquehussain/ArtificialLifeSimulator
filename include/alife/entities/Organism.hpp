#pragma once

#include "alife/ai/NeuralNetwork.hpp"
#include "alife/biology/Species.hpp"
#include "alife/biology/Traits.hpp"
#include "alife/core/Vector2.hpp"
#include <cstdint>
#include <vector>

namespace alife::entities {

using EntityId = uint64_t;

enum class DeathCause : uint8_t {
    Unknown = 0,
    Starvation = 1,
    Predation = 2
};

[[nodiscard]] constexpr const char* deathCauseToString(DeathCause cause) noexcept {
    switch (cause) {
        case DeathCause::Starvation: return "Starvation";
        case DeathCause::Predation: return "Predation";
        default: return "Unknown";
    }
}

/**
 * @brief Represents an individual living organism entity with species role,
 * heritable traits, neural-network controller, lineage tracking, and reproduction cooldown.
 */
class Organism {
public:
    Organism(EntityId id,
             const core::Vector2f& position,
             const core::Vector2f& direction,
             const biology::Traits& traits,
             float energy,
             float maxEnergy,
             EntityId parentId = 0,
             uint32_t generation = 0,
             ai::NeuralNetwork brain = ai::NeuralNetwork{},
             biology::SpeciesId speciesId = biology::SpeciesId::Herbivore);

    // Identity & Lineage
    [[nodiscard]] EntityId getId() const noexcept { return id_; }
    [[nodiscard]] EntityId getParentId() const noexcept { return parentId_; }
    [[nodiscard]] uint32_t getGeneration() const noexcept { return generation_; }
    [[nodiscard]] biology::SpeciesId getSpeciesId() const noexcept { return speciesId_; }
    [[nodiscard]] bool isHerbivore() const noexcept { return speciesId_ == biology::SpeciesId::Herbivore; }
    [[nodiscard]] bool isPredator() const noexcept { return speciesId_ == biology::SpeciesId::Predator; }
    [[nodiscard]] DeathCause getDeathCause() const noexcept { return deathCause_; }

    // Physical state
    [[nodiscard]] const core::Vector2f& getPosition() const noexcept { return position_; }
    [[nodiscard]] const core::Vector2f& getDirection() const noexcept { return direction_; }
    [[nodiscard]] float getEnergy() const noexcept { return energy_; }
    [[nodiscard]] float getMaxEnergy() const noexcept { return maxEnergy_; }
    [[nodiscard]] bool isAlive() const noexcept { return isAlive_; }

    // Heritable traits & brain
    [[nodiscard]] const biology::Traits& getTraits() const noexcept { return traits_; }
    [[nodiscard]] biology::Traits& getTraits() noexcept { return traits_; }
    [[nodiscard]] float getSpeed() const noexcept { return traits_.speed; }

    [[nodiscard]] const ai::NeuralNetwork& getBrain() const noexcept { return brain_; }
    [[nodiscard]] ai::NeuralNetwork& getBrain() noexcept { return brain_; }
    void setBrain(ai::NeuralNetwork brain) noexcept { brain_ = std::move(brain); }
    void setSpeciesId(biology::SpeciesId s) noexcept { speciesId_ = s; }

    [[nodiscard]] const std::vector<float>& getLastSensors() const noexcept { return lastSensors_; }
    [[nodiscard]] const std::vector<float>& getLastOutputs() const noexcept { return brain_.getLastOutputs(); }

    // Reproduction state
    [[nodiscard]] float getReproductionCooldown() const noexcept { return reproductionCooldownTimer_; }
    [[nodiscard]] bool canReproduce(float threshold) const noexcept {
        return isAlive_ && energy_ >= threshold && reproductionCooldownTimer_ <= 0.0f;
    }

    void setPosition(const core::Vector2f& pos) noexcept { position_ = pos; }
    void setDirection(const core::Vector2f& dir) noexcept { direction_ = dir.normalized(); }
    void resetReproductionCooldown(float cooldown) noexcept { reproductionCooldownTimer_ = cooldown; }

    // Stepping: movement, boundaries, metabolic energy consumption, cooldown decrement
    // Accepts optional sensor inputs. If provided, neural-network outputs govern steering and throttle.
    void update(float dt,
                float movementCostMultiplier,
                float worldWidth,
                float worldHeight,
                float radius,
                const std::vector<float>& sensorInputs = {});

    // Energy methods
    void consumeEnergy(float amount) noexcept;
    void gainEnergy(float amount) noexcept;
    void kill(DeathCause cause = DeathCause::Predation) noexcept;

private:
    void handleBoundaries(float worldWidth, float worldHeight, float radius) noexcept;

    EntityId id_{0};
    EntityId parentId_{0};
    uint32_t generation_{0};
    biology::SpeciesId speciesId_{biology::SpeciesId::Herbivore};
    DeathCause deathCause_{DeathCause::Unknown};

    core::Vector2f position_{0.0f, 0.0f};
    core::Vector2f direction_{1.0f, 0.0f};

    biology::Traits traits_;
    ai::NeuralNetwork brain_;

    std::vector<float> lastSensors_;

    float energy_{100.0f};
    float maxEnergy_{100.0f};
    float reproductionCooldownTimer_{0.0f};
    bool isAlive_{true};
};

} // namespace alife::entities
