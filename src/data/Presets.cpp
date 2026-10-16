#include "alife/data/Presets.hpp"
#include <algorithm>

namespace alife::data {

ValidationResult validateConfig(const core::AppConfig& config) {
    if (config.worldWidth < 200.0f || config.worldHeight < 200.0f) {
        return {false, "World dimensions must be at least 200x200 pixels."};
    }
    if (config.fixedTimeStep <= 0.0 || config.fixedTimeStep > 0.1) {
        return {false, "Fixed timestep must be in range (0.0, 0.1] seconds."};
    }
    if (config.initialPopulation == 0) {
        return {false, "Initial population must be at least 1."};
    }
    if (config.maxPopulation < config.initialPopulation) {
        return {false, "Maximum population cannot be less than initial population."};
    }
    if (config.initialFoodCount > config.maxFoodCapacity) {
        return {false, "Initial food count cannot exceed maximum food capacity."};
    }
    if (config.minSpeed <= 0.0f || config.maxSpeed < config.minSpeed) {
        return {false, "Max speed must be greater than or equal to min speed."};
    }
    if (config.reproductionEnergyThreshold <= config.reproductionEnergyCost) {
        return {false, "Reproduction energy threshold must exceed reproduction energy cost."};
    }
    if (config.mutationProbability < 0.0f || config.mutationProbability > 1.0f) {
        return {false, "Mutation probability must be in range [0.0, 1.0]."};
    }
    if (config.mutationMagnitude < 0.0f) {
        return {false, "Mutation magnitude cannot be negative."};
    }
    if (config.enableMultiSpecies) {
        if (config.initialHerbivoreCount + config.initialPredatorCount != config.initialPopulation) {
            return {false, "Sum of herbivores and predators must equal initial population."};
        }
        if (config.predatorReproductionThreshold <= config.predatorReproductionCost) {
            return {false, "Predator reproduction threshold must exceed reproduction cost."};
        }
    }
    return {true, ""};
}

std::vector<ExperimentPreset> getAvailablePresets() {
    std::vector<ExperimentPreset> presets;

    // 1. Basic Evolution (Standard baseline)
    {
        core::AppConfig cfg;
        cfg.experimentId = "Preset_BasicEvolution";
        cfg.initialPopulation = 60;
        cfg.initialHerbivoreCount = 45;
        cfg.initialPredatorCount = 15;
        cfg.maxPopulation = 150;
        cfg.initialFoodCount = 80;
        cfg.maxFoodCapacity = 150;
        cfg.foodEnergyValue = 30.0f;
        cfg.foodRegenerationInterval = 0.5f;
        cfg.mutationProbability = 0.20f;
        cfg.mutationMagnitude = 0.15f;
        cfg.enableMultiSpecies = true;
        cfg.enableSeasons = true;

        presets.push_back({
            "basic_evolution",
            "Basic Evolution",
            "Standard baseline multi-species ecosystem with moderate mutations and seasonal cycles.",
            cfg
        });
    }

    // 2. Resource Scarcity (Starvation pressure)
    {
        core::AppConfig cfg;
        cfg.experimentId = "Preset_ResourceScarcity";
        cfg.initialPopulation = 50;
        cfg.initialHerbivoreCount = 40;
        cfg.initialPredatorCount = 10;
        cfg.maxPopulation = 100;
        cfg.initialFoodCount = 30;
        cfg.maxFoodCapacity = 60;
        cfg.foodEnergyValue = 25.0f;
        cfg.foodRegenerationInterval = 1.5f; // Slower regeneration
        cfg.foodSpawnPerInterval = 1;
        cfg.basalMetabolismCost = 2.2f;       // Harsh metabolism drain
        cfg.enableMultiSpecies = true;
        cfg.enableSeasons = true;
        cfg.seasonIntensity = 0.70f;          // Severe seasonal drought

        presets.push_back({
            "resource_scarcity",
            "Resource Scarcity",
            "Harsh environment with low food, high metabolism, and severe seasonal droughts testing foraging efficiency.",
            cfg
        });
    }

    // 3. Rapid Reproduction (Boom-and-bust dynamics)
    {
        core::AppConfig cfg;
        cfg.experimentId = "Preset_RapidReproduction";
        cfg.initialPopulation = 40;
        cfg.initialHerbivoreCount = 35;
        cfg.initialPredatorCount = 5;
        cfg.maxPopulation = 300;
        cfg.reproductionEnergyThreshold = 55.0f; // Low threshold
        cfg.reproductionEnergyCost = 25.0f;
        cfg.offspringInitialEnergy = 25.0f;
        cfg.reproductionCooldown = 1.0f;         // Fast cooldown
        cfg.initialFoodCount = 150;
        cfg.maxFoodCapacity = 300;
        cfg.foodRegenerationInterval = 0.2f;     // Abundant regeneration
        cfg.foodSpawnPerInterval = 4;
        cfg.enableMultiSpecies = true;

        presets.push_back({
            "rapid_reproduction",
            "Rapid Reproduction",
            "Abundant resources and low reproductive barriers causing high population turnover and carrying-capacity clashes.",
            cfg
        });
    }

    // 4. High Mutation (Fast genetic drift)
    {
        core::AppConfig cfg;
        cfg.experimentId = "Preset_HighMutation";
        cfg.initialPopulation = 60;
        cfg.initialHerbivoreCount = 45;
        cfg.initialPredatorCount = 15;
        cfg.maxPopulation = 180;
        cfg.mutationProbability = 0.65f;     // Very high trait mutation
        cfg.mutationMagnitude = 0.40f;       // Wide trait variance
        cfg.nnMutationProbability = 0.50f;   // High brain mutation
        cfg.nnMutationMagnitude = 0.35f;
        cfg.enableMultiSpecies = true;

        presets.push_back({
            "high_mutation",
            "High Mutation",
            "High genetic and neural mutation rates to observe rapid trait diversification and evolutionary exploration.",
            cfg
        });
    }

    // 5. Predator-Prey Ecosystem (Trophic oscillation)
    {
        core::AppConfig cfg;
        cfg.experimentId = "Preset_PredatorPrey";
        cfg.initialPopulation = 80;
        cfg.initialHerbivoreCount = 60;
        cfg.initialPredatorCount = 20;
        cfg.maxPopulation = 250;
        cfg.predatorCaptureRadius = 14.0f;
        cfg.predatorEnergyGain = 60.0f;
        cfg.predatorReproductionThreshold = 85.0f;
        cfg.predatorReproductionCost = 40.0f;
        cfg.initialFoodCount = 120;
        cfg.maxFoodCapacity = 200;
        cfg.enableMultiSpecies = true;
        cfg.enableSeasons = true;
        cfg.seasonCyclePeriod = 25.0f;

        presets.push_back({
            "predator_prey",
            "Predator-Prey Ecosystem",
            "Balanced multi-species predator-prey dynamics observing cyclical Lotka-Volterra trophic population waves.",
            cfg
        });
    }

    // 6. Neural-Network Behaviour (Active cognition)
    {
        core::AppConfig cfg;
        cfg.experimentId = "Preset_NeuralBehaviour";
        cfg.initialPopulation = 50;
        cfg.initialHerbivoreCount = 38;
        cfg.initialPredatorCount = 12;
        cfg.maxPopulation = 150;
        cfg.organismPerceptionRadius = 160.0f; // Broad perception
        cfg.predatorPerceptionRadius = 180.0f;
        cfg.nnInputs = 6;
        cfg.nnHidden = 8;
        cfg.nnOutputs = 2;
        cfg.nnWeightInitRange = 1.5f;
        cfg.nnMutationProbability = 0.30f;
        cfg.nnMutationMagnitude = 0.25f;
        cfg.enableMultiSpecies = true;

        presets.push_back({
            "neural_behaviour",
            "Neural-Network Behaviour",
            "Focuses on emergent sensorimotor steering, predator evasion, and active food navigation.",
            cfg
        });
    }

    // 7. Custom Experiment (Interactive sandbox)
    {
        core::AppConfig cfg;
        cfg.experimentId = "Experiment_Custom";
        cfg.runId = "Run_001";

        presets.push_back({
            "custom_experiment",
            "Custom Experiment",
            "User-configured experiment with fully adjustable simulation, environmental, and genetic parameters.",
            cfg
        });
    }

    return presets;
}

std::optional<ExperimentPreset> findPreset(const std::string& nameOrId) {
    auto all = getAvailablePresets();
    for (const auto& p : all) {
        if (p.id == nameOrId || p.name == nameOrId) {
            return p;
        }
    }
    return std::nullopt;
}

} // namespace alife::data
