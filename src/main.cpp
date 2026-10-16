#include "alife/core/Config.hpp"
#include "alife/rendering/Renderer.hpp"
#include "alife/simulation/SimulationEngine.hpp"
#include "alife/data/Presets.hpp"
#include "alife/data/SnapshotManager.hpp"
#include <chrono>
#include <iostream>
#include <vector>

int main() {
    using namespace alife;

    std::cout << "Starting Artificial Life & Evolution Laboratory (Interactive Ecosystem UI)...\n";

    core::AppConfig config;
    sim::SimulationEngine engine(config);
    render::Renderer renderer(config);

    auto presets = data::getAvailablePresets();
    size_t currentPresetIdx = 0;
    if (!presets.empty()) {
        renderer.setCurrentPresetName(presets[currentPresetIdx].name);
    }

    auto lastTime = std::chrono::steady_clock::now();

    renderer.showNotification("Artificial Life Laboratory ready. [Space] to run/pause, [1-5] for speed, [P] presets.",
                             render::UITheme::HerbivoreGreen, 4.0f);

    auto executeAction = [&](const std::string& action) {
        if (action.empty()) return;

        if (action == "sim:pause_toggle") {
            engine.togglePause();
            renderer.showNotification(engine.isPaused() ? "Simulation: PAUSED" : "Simulation: RUNNING",
                                     engine.isPaused() ? render::UITheme::StatusPaused : render::UITheme::StatusRunning, 1.5f);
        } else if (action == "sim:step_once") {
            engine.stepOnce();
            renderer.showNotification("Single-step: advanced 1 physics tick (0.0167s)", render::UITheme::StatusPaused, 1.5f);
        } else if (action == "sim:reset") {
            engine.reset();
            renderer.clearSelection();
            renderer.showNotification("Simulation, ecosystem, and environment reset.", render::UITheme::FoodAmber, 2.5f);
        } else if (action.rfind("speed:", 0) == 0) {
            try {
                double speed = std::stod(action.substr(6));
                engine.setSpeedMultiplier(speed);
                renderer.showNotification("Simulation Speed: " + action.substr(6) + "x", render::UITheme::AccentBlue, 2.0f);
            } catch (...) {}
        } else if (action.rfind("preset:", 0) == 0) {
            std::string presetId = action.substr(7);
            if (presetId == "basic") presetId = "basic_evolution";
            else if (presetId == "scarcity") presetId = "resource_scarcity";
            else if (presetId == "repro") presetId = "rapid_reproduction";
            else if (presetId == "mutation") presetId = "high_mutation";
            else if (presetId == "predator") presetId = "predator_prey";
            else if (presetId == "neural") presetId = "neural_behaviour";
            else if (presetId == "custom") presetId = "custom_experiment";

            auto foundPreset = data::findPreset(presetId);
            if (foundPreset) {
                engine.applyConfig(foundPreset->config, /*resetWorld=*/true);
                renderer.setCurrentPresetName(foundPreset->name);
                renderer.clearSelection();
                renderer.showNotification("Applied Preset: " + foundPreset->name, render::UITheme::AccentBlue, 3.5f);
            }
        } else if (action == "action:save") {
            auto res = data::SnapshotManager::saveSnapshot(engine, "quicksave.alife");
            renderer.showNotification(res.success ? "Quick Save: Successfully wrote quicksave.alife" : ("Save Failed: " + res.message),
                                     res.success ? render::UITheme::HerbivoreGreen : render::UITheme::PredatorRed, 3.0f);
        } else if (action == "action:load") {
            auto res = data::SnapshotManager::loadSnapshot(engine, "quicksave.alife");
            renderer.showNotification(res.success ? ("Quick Load: Restored " + std::to_string(res.organismsLoaded) + " organisms from quicksave.alife")
                                                  : ("Load Failed: " + res.message),
                                     res.success ? render::UITheme::HerbivoreGreen : render::UITheme::PredatorRed, 3.5f);
        } else if (action == "action:export") {
            if (engine.exportAllScientificData(config.dataExportDirectory)) {
                renderer.showNotification("Exported scientific CSV suite to: " + config.dataExportDirectory, render::UITheme::HerbivoreGreen, 3.5f);
                std::cout << "Exported comprehensive scientific observation suite to: " << config.dataExportDirectory << "\n";
            } else {
                renderer.showNotification("Failed to export scientific observation suite.", render::UITheme::PredatorRed, 3.5f);
            }
        }
    };

    // Main application loop
    while (renderer.isOpen()) {
        // 1. Process OS & input events
        while (const auto event = renderer.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                renderer.close();
            } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                renderer.handleResize(resized->size.x, resized->size.y);
            } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                switch (keyPressed->code) {
                    case sf::Keyboard::Key::Escape:
                        renderer.close();
                        break;
                    case sf::Keyboard::Key::Space:
                        executeAction("sim:pause_toggle");
                        break;
                    case sf::Keyboard::Key::S:
                        executeAction("sim:step_once");
                        break;
                    case sf::Keyboard::Key::Num1:
                        executeAction("speed:0.5");
                        break;
                    case sf::Keyboard::Key::Num2:
                        executeAction("speed:1.0");
                        break;
                    case sf::Keyboard::Key::Num3:
                        executeAction("speed:2.0");
                        break;
                    case sf::Keyboard::Key::Num4:
                        executeAction("speed:5.0");
                        break;
                    case sf::Keyboard::Key::Num5:
                        executeAction("speed:10.0");
                        break;
                    case sf::Keyboard::Key::P:
                        if (!presets.empty()) {
                            currentPresetIdx = (currentPresetIdx + 1) % presets.size();
                            const auto& preset = presets[currentPresetIdx];
                            engine.applyConfig(preset.config, /*resetWorld=*/true);
                            renderer.setCurrentPresetName(preset.name);
                            renderer.clearSelection();
                            renderer.showNotification("Applied Preset: " + preset.name, render::UITheme::AccentBlue, 3.5f);
                        }
                        break;
                    case sf::Keyboard::Key::R:
                        executeAction("sim:reset");
                        break;
                    case sf::Keyboard::Key::G:
                        renderer.toggleGrid();
                        break;
                    case sf::Keyboard::Key::C:
                        renderer.toggleCharts();
                        break;
                    case sf::Keyboard::Key::I:
                        renderer.toggleInspector();
                        break;
                    case sf::Keyboard::Key::N:
                        renderer.selectNextOrganism(engine.getWorld());
                        break;
                    case sf::Keyboard::Key::F:
                        renderer.toggleFollow();
                        break;
                    case sf::Keyboard::Key::Home:
                        renderer.resetCamera();
                        break;
                    case sf::Keyboard::Key::F5:
                        executeAction("action:save");
                        break;
                    case sf::Keyboard::Key::F9:
                        executeAction("action:load");
                        break;
                    case sf::Keyboard::Key::E:
                        executeAction("action:export");
                        break;
                    default:
                        break;
                }
            } else {
                std::string action = renderer.processInputEvent(*event, engine.getWorld());
                executeAction(action);
            }
        }

        // 2. Measure wall-clock delta time & update renderer state
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> wallDeltaTime = currentTime - lastTime;
        lastTime = currentTime;

        renderer.update(static_cast<float>(wallDeltaTime.count()));

        // 3. Advance simulation
        engine.update(wallDeltaTime);

        // 4. Render 5-area workspace layout
        auto snapshot = engine.getWorld().getPopulation().computeEvolutionSnapshot(
            engine.getClock().getSimulationTime(),
            engine.getClock().getStepCount()
        );
        double avgLifespan = engine.getWorld().getPopulation().getLineageTracker().getAverageLifespan();

        renderer.beginFrame();
        renderer.renderWorldViewport(engine.getWorld());
        renderer.renderTopBar(engine.isPaused(),
                              engine.getClock().getSimulationTime(),
                              engine.getClock().getStepCount(),
                              snapshot.maxGeneration,
                              engine.getSpeedMultiplier());
        renderer.renderLeftSidebar();
        renderer.renderRightSidebar(engine.getWorld(),
                                    engine.getEventLogger(),
                                    snapshot.populationSize,
                                    engine.getWorld().getPopulation().getHerbivoreCount(),
                                    engine.getWorld().getPopulation().getPredatorCount(),
                                    engine.getWorld().getFoodManager().getFoodCount(),
                                    snapshot.totalBirths,
                                    snapshot.totalDeaths,
                                    engine.getWorld().getPopulation().getTotalPredations(),
                                    engine.getWorld().getPopulation().getAverageEnergy(),
                                    static_cast<float>(avgLifespan),
                                    engine.getWorld().getEnvironment().getSeasonName(),
                                    engine.getWorld().getEnvironment().getFoodRegenMultiplier());
        renderer.renderBottomPanel(engine.getHistoryRecorder().getSnapshots(),
                                   engine.getEventLogger());
        renderer.renderHoverTooltip(engine.getWorld());
        renderer.renderToastNotifications();
        renderer.endFrame();
    }

    if (config.exportCsvOnExit) {
        if (engine.exportHistoryToCsv(config.csvExportPath)) {
            std::cout << "Exported evolutionary telemetry to: " << config.csvExportPath << "\n";
        }
        if (engine.exportAllScientificData(config.dataExportDirectory)) {
            std::cout << "Exported comprehensive scientific observation suite to: " << config.dataExportDirectory << "\n";
        }
    }

    std::cout << "Application closed cleanly.\n";
    return 0;
}
