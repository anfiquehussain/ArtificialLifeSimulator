#pragma once

#include "alife/core/Config.hpp"
#include "alife/simulation/World.hpp"
#include "alife/data/EventLogger.hpp"
#include "alife/data/Presets.hpp"
#include "alife/rendering/UITheme.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include <string>
#include <functional>

namespace alife::render {

/**
 * @brief Interactive UI button for the laboratory layout.
 */
struct UIButton {
    sf::FloatRect bounds;
    std::string id;
    std::string label;
    std::string tooltip;
    bool isHovered{false};
    bool isSelected{false};
    sf::Color bgColor{UITheme::ButtonDefault};
    sf::Color textColor{UITheme::TextPrimary};
};

/**
 * @brief Bottom analysis panel tabs.
 */
enum class BottomTab {
    PopulationChart = 0,
    TraitEvolution = 1,
    EventArchive = 2,
    LabGuide = 3
};

/**
 * @brief Professional Laboratory UI & Scientific Visualization Engine.
 * Features a 5-area workspace:
 *   - A: Top Application Bar (metrics, status, presets, actions)
 *   - B: Left Control Sidebar (sim controls, presets, view layers)
 *   - C: Central Ecosystem Viewport (zoom/pan, organism selection, hover tooltips)
 *   - D: Right Observation Sidebar (ecosystem overview, inspector, real-time event feed)
 *   - E: Bottom Timeline & Analysis Panel (multi-series charts, trait evolution, guide)
 */
class Renderer {
public:
    explicit Renderer(const core::AppConfig& config);

    // Window management
    [[nodiscard]] bool isOpen() const { return window_.isOpen(); }
    void close() { window_.close(); }

    // Event handling
    [[nodiscard]] std::optional<sf::Event> pollEvent() { return window_.pollEvent(); }
    void handleResize(unsigned int newWidth, unsigned int newHeight);

    // Main frame lifecycle
    void beginFrame();
    void endFrame();
    void update(float dt);

    // Central Ecosystem Viewport rendering
    void renderWorldViewport(const sim::World& world);

    // Laboratory UI Shell rendering
    void renderTopBar(bool isPaused,
                      double simTime,
                      uint64_t stepCount,
                      uint32_t maxGen,
                      double speedMultiplier);

    void renderLeftSidebar();

    void renderRightSidebar(const sim::World& world,
                            const data::EventLogger& eventLogger,
                            size_t population,
                            size_t herbivoreCount,
                            size_t predatorCount,
                            size_t foodCount,
                            uint64_t totalBirths,
                            uint64_t totalDeaths,
                            uint64_t totalPredations,
                            float avgEnergy,
                            float avgLifespan,
                            const std::string& seasonName,
                            float regenMultiplier);

    void renderBottomPanel(const std::vector<biology::EvolutionSnapshot>& snapshots,
                           const data::EventLogger& eventLogger);

    void renderHoverTooltip(const sim::World& world);
    void renderToastNotifications();

    // Mouse & UI Interaction handling
    // Returns an action identifier string (e.g. "toggle_pause", "step", "preset:basic", etc.)
    std::string processInputEvent(const sf::Event& event, const sim::World& world);

    // Inspection & Selection
    void selectNextOrganism(const sim::World& world);
    bool selectOrganismAt(sf::Vector2i mousePixelPos, const sim::World& world);
    [[nodiscard]] std::optional<entities::EntityId> getSelectedOrganismId() const noexcept { return selectedOrganismId_; }
    void clearSelection() noexcept { selectedOrganismId_ = std::nullopt; }

    // Camera control
    void toggleFollow() noexcept { followSelected_ = !followSelected_; }
    [[nodiscard]] bool isFollowing() const noexcept { return followSelected_; }
    void resetCamera();

    // Preset & Toast Notifications
    void setCurrentPresetName(std::string name) { currentPresetName_ = std::move(name); }
    [[nodiscard]] const std::string& getCurrentPresetName() const noexcept { return currentPresetName_; }
    void showNotification(const std::string& message, sf::Color color = UITheme::HerbivoreGreen, float durationSec = 3.0f);

    // Toggles for external callers / hotkeys
    void toggleGrid() noexcept { showGrid_ = !showGrid_; }
    void toggleLeftSidebar() noexcept { leftSidebarOpen_ = !leftSidebarOpen_; updateViewports(); }
    void toggleRightSidebar() noexcept { rightSidebarOpen_ = !rightSidebarOpen_; updateViewports(); }
    void toggleBottomPanel() noexcept { bottomPanelOpen_ = !bottomPanelOpen_; updateViewports(); }
    void toggleCharts() noexcept { activeBottomTab_ = BottomTab::PopulationChart; bottomPanelOpen_ = true; updateViewports(); }
    void toggleInspector() noexcept { rightSidebarOpen_ = !rightSidebarOpen_; updateViewports(); }
    [[nodiscard]] bool isInspectorOpen() const noexcept { return rightSidebarOpen_; }
    [[nodiscard]] bool isShowingCharts() const noexcept { return bottomPanelOpen_ && activeBottomTab_ == BottomTab::PopulationChart; }

    [[nodiscard]] sf::RenderWindow& getWindow() { return window_; }

private:
    void rebuildGridVertices(float worldWidth, float worldHeight, float cellSize);
    void updateViewports();
    [[nodiscard]] sf::FloatRect getCentralViewportBounds() const;
    void registerButton(const sf::FloatRect& bounds,
                        const std::string& id,
                        const std::string& label,
                        const std::string& tooltip = "",
                        bool isSelected = false,
                        sf::Color bgColor = UITheme::ButtonDefault,
                        sf::Color textColor = UITheme::TextPrimary);

    void registerButton(sf::Vector2f pos,
                        sf::Vector2f size,
                        const std::string& id,
                        const std::string& label,
                        const std::string& tooltip = "",
                        bool isSelected = false,
                        sf::Color bgColor = UITheme::ButtonDefault,
                        sf::Color textColor = UITheme::TextPrimary) {
        registerButton(sf::FloatRect(pos, size), id, label, tooltip, isSelected, bgColor, textColor);
    }

    sf::RenderWindow window_;
    sf::View worldView_;
    sf::View uiView_;

    // Layout configuration
    float leftSidebarWidth_{240.0f};
    float rightSidebarWidth_{330.0f};
    float topBarHeight_{42.0f};
    float bottomPanelHeight_{220.0f};

    bool leftSidebarOpen_{true};
    bool rightSidebarOpen_{true};
    bool bottomPanelOpen_{true};
    BottomTab activeBottomTab_{BottomTab::PopulationChart};

    // Camera Navigation State
    float cameraZoom_{1.0f};
    bool isDraggingCamera_{false};
    sf::Vector2i dragStartMousePos_{0, 0};
    sf::Vector2f dragStartCenter_{600.0f, 400.0f};
    bool followSelected_{false};

    // Layer Toggles
    bool showGrid_{true};
    bool showOrganisms_{true};
    bool showFood_{true};
    bool showVisionRadii_{false};
    bool showSensoryRays_{true};
    bool showHeadings_{true};

    // Inspection & Hover State
    std::optional<entities::EntityId> selectedOrganismId_{std::nullopt};
    std::optional<entities::EntityId> hoveredOrganismId_{std::nullopt};
    sf::Vector2i currentMousePos_{0, 0};
    std::string hoveredButtonTooltip_;

    // UI Interactive Button Cache (cleared and repopulated each frame)
    std::vector<UIButton> buttons_;

    // Grid vertex cache
    std::vector<sf::Vertex> gridVertices_;
    float cachedGridWidth_{0.0f};
    float cachedGridHeight_{0.0f};
    float cachedCellSize_{0.0f};

    // Reusable shapes & batches
    sf::CircleShape perceptionCircle_;
    sf::CircleShape selectionRing_;
    std::vector<sf::Vertex> foodBatch_;
    std::vector<sf::Vertex> herbivoreBatch_;
    std::vector<sf::Vertex> predatorBatch_;
    std::vector<sf::Vertex> headingBatch_;
    std::vector<sf::Vertex> chartBatch_;

    // Preset & Timing state
    std::string currentPresetName_{"Basic Evolution"};
    core::AppConfig config_;

    // FPS calculation
    float fpsTimer_{0.0f};
    int frameCount_{0};
    float currentFps_{60.0f};

    // Toast Notifications
    struct Notification {
        std::string text;
        float remainingTime{0.0f};
        sf::Color color{UITheme::HerbivoreGreen};
    };
    std::vector<Notification> notifications_;

    // Fonts
    sf::Font fontUi_;
    bool fontLoaded_{false};
};

} // namespace alife::render
