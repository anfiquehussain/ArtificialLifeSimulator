#include "alife/simulation/World.hpp"

namespace alife::sim {

World::World(const core::AppConfig& config)
    : width_(config.worldWidth > 0.0f ? config.worldWidth : 1200.0f),
      height_(config.worldHeight > 0.0f ? config.worldHeight : 800.0f),
      environment_(config),
      population_(config),
      foodManager_(config) {}

void World::setDimensions(float width, float height) noexcept {
    if (width > 0.0f) width_ = width;
    if (height > 0.0f) height_ = height;
}

void World::update(double fixedDeltaTime, core::Rng& rng, double currentTime) {
    ++updateCount_;
    float dt = static_cast<float>(fixedDeltaTime);

    // 1. Advance environmental simulation time and seasonal oscillation
    environment_.update(currentTime);

    // 2. Food resource regeneration with seasonal modulation
    foodManager_.updateRegeneration(dt, rng, environment_.getFoodRegenMultiplier());

    // 3. Ecological sensory perception, neural network inference, movement, and boundary handling
    population_.update(dt, width_, height_, foodManager_.getFoodItems(), environment_.getSeasonFactor(), currentTime, &foodManager_.getFoodGrid());

    // 4. Resource competition: herbivores forage on reachable food pellets
    foodManager_.resolveConsumption(population_);

    // 5. Predator-prey interactions: predators hunt and capture reachable prey
    population_.resolvePredation();

    // 6. Asexual reproduction across surviving eligible organisms
    population_.updateReproduction(rng, width_, height_, currentTime);
}

void World::reset(core::Rng& rng, double currentTime) {
    updateCount_ = 0;
    environment_.reset();
    foodManager_.reset(rng);
    population_.reset(rng, currentTime);
}

} // namespace alife::sim
