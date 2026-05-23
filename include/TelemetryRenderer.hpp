#pragma once

#include "RadarConfig.hpp"
#include "RocketSimulation.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>

// =============================================================================
//  TelemetryRenderer — replaces the old RadarRenderer
//  Draws the radar display (left) and telemetry panel (right).
// =============================================================================
class TelemetryRenderer {
public:
    explicit TelemetryRenderer(RadarConfig config);

    /// Returns false if the font could not be loaded (non-fatal — text is skipped).
    bool loadFont(const std::string& path);

    void render(sf::RenderWindow& window, const RocketSimulation& simulation) const;

private:
    // ---- Radar display (left side) ------------------------------------------
    void drawRadarBackground(sf::RenderWindow& window) const;
    void drawAltitudeRings(sf::RenderWindow& window) const;
    void drawSweepSector(sf::RenderWindow& window, float angleDeg) const;
    void drawStationBlip(sf::RenderWindow& window) const;
    void drawRocketBlips(sf::RenderWindow& window, const RocketSimulation& sim) const;
    void drawDetectionPulses(sf::RenderWindow& window, const RocketSimulation& sim) const;
    void drawTrajectoryTrails(sf::RenderWindow& window, const RocketSimulation& sim) const;

    // ---- Telemetry panel (right side) ---------------------------------------
    void drawTelemetryPanel(sf::RenderWindow& window, const RocketSimulation& sim) const;
    void drawMissionTimer(sf::RenderWindow& window, float elapsed, float x, float y) const;
    void drawSelectedRocketTelemetry(sf::RenderWindow& window,
                                      const RocketTarget& rocket,
                                      float x, float y) const;
    void drawSignalBar(sf::RenderWindow& window, float strength, float x, float y) const;
    void drawFuelGauge(sf::RenderWindow& window, float fuelPercent, float x, float y) const;
    void drawFlightStateLabel(sf::RenderWindow& window, FlightState state,
                               float x, float y) const;
    void drawRocketSummaryList(sf::RenderWindow& window,
                                const RocketSimulation& sim,
                                float x, float y) const;

    // ---- HUD bars -----------------------------------------------------------
    void drawTopBar(sf::RenderWindow& window, const RocketSimulation& sim) const;
    void drawBottomBar(sf::RenderWindow& window) const;

    // ---- Utilities ----------------------------------------------------------
    sf::Vector2f radarCenter() const;
    sf::Vector2f worldToRadar(sf::Vector2f worldPos) const;
    bool         isInsideRadarRange(sf::Vector2f worldPos) const;
    float        vecLength(sf::Vector2f v) const;

    RadarConfig m_config;
    sf::Font    m_font;
    bool        m_fontLoaded = false;
};
