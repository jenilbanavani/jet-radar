#pragma once

#include <SFML/Graphics/Color.hpp>

struct RadarConfig {
    unsigned int windowWidth  = 1000;
    unsigned int windowHeight = 1000;

    float radarRadius = 370.0f;
    float radarRange  = 1000.0f;

    float scanDegreesPerSecond = 90.0f;

    float enemyBlipRadius  = 6.0f;
    float playerBlipRadius = 8.0f;

    // Detection pulse
    float pulseMaxRadius   = 28.0f;  // px — how big the ring grows
    float pulseDuration    = 1.4f;   // seconds the ring lives

    // Lock-on brackets
    float lockOnBracketSize = 20.0f; // half-size of corner brackets

    // Lock-on acquisition timer
    float lockAcquireTime  = 2.5f;   // seconds to go from ACQUIRING -> LOCKED
    float lockWarnFlashHz  = 6.0f;   // flashes per second during acquiring

    // Missile
    float missileReadyFlashHz = 2.5f;  // slower pulse once locked

    // Player movement
    float playerSpeed = 200.0f;        // world-units per second

    // Colours
    sf::Color radarGreen       = sf::Color(0, 255, 70);
    sf::Color dimRadarGreen    = sf::Color(0, 140, 45);
    sf::Color lockOnColor      = sf::Color(255, 220, 0);    // gold — locked
    sf::Color acquiringColor   = sf::Color(255, 140, 0);    // amber — acquiring
    sf::Color pulseColor       = sf::Color(80, 255, 130);   // bright green
    sf::Color missileReadyColor= sf::Color(255, 60, 60);    // hot-red — missile ready
};
