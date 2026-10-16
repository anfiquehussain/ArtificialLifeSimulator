#include "alife/data/SnapshotManager.hpp"
#include "alife/data/Presets.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <iostream>

namespace alife::data {

SaveResult SnapshotManager::saveSnapshot(const sim::SimulationEngine& engine, const std::string& filepath) {
    if (filepath.empty()) {
        return {false, "Error: Empty snapshot filepath.", filepath};
    }

    std::string tmpPath = filepath + ".tmp";
    std::ofstream out(tmpPath, std::ios::trunc);
    if (!out.is_open()) {
        return {false, "Error: Unable to open temporary save file for writing: " + tmpPath, filepath};
    }

    const auto& config = engine.getConfig();
    const auto& clock = engine.getClock();
    const auto& world = engine.getWorld();
    const auto& popMgr = world.getPopulation();
    const auto& foodMgr = world.getFoodManager();
    const auto& organisms = popMgr.getOrganisms();
    const auto& foods = foodMgr.getFoodItems();

    out << std::setprecision(8);

    // 1. Header
    out << FormatHeader << "\n";

    // 2. Metadata
    out << "[METADATA]\n";
    out << "experimentId " << config.experimentId << "\n";
    out << "runId " << config.runId << "\n";

    // 3. Clock & Simulation State
    out << "[CLOCK]\n";
    out << "simTime " << clock.getSimulationTime() << "\n";
    out << "stepCount " << clock.getStepCount() << "\n";
    out << "speedMultiplier " << clock.getSpeedMultiplier() << "\n";
    out << "isPaused " << (clock.isPaused() ? 1 : 0) << "\n";

    // 4. World State
    out << "[WORLD]\n";
    out << "width " << world.getWidth() << "\n";
    out << "height " << world.getHeight() << "\n";
    out << "updateCount " << world.getUpdateCount() << "\n";

    // 5. Config State
    out << "[CONFIG]\n";
    out << "initialPopulation " << config.initialPopulation << "\n";
    out << "initialHerbivoreCount " << config.initialHerbivoreCount << "\n";
    out << "initialPredatorCount " << config.initialPredatorCount << "\n";
    out << "maxPopulation " << config.maxPopulation << "\n";
    out << "initialFoodCount " << config.initialFoodCount << "\n";
    out << "maxFoodCapacity " << config.maxFoodCapacity << "\n";
    out << "foodEnergyValue " << config.foodEnergyValue << "\n";
    out << "foodRegenerationInterval " << config.foodRegenerationInterval << "\n";
    out << "foodSpawnPerInterval " << config.foodSpawnPerInterval << "\n";
    out << "reproductionEnergyThreshold " << config.reproductionEnergyThreshold << "\n";
    out << "reproductionEnergyCost " << config.reproductionEnergyCost << "\n";
    out << "offspringInitialEnergy " << config.offspringInitialEnergy << "\n";
    out << "reproductionCooldown " << config.reproductionCooldown << "\n";
    out << "mutationProbability " << config.mutationProbability << "\n";
    out << "mutationMagnitude " << config.mutationMagnitude << "\n";
    out << "randomSeed " << config.randomSeed << "\n";
    out << "enableMultiSpecies " << (config.enableMultiSpecies ? 1 : 0) << "\n";
    out << "enableSeasons " << (config.enableSeasons ? 1 : 0) << "\n";

    // 6. Food Items
    out << "[FOODS]\n";
    out << foods.size() << "\n";
    for (const auto& food : foods) {
        out << food.getId() << " "
            << food.getPosition().x << " " << food.getPosition().y << " "
            << food.getEnergyValue() << " "
            << (food.isConsumed() ? 1 : 0) << "\n";
    }

    // 7. Organisms
    out << "[ORGANISMS]\n";
    out << organisms.size() << "\n";
    for (const auto& org : organisms) {
        out << "ORG "
            << org.getId() << " "
            << org.getPosition().x << " " << org.getPosition().y << " "
            << org.getDirection().x << " " << org.getDirection().y << " "
            << org.getEnergy() << " " << org.getMaxEnergy() << " "
            << org.getParentId() << " " << org.getGeneration() << " "
            << org.getReproductionCooldown() << " "
            << static_cast<int>(org.getSpeciesId()) << " "
            << (org.isAlive() ? 1 : 0) << "\n";

        const auto& traits = org.getTraits();
        out << "TRAITS "
            << traits.speed << " "
            << traits.basalMetabolismCost << " "
            << traits.foodInteractionRadius << "\n";

        const auto& brain = org.getBrain();
        out << "BRAIN "
            << brain.getNumInputs() << " "
            << brain.getNumHidden() << " "
            << brain.getNumOutputs() << "\n";

        // Weights Input-Hidden
        out << "WIH";
        for (float w : brain.getWeightsInputHidden()) {
            out << " " << w;
        }
        out << "\n";

        // Biases Hidden
        out << "BH";
        for (float b : brain.getBiasesHidden()) {
            out << " " << b;
        }
        out << "\n";

        // Weights Hidden-Output
        out << "WHO";
        for (float w : brain.getWeightsHiddenOutput()) {
            out << " " << w;
        }
        out << "\n";

        // Biases Output
        out << "BO";
        for (float b : brain.getBiasesOutput()) {
            out << " " << b;
        }
        out << "\n";
    }

    // 8. Footer Checksum Tag
    out << FormatFooter << "\n";
    out.flush();

    if (!out.good()) {
        out.close();
        std::error_code ec;
        std::filesystem::remove(tmpPath, ec);
        return {false, "Error: File stream error while writing snapshot data.", filepath};
    }
    out.close();

    // Atomic replacement: remove target if exists and rename temp file
    std::error_code ec;
    if (std::filesystem::exists(filepath, ec)) {
        std::filesystem::remove(filepath, ec);
    }
    std::filesystem::rename(tmpPath, filepath, ec);
    if (ec) {
        std::filesystem::remove(tmpPath, ec);
        return {false, "Error: Atomic rename failed: " + ec.message(), filepath};
    }

    return {true, "Snapshot saved successfully to: " + filepath, filepath};
}

LoadResult SnapshotManager::loadSnapshot(sim::SimulationEngine& engine, const std::string& filepath) {
    if (filepath.empty()) {
        return {false, "Error: Snapshot filepath is empty.", 0, 0, 0.0, 0};
    }

    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        return {false, "Error: File not found: " + filepath, 0, 0, 0.0, 0};
    }

    std::ifstream in(filepath);
    if (!in.is_open()) {
        return {false, "Error: Unable to open file for reading: " + filepath, 0, 0, 0.0, 0};
    }

    std::string header;
    if (!(in >> header) || header != FormatHeader) {
        return {false, "Error: Invalid or incompatible snapshot format header.", 0, 0, 0.0, 0};
    }

    // Staging data structures (leaves live simulation completely untouched until 100% validated)
    core::AppConfig stagedConfig = engine.getConfig();
    double stagedSimTime = 0.0;
    uint64_t stagedStepCount = 0;
    double stagedSpeedMultiplier = 1.0;
    bool stagedPaused = true;
    float stagedWidth = 1200.0f;
    float stagedHeight = 800.0f;
    uint64_t stagedUpdateCount = 0;

    std::vector<entities::Food> stagedFoods;
    std::vector<entities::Organism> stagedOrganisms;
    entities::EntityId maxOrgId = 0;
    entities::FoodId maxFoodId = 0;

    std::string section;
    bool reachedEnd = false;

    while (in >> section) {
        if (section == "[METADATA]") {
            std::string key, val;
            while (in >> key && key != "[CLOCK]" && key != "[WORLD]" && key != "[CONFIG]" && key != "[FOODS]" && key != "[ORGANISMS]" && key != FormatFooter) {
                if (key == "experimentId") in >> stagedConfig.experimentId;
                else if (key == "runId") in >> stagedConfig.runId;
                else in >> val;
            }
            if (key.starts_with("[")) section = key;
        }

        if (section == "[CLOCK]") {
            std::string key;
            while (in >> key && key != "[WORLD]" && key != "[CONFIG]" && key != "[FOODS]" && key != "[ORGANISMS]" && key != FormatFooter) {
                if (key == "simTime") in >> stagedSimTime;
                else if (key == "stepCount") in >> stagedStepCount;
                else if (key == "speedMultiplier") in >> stagedSpeedMultiplier;
                else if (key == "isPaused") { int p; in >> p; stagedPaused = (p != 0); }
                else { std::string skip; in >> skip; }
            }
            if (key.starts_with("[")) section = key;
        }

        if (section == "[WORLD]") {
            std::string key;
            while (in >> key && key != "[CONFIG]" && key != "[FOODS]" && key != "[ORGANISMS]" && key != FormatFooter) {
                if (key == "width") in >> stagedWidth;
                else if (key == "height") in >> stagedHeight;
                else if (key == "updateCount") in >> stagedUpdateCount;
                else { std::string skip; in >> skip; }
            }
            if (key.starts_with("[")) section = key;
        }

        if (section == "[CONFIG]") {
            std::string key;
            while (in >> key && key != "[FOODS]" && key != "[ORGANISMS]" && key != FormatFooter) {
                if (key == "initialPopulation") in >> stagedConfig.initialPopulation;
                else if (key == "initialHerbivoreCount") in >> stagedConfig.initialHerbivoreCount;
                else if (key == "initialPredatorCount") in >> stagedConfig.initialPredatorCount;
                else if (key == "maxPopulation") in >> stagedConfig.maxPopulation;
                else if (key == "initialFoodCount") in >> stagedConfig.initialFoodCount;
                else if (key == "maxFoodCapacity") in >> stagedConfig.maxFoodCapacity;
                else if (key == "foodEnergyValue") in >> stagedConfig.foodEnergyValue;
                else if (key == "foodRegenerationInterval") in >> stagedConfig.foodRegenerationInterval;
                else if (key == "foodSpawnPerInterval") in >> stagedConfig.foodSpawnPerInterval;
                else if (key == "reproductionEnergyThreshold") in >> stagedConfig.reproductionEnergyThreshold;
                else if (key == "reproductionEnergyCost") in >> stagedConfig.reproductionEnergyCost;
                else if (key == "offspringInitialEnergy") in >> stagedConfig.offspringInitialEnergy;
                else if (key == "reproductionCooldown") in >> stagedConfig.reproductionCooldown;
                else if (key == "mutationProbability") in >> stagedConfig.mutationProbability;
                else if (key == "mutationMagnitude") in >> stagedConfig.mutationMagnitude;
                else if (key == "randomSeed") in >> stagedConfig.randomSeed;
                else if (key == "enableMultiSpecies") { int v; in >> v; stagedConfig.enableMultiSpecies = (v != 0); }
                else if (key == "enableSeasons") { int v; in >> v; stagedConfig.enableSeasons = (v != 0); }
                else { std::string skip; in >> skip; }
            }
            if (key.starts_with("[")) section = key;
        }

        if (section == "[FOODS]") {
            size_t foodCount = 0;
            if (!(in >> foodCount)) {
                return {false, "Error: Corrupted food count in snapshot.", 0, 0, 0.0, 0};
            }
            stagedFoods.reserve(foodCount);
            for (size_t i = 0; i < foodCount; ++i) {
                entities::FoodId id = 0;
                float x = 0.0f, y = 0.0f, energy = 0.0f;
                int isConsumedInt = 0;
                if (!(in >> id >> x >> y >> energy >> isConsumedInt)) {
                    return {false, "Error: Malformed food record in snapshot.", 0, 0, 0.0, 0};
                }
                entities::Food food(id, {x, y}, energy);
                if (isConsumedInt != 0) {
                    food.consume();
                }
                if (id > maxFoodId) maxFoodId = id;
                stagedFoods.push_back(std::move(food));
            }
        }

        if (section == "[ORGANISMS]") {
            size_t orgCount = 0;
            if (!(in >> orgCount)) {
                return {false, "Error: Corrupted organism count in snapshot.", 0, 0, 0.0, 0};
            }
            stagedOrganisms.reserve(orgCount);
            for (size_t i = 0; i < orgCount; ++i) {
                std::string tag;
                if (!(in >> tag) || tag != "ORG") {
                    return {false, "Error: Missing ORG tag in organism record.", 0, 0, 0.0, 0};
                }

                entities::EntityId id = 0, parentId = 0;
                float x = 0.0f, y = 0.0f, dirX = 1.0f, dirY = 0.0f, energy = 0.0f, maxE = 100.0f, cooldown = 0.0f;
                uint32_t generation = 0;
                int speciesIdInt = 0, isAliveInt = 1;

                if (!(in >> id >> x >> y >> dirX >> dirY >> energy >> maxE >> parentId >> generation >> cooldown >> speciesIdInt >> isAliveInt)) {
                    return {false, "Error: Malformed organism fields in snapshot.", 0, 0, 0.0, 0};
                }

                // Read TRAITS
                std::string traitsTag;
                float speed = 50.0f, basalCost = 1.5f, radius = 10.0f;
                if (!(in >> traitsTag) || traitsTag != "TRAITS" || !(in >> speed >> basalCost >> radius)) {
                    return {false, "Error: Malformed organism traits in snapshot.", 0, 0, 0.0, 0};
                }
                biology::Traits traits{speed, basalCost, radius};

                // Read BRAIN
                std::string brainTag;
                size_t nIn = 6, nHid = 8, nOut = 2;
                if (!(in >> brainTag) || brainTag != "BRAIN" || !(in >> nIn >> nHid >> nOut)) {
                    return {false, "Error: Malformed neural network dimensions in snapshot.", 0, 0, 0.0, 0};
                }

                ai::NeuralNetwork brain(nIn, nHid, nOut);

                std::string wihTag;
                if (!(in >> wihTag) || wihTag != "WIH") return {false, "Error: Missing WIH tag.", 0, 0, 0.0, 0};
                std::vector<float> wIH(nIn * nHid, 0.0f);
                for (auto& w : wIH) if (!(in >> w)) return {false, "Error: Truncated WIH weights.", 0, 0, 0.0, 0};

                std::string bhTag;
                if (!(in >> bhTag) || bhTag != "BH") return {false, "Error: Missing BH tag.", 0, 0, 0.0, 0};
                std::vector<float> bH(nHid, 0.0f);
                for (auto& b : bH) if (!(in >> b)) return {false, "Error: Truncated BH biases.", 0, 0, 0.0, 0};

                std::string whoTag;
                if (!(in >> whoTag) || whoTag != "WHO") return {false, "Error: Missing WHO tag.", 0, 0, 0.0, 0};
                std::vector<float> wHO(nHid * nOut, 0.0f);
                for (auto& w : wHO) if (!(in >> w)) return {false, "Error: Truncated WHO weights.", 0, 0, 0.0, 0};

                std::string boTag;
                if (!(in >> boTag) || boTag != "BO") return {false, "Error: Missing BO tag.", 0, 0, 0.0, 0};
                std::vector<float> bO(nOut, 0.0f);
                for (auto& b : bO) if (!(in >> b)) return {false, "Error: Truncated BO biases.", 0, 0, 0.0, 0};

                brain.setParameters(std::move(wIH), std::move(bH), std::move(wHO), std::move(bO));

                biology::SpeciesId sp = (speciesIdInt == 1) ? biology::SpeciesId::Predator : biology::SpeciesId::Herbivore;
                entities::Organism org(id, {x, y}, {dirX, dirY}, traits, energy, maxE, parentId, generation, std::move(brain), sp);
                org.resetReproductionCooldown(cooldown);
                if (isAliveInt == 0) {
                    org.kill();
                }

                if (id > maxOrgId) maxOrgId = id;
                stagedOrganisms.push_back(std::move(org));
            }
        }

        if (section == FormatFooter) {
            reachedEnd = true;
            break;
        }
    }

    if (!reachedEnd) {
        return {false, "Error: Truncated snapshot file missing valid footer tag.", 0, 0, 0.0, 0};
    }

    // 2. Comprehensive Validation Check
    auto val = validateConfig(stagedConfig);
    if (!val.valid) {
        return {false, "Error: Snapshot configuration invalid: " + val.errorMessage, 0, 0, 0.0, 0};
    }
    if (stagedWidth < 200.0f || stagedHeight < 200.0f) {
        return {false, "Error: Invalid world dimensions in snapshot.", 0, 0, 0.0, 0};
    }
    if (stagedSimTime < 0.0) {
        return {false, "Error: Negative simulation time in snapshot.", 0, 0, 0.0, 0};
    }

    // 3. Atomically apply verified state to live simulation
    engine.applyConfig(stagedConfig, false);
    auto& world = engine.getWorld();
    world.setDimensions(stagedWidth, stagedHeight);
    world.restoreState(stagedUpdateCount);

    world.getFoodManager().restoreState(std::move(stagedFoods), maxFoodId + 1);
    world.getPopulation().restoreState(std::move(stagedOrganisms), maxOrgId + 1);
    world.getEnvironment().update(stagedSimTime);

    engine.restoreState(stagedSimTime, stagedStepCount, stagedPaused);
    engine.setSpeedMultiplier(stagedSpeedMultiplier);

    return {
        true,
        "Snapshot successfully restored (" + std::to_string(world.getPopulation().getActiveCount()) + " organisms, " +
        std::to_string(world.getFoodManager().getFoodCount()) + " food items).",
        static_cast<uint32_t>(world.getPopulation().getActiveCount()),
        static_cast<uint32_t>(world.getFoodManager().getFoodCount()),
        stagedSimTime,
        stagedStepCount
    };
}

} // namespace alife::data
