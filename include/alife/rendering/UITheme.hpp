#pragma once

#include <SFML/Graphics/Color.hpp>

namespace alife::render {

/**
 * @brief Professional scientific laboratory UI theme tokens and color palette.
 * Carefully balanced for high contrast, legibility, and dark-mode aesthetic.
 */
struct UITheme {
    // Window & Canvas Backgrounds
    static constexpr sf::Color AppBackground{11, 16, 24};      // #0B1018 Deep navy charcoal
    static constexpr sf::Color ViewportBackground{14, 19, 28}; // #0E131C Arena background
    static constexpr sf::Color PanelBackground{17, 25, 37};    // #111925 Primary panel surface
    static constexpr sf::Color CardSurface{23, 33, 48};        // #172130 Elevated card surface
    static constexpr sf::Color PanelBorder{41, 54, 73};        // #293649 Subtle panel border
    static constexpr sf::Color PanelBorderSubtle{30, 42, 58};  // #1E2A3A Faint divider

    // Typography & Contrast
    static constexpr sf::Color TextPrimary{237, 243, 250};     // #EDF3FA High-contrast white
    static constexpr sf::Color TextSecondary{166, 180, 197};   // #A6B4C5 Readable soft grey
    static constexpr sf::Color TextMuted{115, 130, 150};       // #738296 Metadata / helper
    static constexpr sf::Color TextAccent{98, 169, 255};       // #62A9FF Active element

    // Ecological Species Palette (distinct from UI status colors)
    static constexpr sf::Color HerbivoreGreen{70, 215, 155};   // #46D79B Bio-emerald
    static constexpr sf::Color HerbivoreGlow{120, 245, 190};   // Highlight / ring
    static constexpr sf::Color PredatorRed{238, 75, 88};       // #EE4B58 Coral crimson
    static constexpr sf::Color PredatorGlow{255, 120, 130};    // Threat ring
    static constexpr sf::Color FoodAmber{245, 198, 75};        // #F5C64B Faceted nutrient gold
    static constexpr sf::Color FoodGlow{255, 235, 140};        // Core sparkle

    // Interface Status & Accents
    static constexpr sf::Color AccentBlue{85, 155, 245};       // #559BF5 Primary accent
    static constexpr sf::Color StatusRunning{60, 200, 140};    // Active running state
    static constexpr sf::Color StatusPaused{240, 180, 65};     // Paused warning state
    static constexpr sf::Color StatusExtinct{240, 70, 70};     // Extinction alert
    static constexpr sf::Color SelectionGold{255, 220, 60};    // Reticle & focus

    // Interactive Button States
    static constexpr sf::Color ButtonDefault{25, 36, 52};      // Normal state
    static constexpr sf::Color ButtonHover{38, 54, 78};        // Mouse hover
    static constexpr sf::Color ButtonActive{52, 75, 110};      // Mouse down / selected
    static constexpr sf::Color ButtonBorder{48, 65, 90};       // Outline

    // Grid & Overlays
    static constexpr sf::Color GridLine{26, 36, 50, 160};      // Spatial partition grid
    static constexpr sf::Color ArenaBorder{58, 78, 108};       // World boundary outline
    static constexpr sf::Color SensoryTarget{255, 225, 90, 200}; // Sensory ray to food/prey
    static constexpr sf::Color SensoryThreat{245, 90, 70, 220};  // Sensory ray to predator
    static constexpr sf::Color PerceptionArea{0, 180, 255, 22};  // Translucent vision circle
    static constexpr sf::Color PerceptionBorder{0, 200, 255, 90};
};

} // namespace alife::render
