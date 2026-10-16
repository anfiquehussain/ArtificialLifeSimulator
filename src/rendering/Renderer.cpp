#include "alife/rendering/Renderer.hpp"
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace alife::render {

Renderer::Renderer(const core::AppConfig& config)
    : window_(sf::VideoMode({config.windowWidth, config.windowHeight}), config.windowTitle, sf::Style::Default),
      worldView_(sf::FloatRect({0.0f, 0.0f}, {config.worldWidth, config.worldHeight})),
      uiView_(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(config.windowWidth), static_cast<float>(config.windowHeight)})),
      perceptionCircle_(config.organismPerceptionRadius),
      selectionRing_(config.organismRadius + 5.0f),
      config_(config)
{
    window_.setFramerateLimit(config.targetFrameRate);
    foodBatch_.reserve(config.maxFoodCapacity * 6);
    herbivoreBatch_.reserve(config.maxPopulation * 24);
    predatorBatch_.reserve(config.maxPopulation * 12);
    headingBatch_.reserve(config.maxPopulation * 2);
    chartBatch_.reserve(4096);

    // Setup selection ring
    selectionRing_.setOrigin({config.organismRadius + 5.0f, config.organismRadius + 5.0f});
    selectionRing_.setFillColor(sf::Color::Transparent);
    selectionRing_.setOutlineColor(UITheme::SelectionGold);
    selectionRing_.setOutlineThickness(2.0f);

    // Setup perception circle
    perceptionCircle_.setOrigin({config.organismPerceptionRadius, config.organismPerceptionRadius});
    perceptionCircle_.setFillColor(UITheme::PerceptionArea);
    perceptionCircle_.setOutlineColor(UITheme::PerceptionBorder);
    perceptionCircle_.setOutlineThickness(1.0f);

    // Load clean interface font (Segoe UI preferred on Windows 11, Consolas/Arial fallback)
    const std::vector<std::string> fontCandidates = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/consola.ttf"
    };

    for (const auto& path : fontCandidates) {
        if (fontUi_.openFromFile(path)) {
            fontLoaded_ = true;
            break;
        }
    }

    // Initialize camera center to middle of world
    worldView_.setCenter({config.worldWidth * 0.5f, config.worldHeight * 0.5f});
    updateViewports();
}

void Renderer::handleResize(unsigned int newWidth, unsigned int newHeight) {
    if (newWidth == 0 || newHeight == 0) return;

    uiView_.setSize({static_cast<float>(newWidth), static_cast<float>(newHeight)});
    uiView_.setCenter({static_cast<float>(newWidth) * 0.5f, static_cast<float>(newHeight) * 0.5f});

    updateViewports();
}

sf::FloatRect Renderer::getCentralViewportBounds() const {
    float winW = static_cast<float>(window_.getSize().x);
    float winH = static_cast<float>(window_.getSize().y);

    float left = leftSidebarOpen_ ? leftSidebarWidth_ : 0.0f;
    float right = rightSidebarOpen_ ? rightSidebarWidth_ : 0.0f;
    float top = topBarHeight_;
    float bottom = bottomPanelOpen_ ? bottomPanelHeight_ : 0.0f;

    float vpX = left;
    float vpY = top;
    float vpW = std::max(100.0f, winW - left - right);
    float vpH = std::max(100.0f, winH - top - bottom);

    return sf::FloatRect({vpX, vpY}, {vpW, vpH});
}

void Renderer::updateViewports() {
    float winW = static_cast<float>(window_.getSize().x);
    float winH = static_cast<float>(window_.getSize().y);
    if (winW <= 0.0f || winH <= 0.0f) return;

    sf::FloatRect bounds = getCentralViewportBounds();

    // Map to normalized SFML viewport coordinates [0..1]
    worldView_.setViewport(sf::FloatRect({bounds.position.x / winW, bounds.position.y / winH},
                                        {bounds.size.x / winW, bounds.size.y / winH}));

    // Adjust world view size maintaining aspect ratio
    float vpRatio = bounds.size.x / bounds.size.y;
    float worldBaseW = config_.worldWidth;
    float worldBaseH = config_.worldHeight;
    float worldRatio = worldBaseW / worldBaseH;

    float viewW, viewH;
    if (vpRatio > worldRatio) {
        viewH = worldBaseH * cameraZoom_;
        viewW = viewH * vpRatio;
    } else {
        viewW = worldBaseW * cameraZoom_;
        viewH = viewW / vpRatio;
    }
    worldView_.setSize({viewW, viewH});
}

void Renderer::resetCamera() {
    cameraZoom_ = 1.0f;
    followSelected_ = false;
    worldView_.setCenter({config_.worldWidth * 0.5f, config_.worldHeight * 0.5f});
    updateViewports();
    showNotification("Camera centered on ecosystem arena", UITheme::AccentBlue, 2.0f);
}

void Renderer::registerButton(const sf::FloatRect& bounds,
                              const std::string& id,
                              const std::string& label,
                              const std::string& tooltip,
                              bool isSelected,
                              sf::Color bgColor,
                              sf::Color textColor) {
    bool isHovered = bounds.contains({static_cast<float>(currentMousePos_.x), static_cast<float>(currentMousePos_.y)});
    if (isHovered && !tooltip.empty()) {
        hoveredButtonTooltip_ = tooltip;
    }
    buttons_.push_back({bounds, id, label, tooltip, isHovered, isSelected, bgColor, textColor});
}

void Renderer::update(float dt) {
    if (dt <= 0.0f) return;

    frameCount_++;
    fpsTimer_ += dt;
    if (fpsTimer_ >= 0.5f) {
        currentFps_ = static_cast<float>(frameCount_) / fpsTimer_;
        frameCount_ = 0;
        fpsTimer_ = 0.0f;
    }

    // Update toast notifications
    for (auto it = notifications_.begin(); it != notifications_.end();) {
        it->remainingTime -= dt;
        if (it->remainingTime <= 0.0f) {
            it = notifications_.erase(it);
        } else {
            ++it;
        }
    }
}

void Renderer::showNotification(const std::string& message, sf::Color color, float durationSec) {
    notifications_.push_back({message, durationSec, color});
    if (notifications_.size() > 5) {
        notifications_.erase(notifications_.begin());
    }
}

void Renderer::beginFrame() {
    buttons_.clear();
    hoveredButtonTooltip_.clear();
    window_.clear(UITheme::AppBackground);
}

void Renderer::endFrame() {
    window_.display();
}

std::string Renderer::processInputEvent(const sf::Event& event, const sim::World& world) {
    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
        currentMousePos_ = mouseMoved->position;

        // Camera panning with right mouse button
        if (isDraggingCamera_) {
            sf::Vector2f curWorld = window_.mapPixelToCoords(currentMousePos_, worldView_);
            sf::Vector2f startWorld = window_.mapPixelToCoords(dragStartMousePos_, worldView_);
            sf::Vector2f delta = startWorld - curWorld;
            worldView_.setCenter(worldView_.getCenter() + delta);
            dragStartMousePos_ = currentMousePos_;
            followSelected_ = false;
        }

        // Hit-test organism hover in central viewport
        hoveredOrganismId_ = std::nullopt;
        sf::FloatRect vpBounds = getCentralViewportBounds();
        if (vpBounds.contains({static_cast<float>(currentMousePos_.x), static_cast<float>(currentMousePos_.y)})) {
            sf::Vector2f worldPos = window_.mapPixelToCoords(currentMousePos_, worldView_);
            const auto& organisms = world.getPopulation().getOrganisms();
            for (const auto& org : organisms) {
                if (!org.isAlive()) continue;
                float r = org.isPredator() ? config_.predatorBodyRadius : config_.organismRadius;
                float hitR = r + 10.0f;
                float dx = org.getPosition().x - worldPos.x;
                float dy = org.getPosition().y - worldPos.y;
                if (dx * dx + dy * dy <= hitR * hitR) {
                    hoveredOrganismId_ = org.getId();
                    break;
                }
            }
        }
    } else if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        currentMousePos_ = mousePressed->position;

        if (mousePressed->button == sf::Mouse::Button::Left) {
            for (const auto& btn : buttons_) {
                if (btn.bounds.contains({static_cast<float>(currentMousePos_.x), static_cast<float>(currentMousePos_.y)})) {
                    if (btn.id == "toggle:left_sidebar") {
                        toggleLeftSidebar();
                    } else if (btn.id == "toggle:right_sidebar") {
                        toggleRightSidebar();
                    } else if (btn.id == "toggle:bottom_panel") {
                        toggleBottomPanel();
                    } else if (btn.id == "tab:pop_chart") {
                        activeBottomTab_ = BottomTab::PopulationChart;
                        bottomPanelOpen_ = true;
                        updateViewports();
                    } else if (btn.id == "tab:trait_chart") {
                        activeBottomTab_ = BottomTab::TraitEvolution;
                        bottomPanelOpen_ = true;
                        updateViewports();
                    } else if (btn.id == "tab:events") {
                        activeBottomTab_ = BottomTab::EventArchive;
                        bottomPanelOpen_ = true;
                        updateViewports();
                    } else if (btn.id == "tab:guide") {
                        activeBottomTab_ = BottomTab::LabGuide;
                        bottomPanelOpen_ = true;
                        updateViewports();
                    } else if (btn.id == "layer:grid") {
                        showGrid_ = !showGrid_;
                    } else if (btn.id == "layer:organisms") {
                        showOrganisms_ = !showOrganisms_;
                    } else if (btn.id == "layer:food") {
                        showFood_ = !showFood_;
                    } else if (btn.id == "layer:headings") {
                        showHeadings_ = !showHeadings_;
                    } else if (btn.id == "layer:rays") {
                        showSensoryRays_ = !showSensoryRays_;
                    } else if (btn.id == "layer:follow" || btn.id == "org:follow") {
                        toggleFollow();
                    } else if (btn.id == "cam:reset") {
                        resetCamera();
                    } else if (btn.id == "org:deselect") {
                        clearSelection();
                    }
                    return btn.id;
                }
            }

            // Click inside central viewport -> organism selection
            sf::FloatRect vpBounds = getCentralViewportBounds();
            if (vpBounds.contains({static_cast<float>(currentMousePos_.x), static_cast<float>(currentMousePos_.y)})) {
                selectOrganismAt(currentMousePos_, world);
            }
        } else if (mousePressed->button == sf::Mouse::Button::Right || mousePressed->button == sf::Mouse::Button::Middle) {
            sf::FloatRect vpBounds = getCentralViewportBounds();
            if (vpBounds.contains({static_cast<float>(currentMousePos_.x), static_cast<float>(currentMousePos_.y)})) {
                isDraggingCamera_ = true;
                dragStartMousePos_ = currentMousePos_;
                dragStartCenter_ = worldView_.getCenter();
            }
        }
    } else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mouseReleased->button == sf::Mouse::Button::Right || mouseReleased->button == sf::Mouse::Button::Middle) {
            isDraggingCamera_ = false;
        }
    } else if (const auto* mouseWheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        sf::FloatRect vpBounds = getCentralViewportBounds();
        if (vpBounds.contains({static_cast<float>(mouseWheel->position.x), static_cast<float>(mouseWheel->position.y)})) {
            if (mouseWheel->delta > 0.0f) {
                cameraZoom_ = std::clamp(cameraZoom_ * 0.85f, 0.2f, 5.0f);
            } else if (mouseWheel->delta < 0.0f) {
                cameraZoom_ = std::clamp(cameraZoom_ * 1.15f, 0.2f, 5.0f);
            }
            updateViewports();
        }
    }

    return "";
}

bool Renderer::selectOrganismAt(sf::Vector2i mousePixelPos, const sim::World& world) {
    sf::Vector2f worldPos = window_.mapPixelToCoords(mousePixelPos, worldView_);
    const auto& organisms = world.getPopulation().getOrganisms();

    for (const auto& org : organisms) {
        if (!org.isAlive()) continue;

        float r = org.isPredator() ? config_.predatorBodyRadius : config_.organismRadius;
        float hitR = r + 12.0f;
        float dx = org.getPosition().x - worldPos.x;
        float dy = org.getPosition().y - worldPos.y;

        if (dx * dx + dy * dy <= hitR * hitR) {
            selectedOrganismId_ = org.getId();
            rightSidebarOpen_ = true;
            updateViewports();
            std::string role = org.isPredator() ? "Predator" : "Herbivore";
            showNotification("Inspecting " + role + " #" + std::to_string(org.getId()) + " (Gen " + std::to_string(org.getGeneration()) + ")",
                             org.isPredator() ? UITheme::PredatorRed : UITheme::HerbivoreGreen, 2.5f);
            return true;
        }
    }

    selectedOrganismId_ = std::nullopt;
    return false;
}

void Renderer::selectNextOrganism(const sim::World& world) {
    const auto& organisms = world.getPopulation().getOrganisms();
    if (organisms.empty()) {
        selectedOrganismId_ = std::nullopt;
        return;
    }

    if (!selectedOrganismId_.has_value()) {
        for (const auto& org : organisms) {
            if (org.isAlive()) {
                selectedOrganismId_ = org.getId();
                rightSidebarOpen_ = true;
                updateViewports();
                return;
            }
        }
        return;
    }

    auto curId = *selectedOrganismId_;
    bool foundCurrent = false;
    for (const auto& org : organisms) {
        if (!org.isAlive()) continue;
        if (foundCurrent) {
            selectedOrganismId_ = org.getId();
            rightSidebarOpen_ = true;
            updateViewports();
            return;
        }
        if (org.getId() == curId) foundCurrent = true;
    }
    for (const auto& org : organisms) {
        if (org.isAlive()) {
            selectedOrganismId_ = org.getId();
            rightSidebarOpen_ = true;
            updateViewports();
            return;
        }
    }
    selectedOrganismId_ = std::nullopt;
}

void Renderer::rebuildGridVertices(float worldWidth, float worldHeight, float cellSize) {
    if (worldWidth == cachedGridWidth_ && worldHeight == cachedGridHeight_ && cellSize == cachedCellSize_) {
        return;
    }

    gridVertices_.clear();
    cachedGridWidth_ = worldWidth;
    cachedGridHeight_ = worldHeight;
    cachedCellSize_ = cellSize;

    if (cellSize <= 0.0f) return;

    for (float x = 0.0f; x <= worldWidth; x += cellSize) {
        gridVertices_.emplace_back(sf::Vector2f{x, 0.0f}, UITheme::GridLine);
        gridVertices_.emplace_back(sf::Vector2f{x, worldHeight}, UITheme::GridLine);
    }

    for (float y = 0.0f; y <= worldHeight; y += cellSize) {
        gridVertices_.emplace_back(sf::Vector2f{0.0f, y}, UITheme::GridLine);
        gridVertices_.emplace_back(sf::Vector2f{worldWidth, y}, UITheme::GridLine);
    }
}

void Renderer::renderWorldViewport(const sim::World& world) {
    // 1. Follow camera centering
    if (followSelected_ && selectedOrganismId_.has_value()) {
        for (const auto& org : world.getPopulation().getOrganisms()) {
            if (org.getId() == *selectedOrganismId_ && org.isAlive()) {
                worldView_.setCenter({org.getPosition().x, org.getPosition().y});
                break;
            }
        }
    }

    window_.setView(worldView_);

    float w = world.getWidth();
    float h = world.getHeight();

    // 2. Arena background
    sf::RectangleShape arenaBg({w, h});
    arenaBg.setPosition({0.0f, 0.0f});
    arenaBg.setFillColor(UITheme::ViewportBackground);
    arenaBg.setOutlineColor(UITheme::ArenaBorder);
    arenaBg.setOutlineThickness(3.0f);
    window_.draw(arenaBg);

    // 3. Coordinate Grid
    if (showGrid_) {
        rebuildGridVertices(w, h, config_.gridCellSize);
        if (!gridVertices_.empty()) {
            window_.draw(gridVertices_.data(), gridVertices_.size(), sf::PrimitiveType::Lines);
        }
    }

    // 4. Food Nutrients
    if (showFood_) {
        foodBatch_.clear();
        float r = config_.foodRadius;
        for (const auto& food : world.getFoodManager().getFoodItems()) {
            if (food.isConsumed()) continue;
            const auto& pos = food.getPosition();

            sf::Vector2f top{pos.x, pos.y - r};
            sf::Vector2f right{pos.x + r, pos.y};
            sf::Vector2f bottom{pos.x, pos.y + r};
            sf::Vector2f left{pos.x - r, pos.y};

            // Diamond octahedron pellet (2 triangles)
            foodBatch_.emplace_back(left, UITheme::FoodAmber);
            foodBatch_.emplace_back(top, UITheme::FoodGlow);
            foodBatch_.emplace_back(right, UITheme::FoodAmber);

            foodBatch_.emplace_back(left, UITheme::FoodAmber);
            foodBatch_.emplace_back(right, UITheme::FoodAmber);
            foodBatch_.emplace_back(bottom, UITheme::FoodGlow);
        }
        if (!foodBatch_.empty()) {
            window_.draw(foodBatch_.data(), foodBatch_.size(), sf::PrimitiveType::Triangles);
        }
    }

    // 5. Organisms (Herbivores & Predators)
    if (showOrganisms_) {
        herbivoreBatch_.clear();
        predatorBatch_.clear();
        headingBatch_.clear();

        constexpr int HERB_SEGS = 8;
        static const auto unitCircle = []() {
            std::array<sf::Vector2f, HERB_SEGS> pts;
            for (int i = 0; i < HERB_SEGS; ++i) {
                float a = static_cast<float>(i) * (6.2831853f / static_cast<float>(HERB_SEGS));
                pts[i] = {std::cos(a), std::sin(a)};
            }
            return pts;
        }();

        const auto& organisms = world.getPopulation().getOrganisms();

        for (const auto& org : organisms) {
            if (!org.isAlive()) continue;

            const auto& pos = org.getPosition();
            float energyRatio = org.getMaxEnergy() > 0.0f ? std::clamp(org.getEnergy() / org.getMaxEnergy(), 0.0f, 1.0f) : 0.0f;

            if (org.isHerbivore()) {
                // Bio-emerald herbivore circle
                float r = config_.organismRadius;
                uint8_t cr = static_cast<uint8_t>(220.0f * (1.0f - energyRatio) + 65.0f * energyRatio);
                uint8_t cg = static_cast<uint8_t>(90.0f * (1.0f - energyRatio) + 215.0f * energyRatio);
                uint8_t cb = static_cast<uint8_t>(50.0f * (1.0f - energyRatio) + 155.0f * energyRatio);
                sf::Color bodyColor(cr, cg, cb);
                sf::Color coreColor(static_cast<uint8_t>(std::min(255, cr + 30)),
                                    static_cast<uint8_t>(std::min(255, cg + 35)),
                                    static_cast<uint8_t>(std::min(255, cb + 35)));

                sf::Vector2f centerPos{pos.x, pos.y};
                for (int i = 0; i < HERB_SEGS; ++i) {
                    int next = (i + 1) % HERB_SEGS;
                    herbivoreBatch_.emplace_back(centerPos, coreColor);
                    herbivoreBatch_.emplace_back(sf::Vector2f{pos.x + unitCircle[i].x * r, pos.y + unitCircle[i].y * r}, bodyColor);
                    herbivoreBatch_.emplace_back(sf::Vector2f{pos.x + unitCircle[next].x * r, pos.y + unitCircle[next].y * r}, bodyColor);
                }
            } else {
                // Predator: sharp diamond/kite hunter silhouette
                float r = config_.predatorBodyRadius;
                const auto& dir = org.getDirection();
                sf::Vector2f perp{-dir.y, dir.x};

                sf::Vector2f nose{pos.x + dir.x * (r * 1.4f), pos.y + dir.y * (r * 1.4f)};
                sf::Vector2f rightWing{pos.x + perp.x * (r * 0.9f) - dir.x * (r * 0.5f), pos.y + perp.y * (r * 0.9f) - dir.y * (r * 0.5f)};
                sf::Vector2f leftWing{pos.x - perp.x * (r * 0.9f) - dir.x * (r * 0.5f), pos.y - perp.y * (r * 0.9f) - dir.y * (r * 0.5f)};
                sf::Vector2f tail{pos.x - dir.x * (r * 0.8f), pos.y - dir.y * (r * 0.8f)};

                sf::Color predColor = UITheme::PredatorRed;
                sf::Color predCore = UITheme::PredatorGlow;

                // Triangle 1: nose, rightWing, center
                predatorBatch_.emplace_back(nose, predCore);
                predatorBatch_.emplace_back(rightWing, predColor);
                predatorBatch_.emplace_back(sf::Vector2f{pos.x, pos.y}, predColor);

                // Triangle 2: nose, center, leftWing
                predatorBatch_.emplace_back(nose, predCore);
                predatorBatch_.emplace_back(sf::Vector2f{pos.x, pos.y}, predColor);
                predatorBatch_.emplace_back(leftWing, predColor);

                // Triangle 3: rightWing, tail, center
                predatorBatch_.emplace_back(rightWing, predColor);
                predatorBatch_.emplace_back(tail, predColor);
                predatorBatch_.emplace_back(sf::Vector2f{pos.x, pos.y}, predCore);

                // Triangle 4: leftWing, center, tail
                predatorBatch_.emplace_back(leftWing, predColor);
                predatorBatch_.emplace_back(sf::Vector2f{pos.x, pos.y}, predCore);
                predatorBatch_.emplace_back(tail, predColor);
            }

            // Heading indicator line
            if (showHeadings_) {
                const auto& dir = org.getDirection();
                float len = org.isPredator() ? 14.0f : 10.0f;
                sf::Color hCol = org.isPredator() ? sf::Color(255, 120, 120, 200) : sf::Color(255, 255, 255, 180);
                headingBatch_.emplace_back(sf::Vector2f{pos.x, pos.y}, hCol);
                headingBatch_.emplace_back(sf::Vector2f{pos.x + dir.x * len, pos.y + dir.y * len}, sf::Color(hCol.r, hCol.g, hCol.b, 40));
            }
        }

        if (!herbivoreBatch_.empty()) {
            window_.draw(herbivoreBatch_.data(), herbivoreBatch_.size(), sf::PrimitiveType::Triangles);
        }
        if (!predatorBatch_.empty()) {
            window_.draw(predatorBatch_.data(), predatorBatch_.size(), sf::PrimitiveType::Triangles);
        }
        if (!headingBatch_.empty()) {
            window_.draw(headingBatch_.data(), headingBatch_.size(), sf::PrimitiveType::Lines);
        }

        // 6. Highlight hovered organism (soft cyan outline)
        if (hoveredOrganismId_.has_value() && hoveredOrganismId_ != selectedOrganismId_) {
            for (const auto& org : organisms) {
                if (org.getId() == *hoveredOrganismId_ && org.isAlive()) {
                    float r = org.isPredator() ? (config_.predatorBodyRadius + 4.0f) : (config_.organismRadius + 4.0f);
                    sf::CircleShape hoverRing(r);
                    hoverRing.setOrigin({r, r});
                    hoverRing.setPosition({org.getPosition().x, org.getPosition().y});
                    hoverRing.setFillColor(sf::Color::Transparent);
                    hoverRing.setOutlineColor(UITheme::TextAccent);
                    hoverRing.setOutlineThickness(1.5f);
                    window_.draw(hoverRing);
                    break;
                }
            }
        }

        // 7. Highlight selected organism (reticle, sensory rays, perception area)
        if (selectedOrganismId_.has_value()) {
            for (const auto& org : organisms) {
                if (org.getId() == *selectedOrganismId_ && org.isAlive()) {
                    const auto& orgPos = org.getPosition();
                    float r = org.isPredator() ? (config_.predatorBodyRadius + 5.0f) : (config_.organismRadius + 5.0f);

                    // Luminous selection ring
                    selectionRing_.setRadius(r);
                    selectionRing_.setOrigin({r, r});
                    selectionRing_.setPosition({orgPos.x, orgPos.y});
                    window_.draw(selectionRing_);

                    // Vision perception circle
                    if (showVisionRadii_) {
                        perceptionCircle_.setRadius(config_.organismPerceptionRadius);
                        perceptionCircle_.setOrigin({config_.organismPerceptionRadius, config_.organismPerceptionRadius});
                        perceptionCircle_.setPosition({orgPos.x, orgPos.y});
                        window_.draw(perceptionCircle_);
                    }

                    // Sensory target & threat rays
                    if (showSensoryRays_) {
                        const auto& sensors = org.getLastSensors();
                        float orgHeading = std::atan2(org.getDirection().y, org.getDirection().x);

                        // Target ray (food or prey)
                        if (sensors.size() >= 4 && sensors[3] > 0.0f) {
                            float actualDist = sensors[2] * config_.organismPerceptionRadius;
                            float actualAngle = sensors[1] * 3.14159265f;
                            float targetAngle = orgHeading + actualAngle;

                            sf::Vector2f targetPos{orgPos.x + std::cos(targetAngle) * actualDist,
                                                   orgPos.y + std::sin(targetAngle) * actualDist};
                            sf::Vertex ray[2] = {
                                sf::Vertex{sf::Vector2f{orgPos.x, orgPos.y}, UITheme::SensoryTarget},
                                sf::Vertex{targetPos, sf::Color(255, 225, 90, 50)}
                            };
                            window_.draw(ray, 2, sf::PrimitiveType::Lines);
                        }

                        // Threat ray (predator)
                        if (sensors.size() >= 7 && sensors[6] > 0.0f) {
                            float actualDist = sensors[5] * config_.organismPerceptionRadius;
                            float actualAngle = sensors[4] * 3.14159265f;
                            float threatAngle = orgHeading + actualAngle;

                            sf::Vector2f threatPos{orgPos.x + std::cos(threatAngle) * actualDist,
                                                   orgPos.y + std::sin(threatAngle) * actualDist};
                            sf::Vertex threatRay[2] = {
                                sf::Vertex{sf::Vector2f{orgPos.x, orgPos.y}, UITheme::SensoryThreat},
                                sf::Vertex{threatPos, sf::Color(245, 90, 70, 50)}
                            };
                            window_.draw(threatRay, 2, sf::PrimitiveType::Lines);
                        }
                    }
                    break;
                }
            }
        }
    }
}

void Renderer::renderTopBar(bool isPaused,
                            double simTime,
                            uint64_t stepCount,
                            uint32_t maxGen,
                            double speedMultiplier) {
    window_.setView(uiView_);
    float winW = uiView_.getSize().x;

    // Background bar
    sf::RectangleShape barBg({winW, topBarHeight_});
    barBg.setPosition({0.0f, 0.0f});
    barBg.setFillColor(UITheme::PanelBackground);
    barBg.setOutlineColor(UITheme::PanelBorder);
    barBg.setOutlineThickness(1.0f);
    window_.draw(barBg);

    if (!fontLoaded_) return;

    // 1. Logo & App Title
    sf::Text logo(fontUi_, "ALIFE LAB", 14);
    logo.setFillColor(UITheme::AccentBlue);
    logo.setStyle(sf::Text::Bold);
    logo.setPosition({14.0f, 10.0f});
    window_.draw(logo);

    sf::Text subtitle(fontUi_, "// Evolution & Ecology Laboratory", 12);
    subtitle.setFillColor(UITheme::TextMuted);
    subtitle.setPosition({100.0f, 12.0f});
    window_.draw(subtitle);

    // 2. Status Badge [RUNNING] / [PAUSED]
    float badgeX = 350.0f;
    sf::RectangleShape statusPill({75.0f, 22.0f});
    statusPill.setPosition({badgeX, 10.0f});
    statusPill.setFillColor(isPaused ? sf::Color(70, 55, 20) : sf::Color(20, 60, 40));
    statusPill.setOutlineColor(isPaused ? UITheme::StatusPaused : UITheme::StatusRunning);
    statusPill.setOutlineThickness(1.0f);
    window_.draw(statusPill);

    sf::Text statusText(fontUi_, isPaused ? "PAUSED" : "RUNNING", 11);
    statusText.setFillColor(isPaused ? UITheme::StatusPaused : UITheme::StatusRunning);
    statusText.setStyle(sf::Text::Bold);
    statusText.setPosition({badgeX + 10.0f, 13.0f});
    window_.draw(statusText);

    // 3. Preset Badge
    float presetX = badgeX + 85.0f;
    std::string presetStr = "Preset: " + currentPresetName_;
    sf::Text presetText(fontUi_, presetStr, 11);
    presetText.setFillColor(UITheme::TextSecondary);
    presetText.setPosition({presetX, 13.0f});
    window_.draw(presetText);

    // 4. Metrics: Time, Steps, Generation, Speed & FPS
    float statsX = presetX + 200.0f;
    std::ostringstream ss;
    ss << "Time: " << std::fixed << std::setprecision(1) << simTime << "s  |  "
       << "Gen: " << maxGen << "  |  "
       << "Steps: " << stepCount << "  |  "
       << std::setprecision(1) << speedMultiplier << "x Speed  |  "
       << std::setprecision(0) << currentFps_ << " FPS";

    sf::Text statsText(fontUi_, ss.str(), 11);
    statsText.setFillColor(UITheme::TextSecondary);
    statsText.setPosition({statsX, 13.0f});
    window_.draw(statsText);

    // 5. Quick Action Buttons on Right
    float btnW = 75.0f;
    float btnH = 24.0f;
    float btnY = 9.0f;
    float startBtnX = winW - 350.0f;

    registerButton({{startBtnX, btnY}, {btnW, btnH}}, "action:save", "F5 Save", "Quick save simulation snapshot to file");
    registerButton({{startBtnX + 80.0f, btnY}, {btnW, btnH}}, "action:load", "F9 Load", "Quick load simulation snapshot from file");
    registerButton({{startBtnX + 160.0f, btnY}, {95.0f, btnH}}, "action:export", "E Export CSV", "Export comprehensive scientific datasets");
    registerButton({{startBtnX + 260.0f, btnY}, {70.0f, btnH}}, "tab:guide", "? Guide", "Open laboratory guide & documentation");

    for (const auto& btn : buttons_) {
        if (btn.bounds.position.y < topBarHeight_) {
            sf::RectangleShape r(btn.bounds.size);
            r.setPosition(btn.bounds.position);
            r.setFillColor(btn.isHovered ? UITheme::ButtonHover : UITheme::ButtonDefault);
            r.setOutlineColor(btn.isHovered ? UITheme::AccentBlue : UITheme::ButtonBorder);
            r.setOutlineThickness(1.0f);
            window_.draw(r);

            sf::Text t(fontUi_, btn.label, 11);
            t.setFillColor(btn.isHovered ? UITheme::TextPrimary : UITheme::TextSecondary);
            t.setPosition({btn.bounds.position.x + 8.0f, btn.bounds.position.y + 4.0f});
            window_.draw(t);
        }
    }
}

void Renderer::renderLeftSidebar() {
    window_.setView(uiView_);
    float winH = uiView_.getSize().y;

    if (!leftSidebarOpen_) {
        // Collapsed mini-tab
        sf::FloatRect toggleBounds{{0.0f, topBarHeight_ + 10.0f}, {28.0f, 80.0f}};
        registerButton(toggleBounds, "toggle:left_sidebar", ">", "Expand Controls Sidebar");

        sf::RectangleShape tab(toggleBounds.size);
        tab.setPosition(toggleBounds.position);
        tab.setFillColor(UITheme::PanelBackground);
        tab.setOutlineColor(UITheme::PanelBorder);
        tab.setOutlineThickness(1.0f);
        window_.draw(tab);

        if (fontLoaded_) {
            sf::Text t(fontUi_, ">", 14);
            t.setFillColor(UITheme::AccentBlue);
            t.setPosition({toggleBounds.position.x + 8.0f, toggleBounds.position.y + 30.0f});
            window_.draw(t);
        }
        return;
    }

    float barH = winH - topBarHeight_ - (bottomPanelOpen_ ? bottomPanelHeight_ : 0.0f);

    sf::RectangleShape sidebarBg({leftSidebarWidth_, barH});
    sidebarBg.setPosition({0.0f, topBarHeight_});
    sidebarBg.setFillColor(UITheme::PanelBackground);
    sidebarBg.setOutlineColor(UITheme::PanelBorder);
    sidebarBg.setOutlineThickness(1.0f);
    window_.draw(sidebarBg);

    if (!fontLoaded_) return;

    // Sidebar Header
    sf::Text header(fontUi_, "CONTROLS & PRESETS", 12);
    header.setFillColor(UITheme::TextPrimary);
    header.setStyle(sf::Text::Bold);
    header.setPosition({14.0f, topBarHeight_ + 10.0f});
    window_.draw(header);

    // Collapse Button [ < ]
    sf::FloatRect collapseBtnBounds{{leftSidebarWidth_ - 30.0f, topBarHeight_ + 8.0f}, {22.0f, 20.0f}};
    registerButton(collapseBtnBounds, "toggle:left_sidebar", "<", "Collapse Sidebar");
    sf::RectangleShape cBtn(collapseBtnBounds.size);
    cBtn.setPosition(collapseBtnBounds.position);
    cBtn.setFillColor(UITheme::ButtonDefault);
    cBtn.setOutlineColor(UITheme::ButtonBorder);
    cBtn.setOutlineThickness(1.0f);
    window_.draw(cBtn);

    sf::Text cText(fontUi_, "<", 11);
    cText.setFillColor(UITheme::TextSecondary);
    cText.setPosition({collapseBtnBounds.position.x + 6.0f, collapseBtnBounds.position.y + 2.0f});
    window_.draw(cText);

    float curY = topBarHeight_ + 36.0f;

    // --- CARD 1: SIMULATION CONTROLS ---
    sf::RectangleShape cardSim({leftSidebarWidth_ - 20.0f, 130.0f});
    cardSim.setPosition({10.0f, curY});
    cardSim.setFillColor(UITheme::CardSurface);
    cardSim.setOutlineColor(UITheme::PanelBorderSubtle);
    cardSim.setOutlineThickness(1.0f);
    window_.draw(cardSim);

    sf::Text simTitle(fontUi_, "SIMULATION FLOW", 10);
    simTitle.setFillColor(UITheme::TextMuted);
    simTitle.setPosition({16.0f, curY + 6.0f});
    window_.draw(simTitle);

    // Flow buttons: Run/Pause, Step, Reset
    float btnRowY = curY + 24.0f;
    registerButton({{16.0f, btnRowY}, {66.0f, 26.0f}}, "sim:pause_toggle", "Space", "Play or pause the simulation");
    registerButton({{86.0f, btnRowY}, {66.0f, 26.0f}}, "sim:step_once", "S Step", "Advance exactly one simulation tick");
    registerButton({{156.0f, btnRowY}, {66.0f, 26.0f}}, "sim:reset", "R Reset", "Reset simulation world and organisms");

    // Speed multiplier buttons [0.5x] [1x] [2x] [5x] [10x]
    sf::Text speedTitle(fontUi_, "SIMULATION SPEED", 10);
    speedTitle.setFillColor(UITheme::TextMuted);
    speedTitle.setPosition({16.0f, curY + 58.0f});
    window_.draw(speedTitle);

    float spdY = curY + 76.0f;
    float spdW = 40.0f;
    registerButton({{16.0f, spdY}, {spdW, 24.0f}}, "speed:0.5", "0.5x", "Slow motion");
    registerButton({{58.0f, spdY}, {spdW, 24.0f}}, "speed:1.0", "1x", "Real-time physics (60 TPS)");
    registerButton({{100.0f, spdY}, {spdW, 24.0f}}, "speed:2.0", "2x", "2x Accelerated");
    registerButton({{142.0f, spdY}, {spdW, 24.0f}}, "speed:5.0", "5x", "5x Fast");
    registerButton({{184.0f, spdY}, {spdW, 24.0f}}, "speed:10.0", "10x", "10x Ultra-fast");

    curY += 140.0f;

    // --- CARD 2: EXPERIMENT PRESETS ---
    sf::RectangleShape cardPreset({leftSidebarWidth_ - 20.0f, 220.0f});
    cardPreset.setPosition({10.0f, curY});
    cardPreset.setFillColor(UITheme::CardSurface);
    cardPreset.setOutlineColor(UITheme::PanelBorderSubtle);
    cardPreset.setOutlineThickness(1.0f);
    window_.draw(cardPreset);

    sf::Text preTitle(fontUi_, "EXPERIMENT PRESETS (P)", 10);
    preTitle.setFillColor(UITheme::TextMuted);
    preTitle.setPosition({16.0f, curY + 6.0f});
    window_.draw(preTitle);

    const std::vector<std::pair<std::string, std::string>> presetDefs = {
        {"preset:basic", "Basic Evolution"},
        {"preset:scarcity", "Resource Scarcity"},
        {"preset:repro", "Rapid Reproduction"},
        {"preset:mutation", "High Mutation"},
        {"preset:predator", "Predator-Prey"},
        {"preset:neural", "Neural Behaviour"},
        {"preset:custom", "Custom Sandbox"}
    };

    float pBtnY = curY + 24.0f;
    for (const auto& [id, name] : presetDefs) {
        bool isCurrent = (currentPresetName_.find(name) != std::string::npos ||
                          (name == "Predator-Prey" && currentPresetName_.find("Predator") != std::string::npos));
        registerButton({{16.0f, pBtnY}, {leftSidebarWidth_ - 32.0f, 24.0f}}, id, name, "Apply " + name + " preset", isCurrent);
        pBtnY += 27.0f;
    }

    curY += 230.0f;

    // --- CARD 3: VIEW LAYERS & CAMERA ---
    sf::RectangleShape cardView({leftSidebarWidth_ - 20.0f, 150.0f});
    cardView.setPosition({10.0f, curY});
    cardView.setFillColor(UITheme::CardSurface);
    cardView.setOutlineColor(UITheme::PanelBorderSubtle);
    cardView.setOutlineThickness(1.0f);
    window_.draw(cardView);

    sf::Text viewTitle(fontUi_, "VIEWPORT & LAYERS", 10);
    viewTitle.setFillColor(UITheme::TextMuted);
    viewTitle.setPosition({16.0f, curY + 6.0f});
    window_.draw(viewTitle);

    float layerY = curY + 24.0f;
    registerButton({{16.0f, layerY}, {96.0f, 22.0f}}, "layer:grid", (showGrid_ ? "[x] Grid" : "[ ] Grid"), "Toggle spatial partition grid (G)");
    registerButton({{118.0f, layerY}, {104.0f, 22.0f}}, "layer:organisms", (showOrganisms_ ? "[x] Organisms" : "[ ] Organisms"), "Toggle organism entities");

    layerY += 26.0f;
    registerButton({{16.0f, layerY}, {96.0f, 22.0f}}, "layer:food", (showFood_ ? "[x] Food" : "[ ] Food"), "Toggle food nutrient pellets");
    registerButton({{118.0f, layerY}, {104.0f, 22.0f}}, "layer:headings", (showHeadings_ ? "[x] Headings" : "[ ] Headings"), "Toggle direction vectors");

    layerY += 26.0f;
    registerButton({{16.0f, layerY}, {96.0f, 22.0f}}, "layer:rays", (showSensoryRays_ ? "[x] Rays" : "[ ] Rays"), "Toggle sensory target/threat lines");
    registerButton({{118.0f, layerY}, {104.0f, 22.0f}}, "layer:follow", (followSelected_ ? "[x] Follow (F)" : "[ ] Follow (F)"), "Camera tracks selected organism");

    layerY += 28.0f;
    registerButton({{16.0f, layerY}, {leftSidebarWidth_ - 32.0f, 24.0f}}, "cam:reset", "Center Viewport (Home)", "Reset camera position and zoom");

    // Render registered buttons for the sidebar
    for (const auto& btn : buttons_) {
        if (btn.bounds.position.x < leftSidebarWidth_ && btn.bounds.position.y >= topBarHeight_) {
            sf::RectangleShape r(btn.bounds.size);
            r.setPosition(btn.bounds.position);
            r.setFillColor(btn.isSelected ? UITheme::ButtonActive : (btn.isHovered ? UITheme::ButtonHover : UITheme::ButtonDefault));
            r.setOutlineColor(btn.isSelected ? UITheme::AccentBlue : (btn.isHovered ? UITheme::TextSecondary : UITheme::ButtonBorder));
            r.setOutlineThickness(1.0f);
            window_.draw(r);

            sf::Text t(fontUi_, btn.label, 10);
            t.setFillColor(btn.isSelected ? UITheme::AccentBlue : (btn.isHovered ? UITheme::TextPrimary : UITheme::TextSecondary));
            t.setPosition({btn.bounds.position.x + 8.0f, btn.bounds.position.y + 3.0f});
            window_.draw(t);
        }
    }
}

void Renderer::renderRightSidebar(const sim::World& world,
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
                                  float regenMultiplier) {
    window_.setView(uiView_);
    float winW = uiView_.getSize().x;
    float winH = uiView_.getSize().y;

    if (!rightSidebarOpen_) {
        // Collapsed mini-tab
        sf::FloatRect toggleBounds{{winW - 28.0f, topBarHeight_ + 10.0f}, {28.0f, 80.0f}};
        registerButton(toggleBounds, "toggle:right_sidebar", "<", "Expand Telemetry Sidebar");

        sf::RectangleShape tab(toggleBounds.size);
        tab.setPosition(toggleBounds.position);
        tab.setFillColor(UITheme::PanelBackground);
        tab.setOutlineColor(UITheme::PanelBorder);
        tab.setOutlineThickness(1.0f);
        window_.draw(tab);

        if (fontLoaded_) {
            sf::Text t(fontUi_, "<", 14);
            t.setFillColor(UITheme::AccentBlue);
            t.setPosition({toggleBounds.position.x + 8.0f, toggleBounds.position.y + 30.0f});
            window_.draw(t);
        }
        return;
    }

    float barX = winW - rightSidebarWidth_;
    float barH = winH - topBarHeight_ - (bottomPanelOpen_ ? bottomPanelHeight_ : 0.0f);

    sf::RectangleShape sidebarBg({rightSidebarWidth_, barH});
    sidebarBg.setPosition({barX, topBarHeight_});
    sidebarBg.setFillColor(UITheme::PanelBackground);
    sidebarBg.setOutlineColor(UITheme::PanelBorder);
    sidebarBg.setOutlineThickness(1.0f);
    window_.draw(sidebarBg);

    if (!fontLoaded_) return;

    // Sidebar Header
    sf::Text header(fontUi_, "OBSERVATION & TELEMETRY", 12);
    header.setFillColor(UITheme::TextPrimary);
    header.setStyle(sf::Text::Bold);
    header.setPosition({barX + 14.0f, topBarHeight_ + 10.0f});
    window_.draw(header);

    // Collapse Button [ > ]
    sf::FloatRect collapseBtnBounds{{winW - 30.0f, topBarHeight_ + 8.0f}, {22.0f, 20.0f}};
    registerButton(collapseBtnBounds, "toggle:right_sidebar", ">", "Collapse Sidebar");
    sf::RectangleShape cBtn(collapseBtnBounds.size);
    cBtn.setPosition(collapseBtnBounds.position);
    cBtn.setFillColor(UITheme::ButtonDefault);
    cBtn.setOutlineColor(UITheme::ButtonBorder);
    cBtn.setOutlineThickness(1.0f);
    window_.draw(cBtn);

    sf::Text cText(fontUi_, ">", 11);
    cText.setFillColor(UITheme::TextSecondary);
    cText.setPosition({collapseBtnBounds.position.x + 6.0f, collapseBtnBounds.position.y + 2.0f});
    window_.draw(cText);

    float curY = topBarHeight_ + 36.0f;
    float cardW = rightSidebarWidth_ - 20.0f;

    // --- CARD 1: ECOSYSTEM OVERVIEW ---
    sf::RectangleShape cardEco({cardW, 140.0f});
    cardEco.setPosition({barX + 10.0f, curY});
    cardEco.setFillColor(UITheme::CardSurface);
    cardEco.setOutlineColor(UITheme::PanelBorderSubtle);
    cardEco.setOutlineThickness(1.0f);
    window_.draw(cardEco);

    sf::Text ecoTitle(fontUi_, "ECOSYSTEM CARRYING CAPACITY", 10);
    ecoTitle.setFillColor(UITheme::TextMuted);
    ecoTitle.setPosition({barX + 16.0f, curY + 6.0f});
    window_.draw(ecoTitle);

    // Population numbers
    std::ostringstream ssPop;
    ssPop << "Population: " << population << " living  |  " << totalBirths << " births  |  " << totalDeaths << " dead";
    sf::Text popText(fontUi_, ssPop.str(), 11);
    popText.setFillColor(UITheme::TextPrimary);
    popText.setPosition({barX + 16.0f, curY + 22.0f});
    window_.draw(popText);

    // Trophic Ratio Bar: Green (Herbivores) vs Red (Predators)
    float barProgressW = cardW - 12.0f;
    float barProgressH = 14.0f;
    float barProgressY = curY + 42.0f;

    sf::RectangleShape bgTrophicBar({barProgressW, barProgressH});
    bgTrophicBar.setPosition({barX + 16.0f, barProgressY});
    bgTrophicBar.setFillColor(sf::Color(30, 40, 55));
    window_.draw(bgTrophicBar);

    float herbFrac = population > 0 ? static_cast<float>(herbivoreCount) / static_cast<float>(population) : 0.0f;
    if (herbFrac > 0.0f) {
        sf::RectangleShape herbBar({barProgressW * herbFrac, barProgressH});
        herbBar.setPosition({barX + 16.0f, barProgressY});
        herbBar.setFillColor(UITheme::HerbivoreGreen);
        window_.draw(herbBar);
    }
    if (1.0f - herbFrac > 0.0f) {
        sf::RectangleShape predBar({barProgressW * (1.0f - herbFrac), barProgressH});
        predBar.setPosition({barX + 16.0f + barProgressW * herbFrac, barProgressY});
        predBar.setFillColor(UITheme::PredatorRed);
        window_.draw(predBar);
    }

    // Species Labels & counts below bar
    std::ostringstream ssSpecies;
    ssSpecies << "Herbivores: " << herbivoreCount << " (" << std::fixed << std::setprecision(0) << (herbFrac * 100.0f) << "%)   "
              << "Predators: " << predatorCount << " (" << std::setprecision(0) << ((1.0f - herbFrac) * 100.0f) << "%)";
    sf::Text spText(fontUi_, ssSpecies.str(), 10);
    spText.setFillColor(UITheme::TextSecondary);
    spText.setPosition({barX + 16.0f, barProgressY + 18.0f});
    window_.draw(spText);

    // Food Resources & Energy
    std::ostringstream ssRes;
    ssRes << "Food Resources: " << foodCount << " / " << config_.maxFoodCapacity
          << "  |  Eaten: " << world.getFoodManager().getTotalFoodConsumed() << "\n"
          << "Avg Energy: " << std::fixed << std::setprecision(1) << avgEnergy << "  |  "
          << "Avg Lifespan: " << std::setprecision(1) << avgLifespan << "s\n"
          << "Climate: " << seasonName << " (" << std::setprecision(2) << regenMultiplier << "x food regen)";
    sf::Text resText(fontUi_, ssRes.str(), 10);
    resText.setFillColor(UITheme::TextSecondary);
    resText.setPosition({barX + 16.0f, barProgressY + 36.0f});
    window_.draw(resText);

    curY += 150.0f;

    // --- CARD 2: ORGANISM INSPECTOR ---
    const entities::Organism* inspectedOrg = nullptr;
    if (selectedOrganismId_.has_value()) {
        for (const auto& org : world.getPopulation().getOrganisms()) {
            if (org.getId() == *selectedOrganismId_ && org.isAlive()) {
                inspectedOrg = &org;
                break;
            }
        }
    }

    float inspH = inspectedOrg ? 200.0f : 90.0f;
    sf::RectangleShape cardInsp({cardW, inspH});
    cardInsp.setPosition({barX + 10.0f, curY});
    cardInsp.setFillColor(UITheme::CardSurface);
    cardInsp.setOutlineColor(inspectedOrg ? (inspectedOrg->isPredator() ? UITheme::PredatorRed : UITheme::HerbivoreGreen) : UITheme::PanelBorderSubtle);
    cardInsp.setOutlineThickness(1.0f);
    window_.draw(cardInsp);

    if (inspectedOrg) {
        // Detailed inspector
        std::string role = inspectedOrg->isPredator() ? "PREDATOR" : "HERBIVORE";
        sf::Text inspTitle(fontUi_, role + " #" + std::to_string(inspectedOrg->getId()), 12);
        inspTitle.setFillColor(inspectedOrg->isPredator() ? UITheme::PredatorRed : UITheme::HerbivoreGreen);
        inspTitle.setStyle(sf::Text::Bold);
        inspTitle.setPosition({barX + 16.0f, curY + 8.0f});
        window_.draw(inspTitle);

        std::ostringstream ssMeta;
        ssMeta << "Generation: " << inspectedOrg->getGeneration() << "  |  Parent: #" << inspectedOrg->getParentId()
               << "  |  Cooldown: " << std::fixed << std::setprecision(1) << inspectedOrg->getReproductionCooldown() << "s";
        sf::Text metaText(fontUi_, ssMeta.str(), 10);
        metaText.setFillColor(UITheme::TextSecondary);
        metaText.setPosition({barX + 16.0f, curY + 26.0f});
        window_.draw(metaText);

        // Energy Bar
        float energyRatio = inspectedOrg->getMaxEnergy() > 0.0f ? (inspectedOrg->getEnergy() / inspectedOrg->getMaxEnergy()) : 0.0f;
        energyRatio = std::clamp(energyRatio, 0.0f, 1.0f);

        float eBarY = curY + 44.0f;
        sf::RectangleShape eBarBg({cardW - 12.0f, 12.0f});
        eBarBg.setPosition({barX + 16.0f, eBarY});
        eBarBg.setFillColor(sf::Color(30, 40, 55));
        window_.draw(eBarBg);

        sf::Color eColor = energyRatio > 0.5f ? UITheme::HerbivoreGreen : (energyRatio > 0.25f ? UITheme::FoodAmber : UITheme::PredatorRed);
        sf::RectangleShape eBarFill({(cardW - 12.0f) * energyRatio, 12.0f});
        eBarFill.setPosition({barX + 16.0f, eBarY});
        eBarFill.setFillColor(eColor);
        window_.draw(eBarFill);

        std::ostringstream ssEnergy;
        ssEnergy << "Energy: " << std::fixed << std::setprecision(1) << inspectedOrg->getEnergy()
                 << " / " << inspectedOrg->getMaxEnergy() << " (" << std::setprecision(0) << (energyRatio * 100.0f) << "%)";
        sf::Text eText(fontUi_, ssEnergy.str(), 10);
        eText.setFillColor(UITheme::TextPrimary);
        eText.setPosition({barX + 16.0f, eBarY + 14.0f});
        window_.draw(eText);

        // Heritable Traits
        std::ostringstream ssTraits;
        ssTraits << "Heritable Traits:\n"
                 << "  Speed: " << std::setprecision(1) << inspectedOrg->getSpeed() << " px/s  |  "
                 << "Basal Burn: " << std::setprecision(2) << inspectedOrg->getTraits().basalMetabolismCost << " e/s\n"
                 << "  Sense Radius: " << std::setprecision(1) << inspectedOrg->getTraits().foodInteractionRadius << " px";
        sf::Text traitsText(fontUi_, ssTraits.str(), 10);
        traitsText.setFillColor(UITheme::TextSecondary);
        traitsText.setPosition({barX + 16.0f, eBarY + 30.0f});
        window_.draw(traitsText);

        // Neural Controller Decisions
        const auto& outputs = inspectedOrg->getLastOutputs();
        float turnVal = outputs.size() >= 1 ? outputs[0] : 0.0f;
        float throttleVal = outputs.size() >= 2 ? outputs[1] : 0.0f;

        std::ostringstream ssBrain;
        ssBrain << "Neural Network Decisions (tanh):\n"
                << "  Steering: " << std::setprecision(2) << turnVal << " (" << (turnVal >= 0.0f ? "Right" : "Left") << ")  |  "
                << "Throttle: " << std::setprecision(2) << throttleVal;
        sf::Text brainText(fontUi_, ssBrain.str(), 10);
        brainText.setFillColor(UITheme::AccentBlue);
        brainText.setPosition({barX + 16.0f, eBarY + 76.0f});
        window_.draw(brainText);

        // Action buttons: Follow, Deselect
        float actY = curY + 165.0f;
        registerButton({{barX + 16.0f, actY}, {130.0f, 24.0f}}, "org:follow", (followSelected_ ? "Follow Camera [ON]" : "Follow Camera [OFF]"), "Track this organism as it moves (F)");
        registerButton({{barX + 155.0f, actY}, {100.0f, 24.0f}}, "org:deselect", "Deselect", "Clear current inspection focus");
    } else {
        // No organism selected or deceased
        if (selectedOrganismId_.has_value()) {
            sf::Text msg(fontUi_, "ORGANISM DECEASED\nOrganism #" + std::to_string(*selectedOrganismId_) + " has died.\nVital lineage history preserved.", 10);
            msg.setFillColor(UITheme::PredatorRed);
            msg.setPosition({barX + 16.0f, curY + 12.0f});
            window_.draw(msg);
        } else {
            sf::Text msg(fontUi_, "ORGANISM INSPECTOR\nClick any organism in the arena\nto inspect heritable traits, sensors,\nand neural network decisions.", 10);
            msg.setFillColor(UITheme::TextMuted);
            msg.setPosition({barX + 16.0f, curY + 12.0f});
            window_.draw(msg);
        }
    }

    curY += inspH + 10.0f;

    // --- CARD 3: REAL-TIME ECOSYSTEM EVENT FEED ---
    float feedH = std::max(120.0f, barH - (curY - topBarHeight_) - 10.0f);
    sf::RectangleShape cardFeed({cardW, feedH});
    cardFeed.setPosition({barX + 10.0f, curY});
    cardFeed.setFillColor(UITheme::CardSurface);
    cardFeed.setOutlineColor(UITheme::PanelBorderSubtle);
    cardFeed.setOutlineThickness(1.0f);
    window_.draw(cardFeed);

    sf::Text feedTitle(fontUi_, "LIVE ECOSYSTEM EVENT FEED", 10);
    feedTitle.setFillColor(UITheme::TextMuted);
    feedTitle.setPosition({barX + 16.0f, curY + 6.0f});
    window_.draw(feedTitle);

    const auto& events = eventLogger.getEvents();
    float itemY = curY + 24.0f;
    size_t count = std::min(events.size(), size_t(7));

    for (size_t i = 0; i < count; ++i) {
        if (itemY + 14.0f > curY + feedH) break;

        const auto& ev = events[events.size() - 1 - i];

        sf::Color typeColor = UITheme::TextSecondary;
        std::string typeBadge = "EVENT";
        switch (ev.type) {
            case data::EventType::Predation:
                typeColor = UITheme::PredatorRed;
                typeBadge = "PREDATION";
                break;
            case data::EventType::Birth:
                typeColor = UITheme::HerbivoreGreen;
                typeBadge = "BIRTH";
                break;
            case data::EventType::Reproduction:
                typeColor = UITheme::AccentBlue;
                typeBadge = "REPRO";
                break;
            case data::EventType::Death:
                typeColor = sf::Color(180, 140, 140);
                typeBadge = "DEATH";
                break;
            case data::EventType::Extinction:
                typeColor = UITheme::StatusExtinct;
                typeBadge = "EXTINCT";
                break;
            default:
                break;
        }

        std::ostringstream ssEv;
        ssEv << "[" << std::fixed << std::setprecision(1) << ev.timestamp << "s] "
             << "[" << typeBadge << "] ";

        if (ev.type == data::EventType::Predation) {
            ssEv << "Predator #" << ev.primaryId << " caught Prey #" << ev.secondaryId;
        } else if (ev.type == data::EventType::Birth) {
            ssEv << (ev.speciesId == biology::SpeciesId::Predator ? "Predator #" : "Herbivore #")
                 << ev.primaryId << " born";
        } else if (ev.type == data::EventType::Reproduction) {
            ssEv << "Organism #" << ev.primaryId << " reproduced";
        } else if (ev.type == data::EventType::Death) {
            ssEv << "Organism #" << ev.primaryId << " died (" << ev.details << ")";
        } else {
            ssEv << ev.details;
        }

        sf::Text evText(fontUi_, ssEv.str(), 9);
        evText.setFillColor(typeColor);
        evText.setPosition({barX + 16.0f, itemY});
        window_.draw(evText);

        itemY += 16.0f;
    }

    // Render registered buttons for the right sidebar
    for (const auto& btn : buttons_) {
        if (btn.bounds.position.x >= barX && btn.bounds.position.y >= topBarHeight_) {
            sf::RectangleShape r(btn.bounds.size);
            r.setPosition(btn.bounds.position);
            r.setFillColor(btn.isSelected ? UITheme::ButtonActive : (btn.isHovered ? UITheme::ButtonHover : UITheme::ButtonDefault));
            r.setOutlineColor(btn.isSelected ? UITheme::AccentBlue : (btn.isHovered ? UITheme::TextSecondary : UITheme::ButtonBorder));
            r.setOutlineThickness(1.0f);
            window_.draw(r);

            sf::Text t(fontUi_, btn.label, 10);
            t.setFillColor(btn.isSelected ? UITheme::AccentBlue : (btn.isHovered ? UITheme::TextPrimary : UITheme::TextSecondary));
            t.setPosition({btn.bounds.position.x + 8.0f, btn.bounds.position.y + 4.0f});
            window_.draw(t);
        }
    }
}

void Renderer::renderBottomPanel(const std::vector<biology::EvolutionSnapshot>& snapshots,
                                 const data::EventLogger& eventLogger) {
    window_.setView(uiView_);
    float winW = uiView_.getSize().x;
    float winH = uiView_.getSize().y;

    if (!bottomPanelOpen_) {
        // Collapsed mini-tab
        float tabW = 200.0f;
        float tabX = (winW - tabW) * 0.5f;
        sf::FloatRect toggleBounds{{tabX, winH - 24.0f}, {tabW, 24.0f}};
        registerButton(toggleBounds, "toggle:bottom_panel", "^ Expand Charts & History", "Open scientific telemetry charts");

        sf::RectangleShape tab(toggleBounds.size);
        tab.setPosition(toggleBounds.position);
        tab.setFillColor(UITheme::PanelBackground);
        tab.setOutlineColor(UITheme::PanelBorder);
        tab.setOutlineThickness(1.0f);
        window_.draw(tab);

        if (fontLoaded_) {
            sf::Text t(fontUi_, "^ Scientific Charts & History", 10);
            t.setFillColor(UITheme::AccentBlue);
            t.setPosition({tabX + 16.0f, winH - 20.0f});
            window_.draw(t);
        }
        return;
    }

    float left = leftSidebarOpen_ ? leftSidebarWidth_ : 0.0f;
    float right = rightSidebarOpen_ ? rightSidebarWidth_ : 0.0f;
    float panelW = winW - left - right;
    float panelX = left;
    float panelY = winH - bottomPanelHeight_;

    sf::RectangleShape panelBg({panelW, bottomPanelHeight_});
    panelBg.setPosition({panelX, panelY});
    panelBg.setFillColor(UITheme::PanelBackground);
    panelBg.setOutlineColor(UITheme::PanelBorder);
    panelBg.setOutlineThickness(1.0f);
    window_.draw(panelBg);

    if (!fontLoaded_) return;

    // --- TAB BAR ---
    float tabY = panelY + 4.0f;
    float tabH = 24.0f;
    registerButton({{panelX + 10.0f, tabY}, {150.0f, tabH}}, "tab:pop_chart", "Pop & Food History", "Population dynamics time-series chart", activeBottomTab_ == BottomTab::PopulationChart);
    registerButton({{panelX + 165.0f, tabY}, {150.0f, tabH}}, "tab:trait_chart", "Trait Evolution", "Heritable speed and diversity trends", activeBottomTab_ == BottomTab::TraitEvolution);
    registerButton({{panelX + 320.0f, tabY}, {130.0f, tabH}}, "tab:events", "Event Archive", "Historical event log", activeBottomTab_ == BottomTab::EventArchive);
    registerButton({{panelX + 455.0f, tabY}, {130.0f, tabH}}, "tab:guide", "Lab Guide & Help", "Scientific concepts & help guide", activeBottomTab_ == BottomTab::LabGuide);

    // Collapse Button [ v Hide ]
    registerButton({{panelX + panelW - 80.0f, tabY}, {70.0f, tabH}}, "toggle:bottom_panel", "v Hide", "Collapse analysis panel");

    // Render tab buttons
    for (const auto& btn : buttons_) {
        if (btn.bounds.position.y >= panelY && btn.bounds.position.y < panelY + 30.0f) {
            sf::RectangleShape r(btn.bounds.size);
            r.setPosition(btn.bounds.position);
            r.setFillColor(btn.isSelected ? UITheme::ButtonActive : (btn.isHovered ? UITheme::ButtonHover : UITheme::ButtonDefault));
            r.setOutlineColor(btn.isSelected ? UITheme::AccentBlue : (btn.isHovered ? UITheme::TextSecondary : UITheme::ButtonBorder));
            r.setOutlineThickness(1.0f);
            window_.draw(r);

            sf::Text t(fontUi_, btn.label, 10);
            t.setFillColor(btn.isSelected ? UITheme::AccentBlue : (btn.isHovered ? UITheme::TextPrimary : UITheme::TextSecondary));
            t.setPosition({btn.bounds.position.x + 8.0f, btn.bounds.position.y + 4.0f});
            window_.draw(t);
        }
    }

    // --- TAB CONTENT ---
    float contentY = panelY + 34.0f;
    float contentH = bottomPanelHeight_ - 40.0f;

    if (activeBottomTab_ == BottomTab::PopulationChart) {
        // TAB 1: Population & Resources Time-Series Chart
        float plotX = panelX + 50.0f;
        float plotY = contentY + 16.0f;
        float plotW = panelW - 90.0f;
        float plotH = contentH - 35.0f;

        // Draw axes
        sf::Vertex axes[4] = {
            sf::Vertex{sf::Vector2f{plotX, plotY}, UITheme::PanelBorder},
            sf::Vertex{sf::Vector2f{plotX, plotY + plotH}, UITheme::PanelBorder},
            sf::Vertex{sf::Vector2f{plotX, plotY + plotH}, UITheme::PanelBorder},
            sf::Vertex{sf::Vector2f{plotX + plotW, plotY + plotH}, UITheme::PanelBorder}
        };
        window_.draw(axes, 4, sf::PrimitiveType::Lines);

        // Legend
        sf::Text legend(fontUi_, "Green: Herbivore Population  |  Red: Predator Population  |  Amber: Food Pellets", 10);
        legend.setFillColor(UITheme::TextSecondary);
        legend.setPosition({plotX + 10.0f, contentY});
        window_.draw(legend);

        if (snapshots.size() >= 2) {
            size_t count = std::min(snapshots.size(), size_t(300));
            size_t startIdx = snapshots.size() - count;

            size_t maxVal = 10;
            for (size_t i = startIdx; i < snapshots.size(); ++i) {
                maxVal = std::max(maxVal, snapshots[i].herbivorePopulation);
                maxVal = std::max(maxVal, snapshots[i].predatorPopulation);
                maxVal = std::max(maxVal, snapshots[i].foodCount);
            }

            sf::Text maxLabel(fontUi_, std::to_string(maxVal), 9);
            maxLabel.setFillColor(UITheme::TextMuted);
            maxLabel.setPosition({plotX - 35.0f, plotY - 4.0f});
            window_.draw(maxLabel);

            sf::Text zeroLabel(fontUi_, "0", 9);
            zeroLabel.setFillColor(UITheme::TextMuted);
            zeroLabel.setPosition({plotX - 16.0f, plotY + plotH - 8.0f});
            window_.draw(zeroLabel);

            chartBatch_.clear();
            auto mapPt = [&](size_t idx, float val) -> sf::Vector2f {
                float normX = static_cast<float>(idx - startIdx) / static_cast<float>(count - 1);
                float normY = std::clamp(val / static_cast<float>(maxVal), 0.0f, 1.0f);
                return {plotX + normX * plotW, (plotY + plotH) - normY * plotH};
            };

            for (size_t i = startIdx; i + 1 < snapshots.size(); ++i) {
                chartBatch_.emplace_back(mapPt(i, static_cast<float>(snapshots[i].herbivorePopulation)), UITheme::HerbivoreGreen);
                chartBatch_.emplace_back(mapPt(i + 1, static_cast<float>(snapshots[i + 1].herbivorePopulation)), UITheme::HerbivoreGreen);

                chartBatch_.emplace_back(mapPt(i, static_cast<float>(snapshots[i].predatorPopulation)), UITheme::PredatorRed);
                chartBatch_.emplace_back(mapPt(i + 1, static_cast<float>(snapshots[i + 1].predatorPopulation)), UITheme::PredatorRed);

                chartBatch_.emplace_back(mapPt(i, static_cast<float>(snapshots[i].foodCount)), UITheme::FoodAmber);
                chartBatch_.emplace_back(mapPt(i + 1, static_cast<float>(snapshots[i + 1].foodCount)), UITheme::FoodAmber);
            }

            if (!chartBatch_.empty()) {
                window_.draw(chartBatch_.data(), chartBatch_.size(), sf::PrimitiveType::Lines);
            }
        } else {
            sf::Text waiting(fontUi_, "Collecting real-time population telemetry... (advance simulation to plot history)", 11);
            waiting.setFillColor(UITheme::TextMuted);
            waiting.setPosition({plotX + 40.0f, plotY + 40.0f});
            window_.draw(waiting);
        }
    } else if (activeBottomTab_ == BottomTab::TraitEvolution) {
        // TAB 2: Trait Evolution & Diversity
        float plotX = panelX + 50.0f;
        float plotY = contentY + 16.0f;
        float plotW = panelW - 90.0f;
        float plotH = contentH - 35.0f;

        sf::Vertex axes[4] = {
            sf::Vertex{sf::Vector2f{plotX, plotY}, UITheme::PanelBorder},
            sf::Vertex{sf::Vector2f{plotX, plotY + plotH}, UITheme::PanelBorder},
            sf::Vertex{sf::Vector2f{plotX, plotY + plotH}, UITheme::PanelBorder},
            sf::Vertex{sf::Vector2f{plotX + plotW, plotY + plotH}, UITheme::PanelBorder}
        };
        window_.draw(axes, 4, sf::PrimitiveType::Lines);

        sf::Text legend(fontUi_, "Cyan: Mean Movement Speed (px/s)  |  Purple: Trait Diversity Coefficient", 10);
        legend.setFillColor(UITheme::TextSecondary);
        legend.setPosition({plotX + 10.0f, contentY});
        window_.draw(legend);

        if (snapshots.size() >= 2) {
            size_t count = std::min(snapshots.size(), size_t(300));
            size_t startIdx = snapshots.size() - count;

            chartBatch_.clear();
            for (size_t i = startIdx; i + 1 < snapshots.size(); ++i) {
                float normX1 = static_cast<float>(i - startIdx) / static_cast<float>(count - 1);
                float normX2 = static_cast<float>(i + 1 - startIdx) / static_cast<float>(count - 1);

                // Speed scaled [0..120]
                float spd1 = std::clamp(snapshots[i].speed.mean / 120.0f, 0.0f, 1.0f);
                float spd2 = std::clamp(snapshots[i + 1].speed.mean / 120.0f, 0.0f, 1.0f);
                chartBatch_.emplace_back(sf::Vector2f{plotX + normX1 * plotW, (plotY + plotH) - spd1 * plotH}, UITheme::TextAccent);
                chartBatch_.emplace_back(sf::Vector2f{plotX + normX2 * plotW, (plotY + plotH) - spd2 * plotH}, UITheme::TextAccent);

                // Diversity scaled [0..0.5]
                float div1 = std::clamp(snapshots[i].traitDiversity / 0.5f, 0.0f, 1.0f);
                float div2 = std::clamp(snapshots[i + 1].traitDiversity / 0.5f, 0.0f, 1.0f);
                chartBatch_.emplace_back(sf::Vector2f{plotX + normX1 * plotW, (plotY + plotH) - div1 * plotH}, sf::Color(190, 110, 245));
                chartBatch_.emplace_back(sf::Vector2f{plotX + normX2 * plotW, (plotY + plotH) - div2 * plotH}, sf::Color(190, 110, 245));
            }

            if (!chartBatch_.empty()) {
                window_.draw(chartBatch_.data(), chartBatch_.size(), sf::PrimitiveType::Lines);
            }
        }
    } else if (activeBottomTab_ == BottomTab::EventArchive) {
        // TAB 3: Event Archive Table
        const auto& events = eventLogger.getEvents();
        float listY = contentY + 6.0f;
        size_t count = std::min(events.size(), size_t(9));

        for (size_t i = 0; i < count; ++i) {
            const auto& ev = events[events.size() - 1 - i];
            std::ostringstream ss;
            ss << "[" << std::fixed << std::setprecision(1) << ev.timestamp << "s]  Step " << ev.stepCount
               << " | " << data::eventTypeToString(ev.type)
               << " | Primary: #" << ev.primaryId
               << (ev.secondaryId > 0 ? (" -> Target: #" + std::to_string(ev.secondaryId)) : "")
               << " | " << ev.details;

            sf::Text t(fontUi_, ss.str(), 10);
            t.setFillColor(UITheme::TextSecondary);
            t.setPosition({panelX + 20.0f, listY});
            window_.draw(t);

            listY += 16.0f;
        }
    } else if (activeBottomTab_ == BottomTab::LabGuide) {
        // TAB 4: Educational Guide & Concepts
        float cardW = (panelW - 50.0f) * 0.25f;
        auto drawGuideCard = [&](float x, const std::string& title, const std::string& desc, sf::Color accent) {
            sf::RectangleShape c({cardW, contentH - 8.0f});
            c.setPosition({x, contentY + 2.0f});
            c.setFillColor(UITheme::CardSurface);
            c.setOutlineColor(accent);
            c.setOutlineThickness(1.0f);
            window_.draw(c);

            sf::Text t(fontUi_, title, 11);
            t.setFillColor(accent);
            t.setStyle(sf::Text::Bold);
            t.setPosition({x + 8.0f, contentY + 8.0f});
            window_.draw(t);

            sf::Text d(fontUi_, desc, 9);
            d.setFillColor(UITheme::TextSecondary);
            d.setPosition({x + 8.0f, contentY + 26.0f});
            window_.draw(d);
        };

        drawGuideCard(panelX + 10.0f, "NATURAL SELECTION",
                      "Faster organisms catch food\nmore easily but burn more\nmetabolic energy per second.\nSelection discovers balanced\nenergy-efficient speeds.", UITheme::HerbivoreGreen);

        drawGuideCard(panelX + 16.0f + cardW, "NEURAL CONTROLLERS",
                      "Each organism has a neural net\nwith sensor inputs for food/prey\nand predators. Tanh outputs\nsteer turning angle and speed\nthrottle directly.", UITheme::AccentBlue);

        drawGuideCard(panelX + 22.0f + cardW * 2.0f, "PREDATOR-PREY BALANCE",
                      "Predators must capture prey\nto survive and reproduce.\nIf predators overpopulate,\nprey crash and predators starve,\nallowing prey recovery.", UITheme::PredatorRed);

        drawGuideCard(panelX + 28.0f + cardW * 3.0f, "CONTROLS REFERENCE",
                      "Left Click: Select organism\nRight Drag: Pan camera\nWheel: Zoom in / out\nSpace: Pause / Run\nP: Cycle presets | R: Reset", UITheme::FoodAmber);
    }
}

void Renderer::renderHoverTooltip(const sim::World& world) {
    if (!fontLoaded_) return;

    // 1. Organism Hover Tooltip
    if (hoveredOrganismId_.has_value()) {
        for (const auto& org : world.getPopulation().getOrganisms()) {
            if (org.getId() == *hoveredOrganismId_ && org.isAlive()) {
                window_.setView(uiView_);
                float tipW = 160.0f;
                float tipH = 54.0f;
                float tipX = currentMousePos_.x + 16.0f;
                float tipY = currentMousePos_.y + 16.0f;

                if (tipX + tipW > uiView_.getSize().x) tipX = currentMousePos_.x - tipW - 10.0f;
                if (tipY + tipH > uiView_.getSize().y) tipY = currentMousePos_.y - tipH - 10.0f;

                sf::RectangleShape tipBg({tipW, tipH});
                tipBg.setPosition({tipX, tipY});
                tipBg.setFillColor(sf::Color(15, 22, 32, 240));
                tipBg.setOutlineColor(org.isPredator() ? UITheme::PredatorRed : UITheme::HerbivoreGreen);
                tipBg.setOutlineThickness(1.0f);
                window_.draw(tipBg);

                std::ostringstream ss;
                ss << (org.isPredator() ? "[Predator #" : "[Herbivore #") << org.getId() << "] (Gen " << org.getGeneration() << ")\n"
                   << "Energy: " << std::fixed << std::setprecision(1) << org.getEnergy() << " / " << org.getMaxEnergy() << "\n"
                   << "Speed: " << std::setprecision(0) << org.getSpeed() << " px/s (Click)";

                sf::Text tipText(fontUi_, ss.str(), 10);
                tipText.setFillColor(UITheme::TextPrimary);
                tipText.setPosition({tipX + 8.0f, tipY + 6.0f});
                window_.draw(tipText);
                return;
            }
        }
    }

    // 2. Button Hover Tooltip
    if (!hoveredButtonTooltip_.empty()) {
        window_.setView(uiView_);
        sf::Text t(fontUi_, hoveredButtonTooltip_, 10);
        t.setFillColor(UITheme::TextPrimary);

        sf::FloatRect b = t.getLocalBounds();
        float tipW = b.size.x + 16.0f;
        float tipH = 24.0f;
        float tipX = currentMousePos_.x + 12.0f;
        float tipY = currentMousePos_.y + 12.0f;

        if (tipX + tipW > uiView_.getSize().x) tipX = currentMousePos_.x - tipW - 8.0f;
        if (tipY + tipH > uiView_.getSize().y) tipY = currentMousePos_.y - tipH - 8.0f;

        sf::RectangleShape tipBg({tipW, tipH});
        tipBg.setPosition({tipX, tipY});
        tipBg.setFillColor(sf::Color(15, 22, 32, 245));
        tipBg.setOutlineColor(UITheme::TextAccent);
        tipBg.setOutlineThickness(1.0f);
        window_.draw(tipBg);

        t.setPosition({tipX + 8.0f, tipY + 4.0f});
        window_.draw(t);
    }
}

void Renderer::renderToastNotifications() {
    if (notifications_.empty() || !fontLoaded_) return;

    window_.setView(uiView_);
    float uiWidth = uiView_.getSize().x;
    float startY = topBarHeight_ + 10.0f;

    for (const auto& notif : notifications_) {
        sf::Text toastText(fontUi_, notif.text, 12);
        toastText.setFillColor(notif.color);

        sf::FloatRect bounds = toastText.getLocalBounds();
        float panelWidth = bounds.size.x + 30.0f;
        float panelHeight = 28.0f;
        float panelX = (uiWidth - panelWidth) * 0.5f;

        sf::RectangleShape banner({panelWidth, panelHeight});
        banner.setPosition({panelX, startY});
        banner.setFillColor(sf::Color(15, 22, 32, 235));
        banner.setOutlineColor(notif.color);
        banner.setOutlineThickness(1.0f);
        window_.draw(banner);

        toastText.setPosition({panelX + 15.0f, startY + 5.0f});
        window_.draw(toastText);

        startY += panelHeight + 6.0f;
    }
}

} // namespace alife::render
