#pragma once

#include "RadarConfig.hpp"
#include "Simulation.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>

class RadarRenderer {
public:
    explicit RadarRenderer(RadarConfig config);

    // Returns false if the font could not be loaded (non-fatal — text is skipped).
    bool loadFont(const std::string& path);

    void render(sf::RenderWindow& window, const Simulation& simulation) const;

private:
    // Sub-draw helpers
    void drawBackground(sf::RenderWindow& window) const;
    void drawSweepSector(sf::RenderWindow& window, float angleDeg) const;
    void drawPlayerBlip(sf::RenderWindow& window) const;
    void drawEnemyBlips(sf::RenderWindow& window, const Simulation& simulation) const;
    void drawDetectionPulses(sf::RenderWindow& window, const Simulation& simulation) const;
    void drawLockOnIndicator(sf::RenderWindow& window, const Simulation& simulation) const;
    void drawLockTimerArc(sf::RenderWindow& window, const Simulation& simulation) const;
    void drawMissileReadyIndicator(sf::RenderWindow& window, const Simulation& simulation) const;
    void drawHUD(sf::RenderWindow& window, const Simulation& simulation) const;

    sf::Vector2f radarCenter() const;
    sf::Vector2f worldToRadar(sf::Vector2f worldPos, sf::Vector2f playerPos) const;
    bool         isInsideRadarRange(sf::Vector2f worldPos, sf::Vector2f playerPos) const;
    float        distanceBetween(sf::Vector2f a, sf::Vector2f b) const;

    RadarConfig m_config;
    sf::Font    m_font;
    bool        m_fontLoaded = false;
};
