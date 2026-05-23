// =============================================================================
//  TelemetryRenderer.cpp — Rocket Tracking System v1.0
//  Radar display + telemetry panel
// =============================================================================

#include "TelemetryRenderer.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace {

constexpr float kPi = 3.14159265359f;

float toRadians(float deg) { return deg * kPi / 180.0f; }

/// Build a filled fan (sector) as a ConvexShape — carried over from old renderer.
sf::ConvexShape makeSector(sf::Vector2f centre, float radius,
                            float angleDeg, float spreadDeg,
                            unsigned int segments = 48)
{
    sf::ConvexShape shape;
    shape.setPointCount(segments + 2);
    shape.setPoint(0, centre);

    const float startDeg = angleDeg - spreadDeg;
    for (unsigned int i = 0; i <= segments; ++i) {
        const float t   = static_cast<float>(i) / static_cast<float>(segments);
        const float deg = startDeg + t * spreadDeg;
        const float rad = toRadians(deg);
        shape.setPoint(
            i + 1,
            centre + sf::Vector2f(std::cos(rad) * radius,
                                   std::sin(rad) * radius));
    }
    return shape;
}

/// Format seconds into T+MM:SS or T-SS (for countdown).
std::string formatMissionTime(float seconds)
{
    std::ostringstream oss;
    if (seconds < 0.0f) {
        oss << "T-" << std::fixed << std::setprecision(1) << -seconds;
    } else {
        const int totalSec = static_cast<int>(seconds);
        const int mins     = totalSec / 60;
        const int secs     = totalSec % 60;
        oss << "T+" << std::setfill('0') << std::setw(2) << mins
            << ":" << std::setfill('0') << std::setw(2) << secs;
    }
    return oss.str();
}

/// Human-readable flight state name.
const char* flightStateName(FlightState s)
{
    switch (s) {
        case FlightState::Idle:               return "IDLE";
        case FlightState::Launch:             return "IGNITION";
        case FlightState::Ascending:          return "ASCENDING";
        case FlightState::BoosterSeparation:  return "STAGE SEP";
        case FlightState::Coasting:           return "COASTING";
        case FlightState::SignalLost:         return "SIGNAL LOST";
        case FlightState::MissionComplete:    return "COMPLETE";
    }
    return "UNKNOWN";
}

/// Format a number with commas for readability (e.g. 12,450).
std::string formatNumber(float value, int decimals = 0)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << value;
    std::string result = oss.str();

    // Insert commas into the integer part
    std::string intPart = result;
    std::string fracPart;
    auto dotPos = result.find('.');
    if (dotPos != std::string::npos) {
        intPart  = result.substr(0, dotPos);
        fracPart = result.substr(dotPos);
    }

    int insertPos = static_cast<int>(intPart.length()) - 3;
    while (insertPos > 0) {
        if (intPart[0] == '-') {
            if (insertPos > 1) intPart.insert(insertPos, ",");
        } else {
            intPart.insert(insertPos, ",");
        }
        insertPos -= 3;
    }
    return intPart + fracPart;
}

} // namespace

// =============================================================================
// Construction
// =============================================================================

TelemetryRenderer::TelemetryRenderer(RadarConfig config)
    : m_config(config)
{
}

bool TelemetryRenderer::loadFont(const std::string& path)
{
    m_fontLoaded = m_font.openFromFile(path);
    return m_fontLoaded;
}

// =============================================================================
// Main render
// =============================================================================

void TelemetryRenderer::render(sf::RenderWindow& window,
                                const RocketSimulation& simulation) const
{
    drawRadarBackground(window);
    drawAltitudeRings(window);
    drawSweepSector(window, simulation.scanAngleDegrees());
    drawTrajectoryTrails(window, simulation);
    drawDetectionPulses(window, simulation);
    drawRocketBlips(window, simulation);
    drawStationBlip(window);
    drawTelemetryPanel(window, simulation);
    drawTopBar(window, simulation);
    drawBottomBar(window);
}

// =============================================================================
// Radar Background
// =============================================================================

void TelemetryRenderer::drawRadarBackground(sf::RenderWindow& window) const
{
    const auto centre = radarCenter();

    // Full-window dark fill
    sf::RectangleShape bg(
        {static_cast<float>(m_config.windowWidth),
         static_cast<float>(m_config.windowHeight)});
    bg.setPosition({0.0f, 0.0f});
    bg.setFillColor(sf::Color(2, 8, 3));
    window.draw(bg);

    // Outer radar ring with dark fill
    sf::CircleShape outer(m_config.radarRadius);
    outer.setOrigin({m_config.radarRadius, m_config.radarRadius});
    outer.setPosition(centre);
    outer.setFillColor(m_config.radarBackground);
    outer.setOutlineColor(m_config.radarGreen);
    outer.setOutlineThickness(2.0f);
    window.draw(outer);

    // Crosshairs
    const float diam = m_config.radarRadius * 2.0f;
    sf::RectangleShape horizontal({diam, 1.0f});
    horizontal.setOrigin({m_config.radarRadius, 0.5f});
    horizontal.setPosition(centre);
    horizontal.setFillColor(sf::Color(0, 130, 40, 100));
    window.draw(horizontal);

    sf::RectangleShape vertical({1.0f, diam});
    vertical.setOrigin({0.5f, m_config.radarRadius});
    vertical.setPosition(centre);
    vertical.setFillColor(sf::Color(0, 130, 40, 100));
    window.draw(vertical);
}

// =============================================================================
// Altitude Rings — labeled concentric rings
// =============================================================================

void TelemetryRenderer::drawAltitudeRings(sf::RenderWindow& window) const
{
    const auto centre = radarCenter();
    constexpr int ringCount = 4;

    for (int ring = 1; ring <= ringCount; ++ring) {
        const float fraction = static_cast<float>(ring) / static_cast<float>(ringCount);
        const float r = m_config.radarRadius * fraction;

        sf::CircleShape circle(r);
        circle.setOrigin({r, r});
        circle.setPosition(centre);
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineColor(sf::Color(0, 100, 30, ring == ringCount ? 160 : 90));
        circle.setOutlineThickness(1.0f);
        window.draw(circle);

        // Altitude label
        if (m_fontLoaded) {
            const float altMetres = m_config.radarRange * fraction;
            std::ostringstream oss;
            oss << formatNumber(altMetres) << "m";

            sf::Text label(m_font, oss.str(), 10);
            label.setFillColor(sf::Color(0, 140, 50, 180));
            label.setPosition({centre.x + r - 40.0f, centre.y + 4.0f});
            window.draw(label);
        }
    }
}

// =============================================================================
// Sweep Sector + Scan Line
// =============================================================================

void TelemetryRenderer::drawSweepSector(sf::RenderWindow& window, float angleDeg) const
{
    const auto centre = radarCenter();

    // Layered trailing sectors
    constexpr int   layers    = 6;
    constexpr float spreadDeg = 55.0f;

    for (int i = 0; i < layers; ++i) {
        const float layerSpread = spreadDeg * static_cast<float>(layers - i)
                                  / static_cast<float>(layers);
        const auto  alpha = static_cast<std::uint8_t>(32 - i * 5);

        sf::ConvexShape sector = makeSector(
            centre, m_config.radarRadius, angleDeg, layerSpread);

        sector.setFillColor(sf::Color(0, 200, 65, alpha));
        window.draw(sector);
    }

    // Bright scan line
    sf::RectangleShape scanLine({m_config.radarRadius, 2.5f});
    scanLine.setOrigin({0.0f, 1.25f});
    scanLine.setPosition(centre);
    scanLine.setRotation(sf::degrees(angleDeg));
    scanLine.setFillColor(sf::Color(80, 255, 120, 220));
    window.draw(scanLine);
}

// =============================================================================
// Station Blip — fixed at centre
// =============================================================================

void TelemetryRenderer::drawStationBlip(sf::RenderWindow& window) const
{
    const auto centre = radarCenter();

    // Station icon — small diamond
    sf::ConvexShape diamond;
    diamond.setPointCount(4);
    const float s = m_config.stationBlipRadius;
    diamond.setPoint(0, centre + sf::Vector2f(0.0f, -s));
    diamond.setPoint(1, centre + sf::Vector2f(s,  0.0f));
    diamond.setPoint(2, centre + sf::Vector2f(0.0f,  s));
    diamond.setPoint(3, centre + sf::Vector2f(-s, 0.0f));
    diamond.setFillColor(m_config.stationColor);
    window.draw(diamond);

    // "GND" label
    if (m_fontLoaded) {
        sf::Text label(m_font, "GND", 9);
        label.setFillColor(m_config.stationColor);
        label.setPosition({centre.x + s + 3.0f, centre.y - 5.0f});
        window.draw(label);
    }
}

// =============================================================================
// Rocket Blips
// =============================================================================

void TelemetryRenderer::drawRocketBlips(sf::RenderWindow& window,
                                         const RocketSimulation& sim) const
{
    const int selected = sim.selectedRocketIndex();

    for (int i = 0; i < static_cast<int>(sim.rockets().size()); ++i) {
        const RocketTarget& rocket = sim.rockets()[i];

        // Don't show idle rockets on the radar
        if (rocket.flightState() == FlightState::Idle) continue;
        if (!isInsideRadarRange(rocket.position())) continue;

        const sf::Vector2f blipPos = worldToRadar(rocket.position());
        const bool isSelected = (i == selected);

        // Glow behind the blip (fades with signal)
        const float signal = rocket.telemetry().signalStrength;
        if (signal > 0.1f) {
            const float glowR = m_config.rocketBlipRadius * 3.0f;
            sf::CircleShape glow(glowR);
            glow.setOrigin({glowR, glowR});
            glow.setPosition(blipPos);
            glow.setFillColor(sf::Color(0, 255, 180,
                static_cast<std::uint8_t>(40 * signal)));
            window.draw(glow);
        }

        // Main blip
        sf::CircleShape blip(m_config.rocketBlipRadius);
        blip.setOrigin({m_config.rocketBlipRadius, m_config.rocketBlipRadius});
        blip.setPosition(blipPos);

        sf::Color blipColor = m_config.rocketBlipColor;
        if (rocket.flightState() == FlightState::SignalLost) {
            blipColor = m_config.signalLostColor;
        } else if (isSelected) {
            blipColor = sf::Color(0, 255, 255);  // cyan for selected
        }
        blipColor.a = static_cast<std::uint8_t>(255 * std::max(signal, 0.2f));
        blip.setFillColor(blipColor);
        window.draw(blip);

        // Selection brackets
        if (isSelected) {
            const float bs = m_config.rocketBlipRadius * 3.5f;
            const float arm = bs * 0.4f;
            constexpr float thick = 1.5f;
            const sf::Color bracketCol(0, 255, 255, 200);

            auto drawCorner = [&](float cx, float cy, float hDir, float vDir) {
                sf::RectangleShape h({arm, thick});
                h.setOrigin({hDir > 0 ? 0.0f : arm, thick * 0.5f});
                h.setPosition(blipPos + sf::Vector2f(cx, cy));
                h.setFillColor(bracketCol);
                window.draw(h);

                sf::RectangleShape v({thick, arm});
                v.setOrigin({thick * 0.5f, vDir > 0 ? 0.0f : arm});
                v.setPosition(blipPos + sf::Vector2f(cx, cy));
                v.setFillColor(bracketCol);
                window.draw(v);
            };

            drawCorner(-bs, -bs, +1, +1);
            drawCorner( bs, -bs, -1, +1);
            drawCorner(-bs,  bs, +1, -1);
            drawCorner( bs,  bs, -1, -1);
        }

        // Callsign label
        if (m_fontLoaded) {
            sf::Text label(m_font, rocket.callsign(), 9);
            label.setFillColor(isSelected ? sf::Color(0, 255, 255, 200)
                                           : sf::Color(0, 200, 100, 160));
            label.setPosition({blipPos.x + m_config.rocketBlipRadius + 4.0f,
                               blipPos.y - 6.0f});
            window.draw(label);
        }
    }
}

// =============================================================================
// Detection Pulses
// =============================================================================

void TelemetryRenderer::drawDetectionPulses(sf::RenderWindow& window,
                                              const RocketSimulation& sim) const
{
    for (const RocketTarget& rocket : sim.rockets()) {
        if (rocket.pulseTimer() <= 0.0f) continue;
        if (rocket.flightState() == FlightState::Idle) continue;
        if (!isInsideRadarRange(rocket.position())) continue;

        const sf::Vector2f blipPos = worldToRadar(rocket.position());

        const float t        = rocket.pulseTimer() / sim.config().pulseDuration;
        const float progress = 1.0f - t;
        const float radius   = m_config.pulseMaxRadius * progress;
        const auto  alpha    = static_cast<std::uint8_t>(200 * t * t);

        sf::CircleShape pulse(radius);
        pulse.setOrigin({radius, radius});
        pulse.setPosition(blipPos);
        pulse.setFillColor(sf::Color::Transparent);
        pulse.setOutlineColor(sf::Color(
            m_config.pulseColor.r,
            m_config.pulseColor.g,
            m_config.pulseColor.b,
            alpha));
        pulse.setOutlineThickness(1.5f);
        window.draw(pulse);
    }
}

// =============================================================================
// Trajectory Trails
// =============================================================================

void TelemetryRenderer::drawTrajectoryTrails(sf::RenderWindow& window,
                                               const RocketSimulation& sim) const
{
    for (int ri = 0; ri < static_cast<int>(sim.rockets().size()); ++ri) {
        const RocketTarget& rocket = sim.rockets()[ri];
        const auto& trail = rocket.trail();
        if (trail.empty()) continue;

        const bool isSelected = (ri == sim.selectedRocketIndex());

        for (std::size_t i = 0; i < trail.size(); ++i) {
            if (!isInsideRadarRange(trail[i])) continue;

            const sf::Vector2f pos = worldToRadar(trail[i]);

            // Fade older points
            const float age = static_cast<float>(i) / static_cast<float>(trail.size());
            const auto alpha = static_cast<std::uint8_t>(
                (isSelected ? 140 : 80) * age);

            const float dotR = isSelected ? 1.8f : 1.2f;
            sf::CircleShape dot(dotR);
            dot.setOrigin({dotR, dotR});
            dot.setPosition(pos);
            dot.setFillColor(sf::Color(
                m_config.trailColor.r,
                m_config.trailColor.g,
                m_config.trailColor.b,
                alpha));
            window.draw(dot);
        }
    }
}

// =============================================================================
// Telemetry Panel (right side)
// =============================================================================

void TelemetryRenderer::drawTelemetryPanel(sf::RenderWindow& window,
                                             const RocketSimulation& sim) const
{
    const float px = m_config.panelX;
    const float pw = m_config.panelWidth;
    const float ph = static_cast<float>(m_config.windowHeight) - 80.0f;

    // Panel background
    sf::RectangleShape panelBg({pw, ph});
    panelBg.setPosition({px, 40.0f});
    panelBg.setFillColor(m_config.panelBg);
    panelBg.setOutlineColor(m_config.panelBorder);
    panelBg.setOutlineThickness(1.5f);
    window.draw(panelBg);

    // Decorative corner accents
    constexpr float accentLen = 20.0f;
    constexpr float accentThick = 2.0f;
    const sf::Color accentCol = m_config.radarGreen;

    auto drawAccent = [&](float x, float y, float hDir, float vDir) {
        sf::RectangleShape h({accentLen, accentThick});
        h.setPosition({x, y});
        if (hDir < 0) h.setPosition({x - accentLen, y});
        h.setFillColor(accentCol);
        window.draw(h);

        sf::RectangleShape v({accentThick, accentLen});
        v.setPosition({x, y});
        if (vDir < 0) v.setPosition({x, y - accentLen});
        v.setFillColor(accentCol);
        window.draw(v);
    };

    drawAccent(px, 40.0f, 1, 1);
    drawAccent(px + pw, 40.0f, -1, 1);
    drawAccent(px, 40.0f + ph, 1, -1);
    drawAccent(px + pw, 40.0f + ph, -1, -1);

    if (!m_fontLoaded) return;

    float yOffset = 56.0f;
    const float textX = px + 18.0f;

    // ---- Mission Timer ----
    drawMissionTimer(window, sim.missionElapsed(), textX, yOffset);
    yOffset += 50.0f;

    // ---- Separator ----
    sf::RectangleShape sep({pw - 36.0f, 1.0f});
    sep.setPosition({textX, yOffset});
    sep.setFillColor(sf::Color(0, 120, 40, 120));
    window.draw(sep);
    yOffset += 12.0f;

    // ---- Selected rocket telemetry ----
    const int sel = sim.selectedRocketIndex();
    if (sel >= 0 && sel < static_cast<int>(sim.rockets().size())) {
        drawSelectedRocketTelemetry(window, sim.rockets()[sel], textX, yOffset);
    }

    // ---- Rocket summary list (bottom of panel) ----
    drawRocketSummaryList(window, sim, textX,
                           40.0f + ph - 30.0f * static_cast<float>(sim.rockets().size()) - 20.0f);
}

// =============================================================================
// Mission Timer
// =============================================================================

void TelemetryRenderer::drawMissionTimer(sf::RenderWindow& window,
                                           float elapsed,
                                           float x, float y) const
{
    sf::Text label(m_font, "MISSION ELAPSED", 11);
    label.setFillColor(m_config.labelColor);
    label.setPosition({x, y});
    window.draw(label);

    sf::Text value(m_font, formatMissionTime(elapsed), 22);
    value.setFillColor(m_config.telemetryText);
    value.setPosition({x, y + 16.0f});
    window.draw(value);
}

// =============================================================================
// Selected Rocket Telemetry — full readout
// =============================================================================

void TelemetryRenderer::drawSelectedRocketTelemetry(sf::RenderWindow& window,
                                                      const RocketTarget& rocket,
                                                      float x, float y) const
{
    const auto& tel = rocket.telemetry();
    float lineY = y;
    const float labelSize  = 11.0f;
    const float valueSize  = 14.0f;
    const float lineH      = 28.0f;
    const float sectionGap = 14.0f;

    // ---- Callsign + State ----
    {
        std::string title = "RKT-" + rocket.callsign();
        sf::Text callsignText(m_font, title, 16);
        callsignText.setFillColor(sf::Color(0, 255, 255));
        callsignText.setPosition({x, lineY});
        window.draw(callsignText);
        lineY += 22.0f;

        drawFlightStateLabel(window, rocket.flightState(), x, lineY);
        lineY += lineH + sectionGap;
    }

    // ---- Separator ----
    {
        sf::RectangleShape sep({m_config.panelWidth - 36.0f, 1.0f});
        sep.setPosition({x, lineY - 6.0f});
        sep.setFillColor(sf::Color(0, 100, 35, 100));
        window.draw(sep);
    }

    // Helper to draw a label + value row
    const sf::Color defaultValColor = m_config.valueColor;
    auto drawRow = [&](const std::string& label, const std::string& val,
                       sf::Color valColor = sf::Color()) {
        if (valColor == sf::Color()) valColor = defaultValColor;
        sf::Text lbl(m_font, label, static_cast<unsigned int>(labelSize));
        lbl.setFillColor(m_config.labelColor);
        lbl.setPosition({x, lineY});
        window.draw(lbl);

        sf::Text v(m_font, val, static_cast<unsigned int>(valueSize));
        v.setFillColor(valColor);
        v.setPosition({x + 130.0f, lineY - 2.0f});
        window.draw(v);

        lineY += lineH;
    };

    // ---- Altitude ----
    drawRow("ALTITUDE", formatNumber(tel.altitude) + " m");

    // ---- Max Altitude ----
    drawRow("MAX ALT", formatNumber(tel.maxAltitude) + " m",
            sf::Color(0, 200, 255));

    // ---- Velocity ----
    {
        sf::Color velCol = m_config.valueColor;
        if (tel.velocity > 300.0f) velCol = m_config.warningColor;
        drawRow("VELOCITY", formatNumber(tel.velocity, 1) + " m/s", velCol);
    }

    // ---- Acceleration ----
    {
        sf::Color accCol = m_config.valueColor;
        if (tel.acceleration < 0.0f) accCol = m_config.warningColor;
        drawRow("ACCEL", formatNumber(tel.acceleration, 1) + " m/s" + "\xC2\xB2", accCol);
    }

    // ---- Downrange ----
    drawRow("DOWNRANGE", formatNumber(tel.downrangeX, 0) + " m");

    // ---- Mission Time ----
    drawRow("MISSION T", formatMissionTime(rocket.missionTime()));

    lineY += sectionGap;

    // ---- Fuel Gauge ----
    {
        sf::Text lbl(m_font, "FUEL", static_cast<unsigned int>(labelSize));
        lbl.setFillColor(m_config.labelColor);
        lbl.setPosition({x, lineY});
        window.draw(lbl);
        drawFuelGauge(window, tel.fuelPercent, x + 130.0f, lineY + 2.0f);
        lineY += lineH;
    }

    // ---- Signal Strength ----
    {
        sf::Text lbl(m_font, "SIGNAL", static_cast<unsigned int>(labelSize));
        lbl.setFillColor(m_config.labelColor);
        lbl.setPosition({x, lineY});
        window.draw(lbl);
        drawSignalBar(window, tel.signalStrength, x + 130.0f, lineY + 2.0f);
        lineY += lineH;
    }
}

// =============================================================================
// Signal Strength Bar
// =============================================================================

void TelemetryRenderer::drawSignalBar(sf::RenderWindow& window,
                                        float strength, float x, float y) const
{
    constexpr float barW = 180.0f;
    constexpr float barH = 14.0f;

    // Background
    sf::RectangleShape bg({barW, barH});
    bg.setPosition({x, y});
    bg.setFillColor(sf::Color(0, 30, 10, 200));
    bg.setOutlineColor(sf::Color(0, 100, 40, 150));
    bg.setOutlineThickness(1.0f);
    window.draw(bg);

    // Fill
    const float fillW = barW * std::clamp(strength, 0.0f, 1.0f);
    if (fillW > 0.0f) {
        sf::Color barCol = m_config.radarGreen;
        if (strength < 0.3f) barCol = m_config.dangerColor;
        else if (strength < 0.6f) barCol = m_config.warningColor;

        sf::RectangleShape fill({fillW, barH});
        fill.setPosition({x, y});
        fill.setFillColor(barCol);
        window.draw(fill);
    }

    // Percentage label
    if (m_fontLoaded) {
        std::ostringstream oss;
        oss << static_cast<int>(strength * 100.0f) << "%";
        sf::Text pct(m_font, oss.str(), 10);
        pct.setFillColor(sf::Color(255, 255, 255, 200));
        pct.setPosition({x + barW + 6.0f, y});
        window.draw(pct);
    }
}

// =============================================================================
// Fuel Gauge
// =============================================================================

void TelemetryRenderer::drawFuelGauge(sf::RenderWindow& window,
                                        float fuelPercent, float x, float y) const
{
    constexpr float barW = 180.0f;
    constexpr float barH = 14.0f;

    sf::RectangleShape bg({barW, barH});
    bg.setPosition({x, y});
    bg.setFillColor(sf::Color(0, 30, 10, 200));
    bg.setOutlineColor(sf::Color(0, 100, 40, 150));
    bg.setOutlineThickness(1.0f);
    window.draw(bg);

    const float pct = std::clamp(fuelPercent, 0.0f, 100.0f);
    const float fillW = barW * pct / 100.0f;
    if (fillW > 0.0f) {
        sf::Color barCol = sf::Color(0, 200, 255);
        if (pct < 15.0f) barCol = m_config.dangerColor;
        else if (pct < 35.0f) barCol = m_config.warningColor;

        sf::RectangleShape fill({fillW, barH});
        fill.setPosition({x, y});
        fill.setFillColor(barCol);
        window.draw(fill);
    }

    if (m_fontLoaded) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << pct << "%";
        sf::Text label(m_font, oss.str(), 10);
        label.setFillColor(sf::Color(255, 255, 255, 200));
        label.setPosition({x + barW + 6.0f, y});
        window.draw(label);
    }
}

// =============================================================================
// Flight State Label (with color coding)
// =============================================================================

void TelemetryRenderer::drawFlightStateLabel(sf::RenderWindow& window,
                                               FlightState state,
                                               float x, float y) const
{
    sf::Color col;
    switch (state) {
        case FlightState::Idle:               col = m_config.idleColor;       break;
        case FlightState::Launch:             col = m_config.warningColor;    break;
        case FlightState::Ascending:          col = m_config.radarGreen;      break;
        case FlightState::BoosterSeparation:  col = m_config.separationColor; break;
        case FlightState::Coasting:           col = m_config.missionOkColor;  break;
        case FlightState::SignalLost:         col = m_config.signalLostColor; break;
        case FlightState::MissionComplete:    col = m_config.missionOkColor;  break;
    }

    // Status badge background
    const std::string text = flightStateName(state);
    const float badgeW = static_cast<float>(text.length()) * 9.0f + 16.0f;
    sf::RectangleShape badge({badgeW, 20.0f});
    badge.setPosition({x, y});
    badge.setFillColor(sf::Color(col.r / 6, col.g / 6, col.b / 6, 200));
    badge.setOutlineColor(col);
    badge.setOutlineThickness(1.0f);
    window.draw(badge);

    sf::Text label(m_font, text, 11);
    label.setFillColor(col);
    label.setPosition({x + 8.0f, y + 3.0f});
    window.draw(label);
}

// =============================================================================
// Rocket Summary List (bottom of panel)
// =============================================================================

void TelemetryRenderer::drawRocketSummaryList(sf::RenderWindow& window,
                                                const RocketSimulation& sim,
                                                float x, float y) const
{
    // Section header
    sf::Text header(m_font, "ALL ROCKETS", 11);
    header.setFillColor(m_config.labelColor);
    header.setPosition({x, y - 20.0f});
    window.draw(header);

    sf::RectangleShape sep({m_config.panelWidth - 36.0f, 1.0f});
    sep.setPosition({x, y - 6.0f});
    sep.setFillColor(sf::Color(0, 100, 35, 100));
    window.draw(sep);

    for (int i = 0; i < static_cast<int>(sim.rockets().size()); ++i) {
        const RocketTarget& r = sim.rockets()[i];
        const float rowY = y + static_cast<float>(i) * 28.0f;
        const bool isSel = (i == sim.selectedRocketIndex());

        // Selection indicator
        if (isSel) {
            sf::RectangleShape selBg({m_config.panelWidth - 36.0f, 24.0f});
            selBg.setPosition({x, rowY});
            selBg.setFillColor(sf::Color(0, 60, 30, 150));
            window.draw(selBg);

            sf::Text arrow(m_font, ">", 12);
            arrow.setFillColor(sf::Color(0, 255, 255));
            arrow.setPosition({x - 12.0f, rowY + 2.0f});
            window.draw(arrow);
        }

        // Callsign
        sf::Text name(m_font, r.callsign(), 12);
        name.setFillColor(isSel ? sf::Color(0, 255, 255) : m_config.telemetryText);
        name.setPosition({x + 4.0f, rowY + 3.0f});
        window.draw(name);

        // State
        sf::Text state(m_font, flightStateName(r.flightState()), 10);
        sf::Color stateCol;
        switch (r.flightState()) {
            case FlightState::Idle:               stateCol = m_config.idleColor;       break;
            case FlightState::Launch:             stateCol = m_config.warningColor;    break;
            case FlightState::Ascending:          stateCol = m_config.radarGreen;      break;
            case FlightState::BoosterSeparation:  stateCol = m_config.separationColor; break;
            case FlightState::Coasting:           stateCol = m_config.missionOkColor;  break;
            case FlightState::SignalLost:         stateCol = m_config.signalLostColor; break;
            case FlightState::MissionComplete:    stateCol = m_config.missionOkColor;  break;
        }
        state.setFillColor(stateCol);
        state.setPosition({x + 80.0f, rowY + 4.0f});
        window.draw(state);

        // Altitude
        std::ostringstream oss;
        oss << formatNumber(r.telemetry().altitude) << "m";
        sf::Text alt(m_font, oss.str(), 10);
        alt.setFillColor(m_config.valueColor);
        alt.setPosition({x + 200.0f, rowY + 4.0f});
        window.draw(alt);
    }
}

// =============================================================================
// Top Bar
// =============================================================================

void TelemetryRenderer::drawTopBar(sf::RenderWindow& window,
                                     const RocketSimulation& sim) const
{
    const float barH = 34.0f;

    sf::RectangleShape bar(
        {static_cast<float>(m_config.windowWidth), barH});
    bar.setPosition({0.0f, 0.0f});
    bar.setFillColor(sf::Color(0, 20, 8, 230));
    window.draw(bar);

    // Bottom edge line
    sf::RectangleShape edge(
        {static_cast<float>(m_config.windowWidth), 1.5f});
    edge.setPosition({0.0f, barH});
    edge.setFillColor(m_config.radarGreen);
    window.draw(edge);

    if (!m_fontLoaded) return;

    // Title
    sf::Text title(m_font, "ROCKET TRACKING SYSTEM v1.0", 14);
    title.setFillColor(m_config.radarGreen);
    title.setPosition({14.0f, 8.0f});
    window.draw(title);

    // Mission status
    {
        bool anyFlying = false;
        bool allComplete = true;
        for (const auto& r : sim.rockets()) {
            if (r.flightState() != FlightState::Idle &&
                r.flightState() != FlightState::MissionComplete) {
                anyFlying = true;
            }
            if (r.flightState() != FlightState::MissionComplete) {
                allComplete = false;
            }
        }

        std::string status;
        sf::Color statusCol;
        if (allComplete && !sim.rockets().empty()) {
            status = "MISSION COMPLETE";
            statusCol = m_config.missionOkColor;
        } else if (anyFlying) {
            status = "TRACKING ACTIVE";
            statusCol = m_config.radarGreen;
        } else {
            status = "STANDING BY";
            statusCol = m_config.idleColor;
        }

        sf::Text statusText(m_font, status, 13);
        statusText.setFillColor(statusCol);
        statusText.setPosition(
            {static_cast<float>(m_config.windowWidth) - 200.0f, 9.0f});
        window.draw(statusText);
    }
}

// =============================================================================
// Bottom Bar — key hints
// =============================================================================

void TelemetryRenderer::drawBottomBar(sf::RenderWindow& window) const
{
    const float barH = 30.0f;
    const float barY = static_cast<float>(m_config.windowHeight) - barH;

    sf::RectangleShape bar(
        {static_cast<float>(m_config.windowWidth), barH});
    bar.setPosition({0.0f, barY});
    bar.setFillColor(sf::Color(0, 20, 8, 230));
    window.draw(bar);

    sf::RectangleShape edge(
        {static_cast<float>(m_config.windowWidth), 1.5f});
    edge.setPosition({0.0f, barY - 1.5f});
    edge.setFillColor(sf::Color(0, 140, 50, 140));
    window.draw(edge);

    if (!m_fontLoaded) return;

    sf::Text hint(m_font,
        "[TAB] Select Rocket   [SPACE] Launch Selected   [L] Launch All   [ESC] Exit",
        11);
    hint.setFillColor(sf::Color(0, 160, 60, 200));
    hint.setPosition({14.0f, barY + 7.0f});
    window.draw(hint);
}

// =============================================================================
// Utility — coordinate transforms
// =============================================================================

sf::Vector2f TelemetryRenderer::radarCenter() const
{
    return {m_config.radarCenterX, m_config.radarCenterY};
}

sf::Vector2f TelemetryRenderer::worldToRadar(sf::Vector2f worldPos) const
{
    // Station is at origin — worldPos is relative to station already
    const float scale = m_config.radarRadius / m_config.radarRange;
    return radarCenter() + sf::Vector2f(worldPos.x * scale, worldPos.y * scale);
}

bool TelemetryRenderer::isInsideRadarRange(sf::Vector2f worldPos) const
{
    return vecLength(worldPos) <= m_config.radarRange;
}

float TelemetryRenderer::vecLength(sf::Vector2f v) const
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}
