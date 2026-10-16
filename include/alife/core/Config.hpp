#pragma once

#include <string>
#include <cstdint>

namespace alife::core {

/**
 * @brief Global configuration holding window, simulation, organism, and rendering parameters.
 */
struct AppConfig {
    // Window settings
    std::string windowTitle{"Artificial Life & Evolution Laboratory"};
    unsigned int windowWidth{1440};
    unsigned int windowHeight{900};
    unsigned int targetFrameRate{60};

    // Simulation timing settings
    double fixedTimeStep{1.0 / 60.0};
    double maxAccumulator{0.25};

    // World dimensions
    float worldWidth{1200.0f};
    float worldHeight{800.0f};

    // Visualization settings
    bool showGrid{true};
    float gridCellSize{50.0f};

    // Phase 2: Organism and Population settings
    uint32_t initialPopulation{60};
    float initialEnergy{100.0f};
    float maxEnergy{100.0f};
    float basalMetabolismCost{1.5f}; // energy / second
    float movementMetabolismCost{0.02f}; // energy / (second * (speed / 100))
    float minSpeed{30.0f}; // pixels / second
    float maxSpeed{80.0f}; // pixels / second
    float organismRadius{6.0f};
    uint64_t randomSeed{1337};

    // Phase 3: Food and Resource settings
    uint32_t initialFoodCount{80};
    uint32_t maxFoodCapacity{150};
    float foodEnergyValue{30.0f};
    float foodRadius{4.0f};
    float foodInteractionRadius{10.0f}; // Distance within which an organism can eat food
    float foodRegenerationInterval{0.5f}; // Seconds between regeneration attempts
    uint32_t foodSpawnPerInterval{2}; // Food items spawned per regeneration trigger

    // Phase 4: Reproduction, Inheritance, and Mutation settings
    float reproductionEnergyThreshold{80.0f}; // Energy required to trigger reproduction
    float reproductionEnergyCost{45.0f};      // Energy deducted from parent
    float offspringInitialEnergy{35.0f};      // Energy given to offspring
    float reproductionCooldown{3.0f};         // Seconds before reproducing again
    uint32_t maxPopulation{150};              // Maximum population cap
    float mutationProbability{0.20f};         // Probability per trait mutation
    float mutationMagnitude{0.15f};           // Relative variation (+/- 15%)

    // Phase 5: Natural Selection and Evolutionary Dynamics settings
    std::string experimentId{"Experiment_001_NaturalSelection"};
    float statsSamplingInterval{1.0f};        // Sampling interval in simulation seconds
    size_t maxHistorySnapshots{2000};         // Memory limit for retained snapshots
    bool exportCsvOnExit{true};               // Automatically export telemetry on shutdown
    std::string csvExportPath{"evolution_telemetry.csv"};

    // Phase 6: Neural Networks and Emergent Behaviour settings
    size_t nnInputs{6};
    size_t nnHidden{8};
    size_t nnOutputs{2};
    float nnWeightInitRange{1.0f};
    float nnMutationProbability{0.25f};
    float nnMutationMagnitude{0.20f};
    float nnMaxWeight{4.0f};
    float organismPerceptionRadius{120.0f}; // Sensory perception radius for food

    // Phase 7: Complex Ecosystems and Multi-Species Interactions
    bool enableMultiSpecies{true};
    uint32_t initialHerbivoreCount{45};
    uint32_t initialPredatorCount{15};
    float predatorBodyRadius{7.0f};
    float predatorMinSpeed{40.0f};
    float predatorMaxSpeed{90.0f};
    float predatorBasalMetabolismCost{2.0f};
    float predatorCaptureRadius{12.0f};
    float predatorEnergyGain{50.0f};
    float predatorReproductionThreshold{90.0f};
    float predatorReproductionCost{50.0f};
    float predatorOffspringEnergy{40.0f};
    float predatorPerceptionRadius{150.0f};
    bool allowCannibalism{false};

    // Environmental Seasons
    bool enableSeasons{true};
    float seasonCyclePeriod{30.0f}; // Full season cycle in simulation seconds
    float seasonIntensity{0.5f};    // Regeneration variation amplitude (+/- 50%)

    // Phase 8: Scientific Observation, Data Analysis, and Reproducible Experiments
    std::string runId{"Run_001"};
    std::string dataExportDirectory{"./data_exports"};
    bool enableEventLogging{true};
    size_t maxEventLogSize{10000};
};

} // namespace alife::core
