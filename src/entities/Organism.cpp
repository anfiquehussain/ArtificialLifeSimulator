#include "alife/entities/Organism.hpp"
#include <algorithm>
#include <cmath>

namespace alife::entities {

namespace {
constexpr float PI = 3.14159265358979323846f;
}

Organism::Organism(EntityId id,
                   const core::Vector2f& position,
                   const core::Vector2f& direction,
                   const biology::Traits& traits,
                   float energy,
                   float maxEnergy,
                   EntityId parentId,
                   uint32_t generation,
                   ai::NeuralNetwork brain,
                   biology::SpeciesId speciesId)
    : id_(id),
      parentId_(parentId),
      generation_(generation),
      speciesId_(speciesId),
      position_(position),
      direction_(direction.normalized()),
      traits_(traits),
      brain_(std::move(brain)),
      lastSensors_(6, 0.0f),
      energy_(std::max(0.0f, energy)),
      maxEnergy_(maxEnergy > 0.0f ? maxEnergy : 100.0f),
      reproductionCooldownTimer_(0.0f),
      isAlive_(energy_ > 0.0f)
{
    if (direction_.lengthSquared() < 1e-6f) {
        direction_ = {1.0f, 0.0f};
    }
}

void Organism::consumeEnergy(float amount) noexcept {
    if (!isAlive_) return;

    if (amount > 0.0f) {
        energy_ -= amount;
        if (energy_ <= 0.0f) {
            energy_ = 0.0f;
            isAlive_ = false;
            deathCause_ = DeathCause::Starvation;
        }
    }
}

void Organism::gainEnergy(float amount) noexcept {
    if (!isAlive_ || amount <= 0.0f) return;

    energy_ += amount;
    if (energy_ > maxEnergy_) {
        energy_ = maxEnergy_;
    }
}

void Organism::kill(DeathCause cause) noexcept {
    energy_ = 0.0f;
    isAlive_ = false;
    deathCause_ = cause;
}

void Organism::handleBoundaries(float worldWidth, float worldHeight, float radius) noexcept {
    // Reflect at horizontal boundaries
    if (position_.x - radius < 0.0f) {
        position_.x = radius;
        direction_.x = std::abs(direction_.x);
    } else if (position_.x + radius > worldWidth) {
        position_.x = worldWidth - radius;
        direction_.x = -std::abs(direction_.x);
    }

    // Reflect at vertical boundaries
    if (position_.y - radius < 0.0f) {
        position_.y = radius;
        direction_.y = std::abs(direction_.y);
    } else if (position_.y + radius > worldHeight) {
        position_.y = worldHeight - radius;
        direction_.y = -std::abs(direction_.y);
    }

    direction_ = direction_.normalized();
}

void Organism::update(float dt,
                      float movementCostMultiplier,
                      float worldWidth,
                      float worldHeight,
                      float radius,
                      const std::vector<float>& sensorInputs) {
    if (!isAlive_) return;

    // 1. Decrement reproduction cooldown timer
    if (reproductionCooldownTimer_ > 0.0f) {
        reproductionCooldownTimer_ -= dt;
        if (reproductionCooldownTimer_ < 0.0f) {
            reproductionCooldownTimer_ = 0.0f;
        }
    }

    // 2. Neural network decision & action execution
    float effectiveSpeed = traits_.speed;
    if (!sensorInputs.empty()) {
        lastSensors_ = sensorInputs;
        const auto& outputs = brain_.evaluate(sensorInputs);
        if (outputs.size() >= 2) {
            // Output 0: Turn rate steering in [-pi, pi] * dt
            float turnDelta = outputs[0] * PI * dt;
            float currentAngle = std::atan2(direction_.y, direction_.x);
            float newAngle = currentAngle + turnDelta;
            direction_ = {std::cos(newAngle), std::sin(newAngle)};

            // Output 1: Speed throttle multiplier in [0.5, 1.5]
            float speedMod = 1.0f + 0.5f * outputs[1];
            effectiveSpeed = traits_.speed * speedMod;
        }
    }

    // 3. Position update based on velocity (direction * effectiveSpeed) and fixed delta time
    position_ += direction_ * (effectiveSpeed * dt);

    // 4. Boundary handling
    handleBoundaries(worldWidth, worldHeight, radius);

    // 5. Time-based energy consumption:
    // Total energy cost per second = traits_.basalMetabolismCost + (movementCostMultiplier * effectiveSpeed)
    float deltaEnergy = (traits_.basalMetabolismCost + (movementCostMultiplier * effectiveSpeed)) * dt;
    consumeEnergy(deltaEnergy);
}

} // namespace alife::entities
