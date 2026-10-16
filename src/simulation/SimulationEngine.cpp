#include "alife/simulation/SimulationEngine.hpp"

namespace alife::sim {

SimulationEngine::SimulationEngine(const core::AppConfig& config)
    : config_(config),
      rng_(config.randomSeed),
      world_(config),
      clock_(config.fixedTimeStep, config.maxAccumulator),
      historyRecorder_(config.maxHistorySnapshots, config.maxEventLogSize),
      lastStatsSampleTime_(0.0)
{
    if (config_.enableEventLogging) {
        world_.getPopulation().setEventLogger(&historyRecorder_.getEventLogger());
    }

    // Initialize world and population with deterministic seed
    world_.reset(rng_, 0.0);
    // Take initial t=0 snapshot
    sampleEvolutionaryMetrics();
}

void SimulationEngine::sampleEvolutionaryMetrics() {
    double simTime = clock_.getSimulationTime();
    uint64_t step = clock_.getStepCount();
    auto snapshot = world_.getPopulation().computeEvolutionSnapshot(simTime, step);

    // Enrich with ecosystem and environment metrics
    snapshot.foodCount = world_.getFoodManager().getFoodCount();
    snapshot.maxFoodCapacity = config_.maxFoodCapacity;
    snapshot.totalFoodConsumed = world_.getFoodManager().getTotalFoodConsumed();
    snapshot.totalFoodEnergyGained = world_.getFoodManager().getTotalFoodEnergyGained();
    snapshot.seasonFactor = world_.getEnvironment().getSeasonFactor();
    snapshot.foodRegenMultiplier = world_.getEnvironment().getFoodRegenMultiplier();
    snapshot.seasonName = world_.getEnvironment().getSeasonName();

    historyRecorder_.recordSnapshot(std::move(snapshot));
    lastStatsSampleTime_ = simTime;
}

void SimulationEngine::update(core::SimulationClock::Duration wallDeltaTime) {
    clock_.update(wallDeltaTime);

    while (clock_.checkStep()) {
        double currentSimTime = clock_.getSimulationTime();
        world_.update(clock_.getFixedTimeStep(), rng_, currentSimTime);

        // Check periodic sampling interval
        if (currentSimTime - lastStatsSampleTime_ >= config_.statsSamplingInterval) {
            sampleEvolutionaryMetrics();
        }
    }
}

void SimulationEngine::stepOnce() {
    if (!clock_.isPaused()) {
        clock_.pause();
    }
    clock_.stepOnce();
    double currentSimTime = clock_.getSimulationTime();
    world_.update(clock_.getFixedTimeStep(), rng_, currentSimTime);
    if (currentSimTime - lastStatsSampleTime_ >= config_.statsSamplingInterval) {
        sampleEvolutionaryMetrics();
    }
}

void SimulationEngine::applyConfig(const core::AppConfig& config, bool resetWorld) {
    config_ = config;
    if (resetWorld) {
        reset();
    }
}

void SimulationEngine::reset() noexcept {
    clock_.reset();
    rng_.setSeed(config_.randomSeed);
    world_.reset(rng_, 0.0);
    historyRecorder_.reset();
    if (config_.enableEventLogging) {
        world_.getPopulation().setEventLogger(&historyRecorder_.getEventLogger());
    }
    lastStatsSampleTime_ = 0.0;
    sampleEvolutionaryMetrics();
}

data::ExperimentMetadata SimulationEngine::getExperimentMetadata() const {
    data::ExperimentMetadata meta;
    meta.experimentId = config_.experimentId;
    meta.runId = config_.runId;
    meta.appVersion = "0.1.0-phase8";
    meta.randomSeed = config_.randomSeed;
    meta.simulationDuration = clock_.getSimulationTime();
    meta.totalSteps = clock_.getStepCount();
    meta.terminationReason = clock_.isPaused() ? "Paused" : "Running";

    meta.initialPopulation = config_.initialPopulation;
    meta.initialHerbivores = config_.initialHerbivoreCount;
    meta.initialPredators = config_.initialPredatorCount;
    meta.maxPopulation = config_.maxPopulation;
    meta.initialFoodCount = config_.initialFoodCount;
    meta.maxFoodCapacity = config_.maxFoodCapacity;
    meta.foodEnergyValue = config_.foodEnergyValue;
    meta.mutationProbability = config_.mutationProbability;
    meta.mutationMagnitude = config_.mutationMagnitude;
    meta.statsSamplingInterval = config_.statsSamplingInterval;
    meta.enableMultiSpecies = config_.enableMultiSpecies;
    meta.enableSeasons = config_.enableSeasons;

    meta.finalPopulation = world_.getPopulation().getActiveCount();
    meta.finalHerbivores = world_.getPopulation().getHerbivoreCount();
    meta.finalPredators = world_.getPopulation().getPredatorCount();
    meta.totalBirths = world_.getPopulation().getTotalBirths();
    meta.totalDeaths = world_.getPopulation().getTotalDeaths();
    meta.totalPredations = world_.getPopulation().getTotalPredations();
    meta.totalFoodConsumed = world_.getFoodManager().getTotalFoodConsumed();
    meta.starvationDeaths = world_.getPopulation().getDeathsByStarvation();
    meta.predationDeaths = world_.getPopulation().getDeathsByPredation();
    meta.herbivoreExtinct = world_.getPopulation().isExtinct(biology::SpeciesId::Herbivore);
    meta.predatorExtinct = world_.getPopulation().isExtinct(biology::SpeciesId::Predator);
    meta.herbivoreExtinctionTime = world_.getPopulation().getExtinctionTime(biology::SpeciesId::Herbivore);
    meta.predatorExtinctionTime = world_.getPopulation().getExtinctionTime(biology::SpeciesId::Predator);

    return meta;
}

} // namespace alife::sim
