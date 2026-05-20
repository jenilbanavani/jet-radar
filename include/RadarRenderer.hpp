#pragma once

#include "RadarConfig.hpp"
#include "Simulation.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

class RadarRenderer {
public:
    explicit RadarRenderer(RadarConfig config);

    void render(sf::RenderWindow& window, const Simulation& simulation) const;

private:
    sf::Vector2f radarCenter() const;
    sf::Vector2f worldToRadar(sf::Vector2f worldPosition, sf::Vector2f playerPosition) const;
    bool isInsideRadarRange(sf::Vector2f worldPosition, sf::Vector2f playerPosition) const;

    RadarConfig m_config;
};
