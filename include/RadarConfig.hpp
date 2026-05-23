#pragma once

#include <SFML/Graphics/Color.hpp>

// =============================================================================
//  RadarConfig — tunable constants for the rocket tracking system
// =============================================================================
struct RadarConfig {
    // Window — wider to fit telemetry panel alongside the radar
    unsigned int windowWidth  = 1280;
    unsigned int windowHeight = 900;

    // Radar display geometry
    float radarRadius  = 370.0f;
    float radarCenterX = 420.0f;   // shifted left to leave room for panel
    float radarCenterY = 450.0f;
    float radarRange   = 15000.0f; // metres — full tracking range

    // Sweep
    float scanDegreesPerSecond = 60.0f;  // slower sweep for drama

    // Blip sizes
    float rocketBlipRadius  = 5.0f;
    float stationBlipRadius = 6.0f;

    // Detection pulse
    float pulseMaxRadius = 22.0f;
    float pulseDuration  = 1.8f;

    // Telemetry panel (right side)
    float panelX     = 860.0f;
    float panelWidth = 400.0f;

    // Rocket defaults
    float defaultTargetAltitude = 12000.0f;

    // Signal
    float signalMaxRange = 14000.0f;  // signal degrades beyond this

    // ---- Colour palette — aerospace green theme -----------------------------
    sf::Color radarGreen       = sf::Color(0, 255, 70);
    sf::Color dimRadarGreen    = sf::Color(0, 140, 45);
    sf::Color radarBackground  = sf::Color(0, 18, 5);

    sf::Color rocketBlipColor  = sf::Color(0, 255, 180);
    sf::Color trailColor       = sf::Color(0, 200, 100, 120);
    sf::Color stationColor     = sf::Color(120, 255, 160);

    sf::Color warningColor     = sf::Color(255, 180, 0);
    sf::Color dangerColor      = sf::Color(255, 60, 60);
    sf::Color pulseColor       = sf::Color(80, 255, 130);

    sf::Color panelBg          = sf::Color(0, 12, 4, 220);
    sf::Color panelBorder      = sf::Color(0, 180, 60, 180);
    sf::Color telemetryText    = sf::Color(0, 230, 80);
    sf::Color labelColor       = sf::Color(0, 140, 50);
    sf::Color valueColor       = sf::Color(0, 255, 120);
    sf::Color signalLostColor  = sf::Color(255, 50, 50);
    sf::Color idleColor        = sf::Color(120, 120, 140);
    sf::Color missionOkColor   = sf::Color(0, 200, 255);
    sf::Color separationColor  = sf::Color(255, 200, 50);
};
