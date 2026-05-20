#pragma once

#include <SFML/Graphics/Color.hpp>

struct RadarConfig {
    unsigned int windowWidth = 900;
    unsigned int windowHeight = 900;
    float radarRadius = 330.0f;
    float radarRange = 1000.0f;
    float scanDegreesPerSecond = 95.0f;
    float enemyBlipRadius = 5.0f;
    float playerBlipRadius = 7.0f;
    sf::Color radarGreen = sf::Color(0, 255, 70);
    sf::Color dimRadarGreen = sf::Color(0, 140, 45);
};
