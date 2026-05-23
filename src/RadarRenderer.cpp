// =============================================================================
//  RadarRenderer.cpp  —  v0.2  (SFML 3.x)
//  Features: sweep sector, enemy detection pulse, lock-on indicator, HUD text
// =============================================================================

#include "RadarRenderer.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace {

constexpr float kPi = 3.14159265359f;

float toRadians(float deg) { return deg * kPi / 180.0f; }

float vecLength(sf::Vector2f v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

// Build a filled fan (sector) as a ConvexShape.
// angleDeg  : current scan-line angle (clockwise from +X axis)
// spreadDeg : how many degrees back the trail covers
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

} // namespace

// ---------------------------------------------------------------------------
RadarRenderer::RadarRenderer(RadarConfig config)
    : m_config(config)
{
}

bool RadarRenderer::loadFont(const std::string& path)
{
    // SFML 3: Font uses openFromFile instead of loadFromFile
    m_fontLoaded = m_font.openFromFile(path);
    return m_fontLoaded;
}

// ---------------------------------------------------------------------------
void RadarRenderer::render(sf::RenderWindow& window, const Simulation& simulation) const
{
    drawBackground(window);
    drawSweepSector(window, simulation.scanAngleDegrees());
    drawDetectionPulses(window, simulation);
    drawEnemyBlips(window, simulation);
    drawPlayerBlip(window);
    drawLockOnIndicator(window, simulation);
    drawLockTimerArc(window, simulation);
    drawMissileReadyIndicator(window, simulation);
    drawHUD(window, simulation);

    // Full-screen flash on missile fire
    if (simulation.missileFlashTimer() > 0.0f) {
        const float t = simulation.missileFlashTimer() / 0.3f; // normalise to [0,1]
        const auto alpha = static_cast<std::uint8_t>(180 * t);
        sf::RectangleShape flash(
            {static_cast<float>(m_config.windowWidth),
             static_cast<float>(m_config.windowHeight)});
        flash.setPosition({0.0f, 0.0f});
        flash.setFillColor(sf::Color(255, 60, 40, alpha));
        window.draw(flash);
    }
}

// ---------------------------------------------------------------------------
// Background — outer ring, range rings, crosshairs
// ---------------------------------------------------------------------------
void RadarRenderer::drawBackground(sf::RenderWindow& window) const
{
    const auto centre = radarCenter();

    // Outer ring with dark fill
    sf::CircleShape outer(m_config.radarRadius);
    outer.setOrigin({m_config.radarRadius, m_config.radarRadius});
    outer.setPosition(centre);
    outer.setFillColor(sf::Color(0, 18, 5));
    outer.setOutlineColor(m_config.radarGreen);
    outer.setOutlineThickness(2.0f);
    window.draw(outer);

    // Range rings (25 %, 50 %, 75 %)
    for (int ring = 1; ring <= 3; ++ring) {
        const float r = m_config.radarRadius * static_cast<float>(ring) / 4.0f;
        sf::CircleShape circle(r);
        circle.setOrigin({r, r});
        circle.setPosition(centre);
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineColor(sf::Color(0, 100, 30, 120));
        circle.setOutlineThickness(1.0f);
        window.draw(circle);
    }

    // Crosshairs
    const float diam = m_config.radarRadius * 2.0f;
    sf::RectangleShape horizontal({diam, 1.0f});
    horizontal.setOrigin({m_config.radarRadius, 0.5f});
    horizontal.setPosition(centre);
    horizontal.setFillColor(sf::Color(0, 130, 40, 130));
    window.draw(horizontal);

    sf::RectangleShape vertical({1.0f, diam});
    vertical.setOrigin({0.5f, m_config.radarRadius});
    vertical.setPosition(centre);
    vertical.setFillColor(sf::Color(0, 130, 40, 130));
    window.draw(vertical);
}

// ---------------------------------------------------------------------------
// Sweep sector + scan line
// ---------------------------------------------------------------------------
void RadarRenderer::drawSweepSector(sf::RenderWindow& window, float angleDeg) const
{
    const auto centre = radarCenter();

    // Layered trailing sectors (front → back, each narrower + more transparent)
    constexpr int   layers    = 6;
    constexpr float spreadDeg = 55.0f;

    for (int i = 0; i < layers; ++i) {
        const float layerSpread = spreadDeg * static_cast<float>(layers - i) / static_cast<float>(layers);
        const auto  alpha       = static_cast<std::uint8_t>(38 - i * 6);

        sf::ConvexShape sector = makeSector(
            centre,
            m_config.radarRadius,
            angleDeg,
            layerSpread);

        sector.setFillColor(sf::Color(0, 220, 70, alpha));
        window.draw(sector);
    }

    // Bright scan line on top
    sf::RectangleShape scanLine({m_config.radarRadius, 3.0f});
    scanLine.setOrigin({0.0f, 1.5f});
    scanLine.setPosition(centre);
    scanLine.setRotation(sf::degrees(angleDeg));
    scanLine.setFillColor(sf::Color(100, 255, 130, 230));
    window.draw(scanLine);
}

// ---------------------------------------------------------------------------
// Detection pulses — expanding ring that fades out over pulseDuration
// ---------------------------------------------------------------------------
void RadarRenderer::drawDetectionPulses(sf::RenderWindow& window,
                                         const Simulation& simulation) const
{
    const auto playerPos = simulation.player().position();

    for (const Enemy& enemy : simulation.enemies()) {
        if (enemy.pulseTimer() <= 0.0f) continue;
        if (!isInsideRadarRange(enemy.position(), playerPos)) continue;

        const sf::Vector2f blipPos = worldToRadar(enemy.position(), playerPos);

        // t = 1 when fresh, 0 when expired
        const float t        = enemy.pulseTimer() / simulation.config().pulseDuration;
        const float progress = 1.0f - t;
        const float radius   = m_config.pulseMaxRadius * progress;
        const auto  alpha    = static_cast<std::uint8_t>(255 * t * t);

        sf::CircleShape pulse(radius);
        pulse.setOrigin({radius, radius});
        pulse.setPosition(blipPos);
        pulse.setFillColor(sf::Color::Transparent);
        pulse.setOutlineColor(sf::Color(
            m_config.pulseColor.r,
            m_config.pulseColor.g,
            m_config.pulseColor.b,
            alpha));
        pulse.setOutlineThickness(2.0f);
        window.draw(pulse);
    }
}

// ---------------------------------------------------------------------------
// Enemy blips
// ---------------------------------------------------------------------------
void RadarRenderer::drawEnemyBlips(sf::RenderWindow& window,
                                    const Simulation& simulation) const
{
    const auto playerPos   = simulation.player().position();
    const int  lockedIndex = simulation.lockedEnemyIndex();

    for (int i = 0; i < static_cast<int>(simulation.enemies().size()); ++i) {
        const Enemy& enemy = simulation.enemies()[i];
        if (!isInsideRadarRange(enemy.position(), playerPos)) continue;

        const sf::Vector2f blipPos  = worldToRadar(enemy.position(), playerPos);
        const bool         isLocked = (i == lockedIndex);

        sf::CircleShape blip(m_config.enemyBlipRadius);
        blip.setOrigin({m_config.enemyBlipRadius, m_config.enemyBlipRadius});
        blip.setPosition(blipPos);
        blip.setFillColor(isLocked ? sf::Color(255, 220, 0) : sf::Color(255, 65, 65));
        window.draw(blip);
    }
}

// ---------------------------------------------------------------------------
// Player blip (always at centre)
// ---------------------------------------------------------------------------
void RadarRenderer::drawPlayerBlip(sf::RenderWindow& window) const
{
    const auto centre = radarCenter();

    sf::CircleShape playerBlip(m_config.playerBlipRadius);
    playerBlip.setOrigin({m_config.playerBlipRadius, m_config.playerBlipRadius});
    playerBlip.setPosition(centre);
    playerBlip.setFillColor(sf::Color(120, 255, 160));
    window.draw(playerBlip);
}

// ---------------------------------------------------------------------------
// Lock-on indicator — four corner L-brackets + pulsing ring
// ---------------------------------------------------------------------------
void RadarRenderer::drawLockOnIndicator(sf::RenderWindow& window,
                                         const Simulation& simulation) const
{
    const int lockedIndex = simulation.lockedEnemyIndex();
    if (lockedIndex < 0 || lockedIndex >= static_cast<int>(simulation.enemies().size()))
        return;

    const Enemy&       enemy     = simulation.enemies()[lockedIndex];
    const sf::Vector2f playerPos = simulation.player().position();
    if (!isInsideRadarRange(enemy.position(), playerPos)) return;

    const sf::Vector2f blipPos = worldToRadar(enemy.position(), playerPos);
    const float        s       = m_config.lockOnBracketSize;
    const float        arm     = s * 0.45f;
    const sf::Color    col     = m_config.lockOnColor;
    constexpr float    thick   = 2.0f;

    // Draw one L-shaped corner:
    //   cornerX/Y  : offset from blipPos to the outer corner tip
    //   hDir       : +1 = arm points right,  -1 = arm points left
    //   vDir       : +1 = arm points down,   -1 = arm points up
    auto drawCorner = [&](float cornerX, float cornerY, float hDir, float vDir) {
        sf::RectangleShape h({arm, thick});
        h.setOrigin({hDir > 0 ? 0.0f : arm, thick * 0.5f});
        h.setPosition(blipPos + sf::Vector2f(cornerX, cornerY));
        h.setFillColor(col);
        window.draw(h);

        sf::RectangleShape v({thick, arm});
        v.setOrigin({thick * 0.5f, vDir > 0 ? 0.0f : arm});
        v.setPosition(blipPos + sf::Vector2f(cornerX, cornerY));
        v.setFillColor(col);
        window.draw(v);
    };

    drawCorner(-s, -s, +1.0f, +1.0f);  // top-left
    drawCorner( s, -s, -1.0f, +1.0f);  // top-right
    drawCorner(-s,  s, +1.0f, -1.0f);  // bottom-left
    drawCorner( s,  s, -1.0f, -1.0f);  // bottom-right

    // Outer pulsing ring
    const float r = s * 1.3f;
    sf::CircleShape ring(r);
    ring.setOrigin({r, r});
    ring.setPosition(blipPos);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(1.5f);
    ring.setOutlineColor(sf::Color(col.r, col.g, col.b, 160));
    window.draw(ring);
}

// ---------------------------------------------------------------------------
// Lock timer arc — yellow arc that grows as lock progresses from 0→1
// ---------------------------------------------------------------------------
void RadarRenderer::drawLockTimerArc(sf::RenderWindow& window,
                                      const Simulation& simulation) const
{
    if (simulation.lockState() != LockState::Acquiring) return;

    const int lockedIndex = simulation.lockedEnemyIndex();
    if (lockedIndex < 0 || lockedIndex >= static_cast<int>(simulation.enemies().size()))
        return;

    const Enemy&       enemy     = simulation.enemies()[lockedIndex];
    const sf::Vector2f playerPos = simulation.player().position();
    if (!isInsideRadarRange(enemy.position(), playerPos)) return;

    const sf::Vector2f blipPos  = worldToRadar(enemy.position(), playerPos);
    const float        progress = simulation.lockProgress(); // 0.0 → 1.0
    const float        radius   = m_config.lockOnBracketSize * 1.6f;

    constexpr unsigned int segments = 64;
    const unsigned int usedSegments = static_cast<unsigned int>(
        static_cast<float>(segments) * progress);

    if (usedSegments < 2) return;

    sf::ConvexShape arc;
    arc.setPointCount(usedSegments + 1);

    for (unsigned int i = 0; i <= usedSegments; ++i) {
        const float t   = static_cast<float>(i) / static_cast<float>(segments);
        const float deg = -90.0f + t * 360.0f; // start from top
        const float rad = toRadians(deg);
        arc.setPoint(i, blipPos + sf::Vector2f(std::cos(rad) * radius,
                                                std::sin(rad) * radius));
    }

    arc.setFillColor(sf::Color::Transparent);
    arc.setOutlineThickness(2.5f);
    arc.setOutlineColor(m_config.acquiringColor);
    window.draw(arc);
}

// ---------------------------------------------------------------------------
// Missile ready indicator — flashing red ring + "READY" label when locked
// ---------------------------------------------------------------------------
void RadarRenderer::drawMissileReadyIndicator(sf::RenderWindow& window,
                                               const Simulation& simulation) const
{
    if (!simulation.missileReady()) return;

    const int lockedIndex = simulation.lockedEnemyIndex();
    if (lockedIndex < 0 || lockedIndex >= static_cast<int>(simulation.enemies().size()))
        return;

    const Enemy&       enemy     = simulation.enemies()[lockedIndex];
    const sf::Vector2f playerPos = simulation.player().position();
    if (!isInsideRadarRange(enemy.position(), playerPos)) return;

    const sf::Vector2f blipPos = worldToRadar(enemy.position(), playerPos);

    // Flash visibility: on/off at missileReadyFlashHz
    const float phase = simulation.totalTime() * m_config.missileReadyFlashHz;
    const bool  on    = (phase - std::floor(phase)) < 0.5f;
    if (!on) return;

    // Flashing red ring around target
    const float r = m_config.lockOnBracketSize * 1.8f;
    sf::CircleShape ring(r);
    ring.setOrigin({r, r});
    ring.setPosition(blipPos);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(2.5f);
    ring.setOutlineColor(m_config.missileReadyColor);
    window.draw(ring);

    // "READY" label beneath the target
    if (m_fontLoaded) {
        sf::Text readyLabel(m_font, "READY", 11);
        readyLabel.setFillColor(m_config.missileReadyColor);
        const float labelX = blipPos.x - 16.0f;
        const float labelY = blipPos.y + r + 4.0f;
        readyLabel.setPosition({labelX, labelY});
        window.draw(readyLabel);
    }
}

// ---------------------------------------------------------------------------
// HUD — version badge, lock-on readout, key hints
// ---------------------------------------------------------------------------
void RadarRenderer::drawHUD(sf::RenderWindow& window, const Simulation& simulation) const
{
    // Version badge (top-right, always drawn)
    {
        const float w = 130.0f, h = 28.0f;
        const float x = static_cast<float>(m_config.windowWidth)  - w - 10.0f;
        const float y = 10.0f;

        sf::RectangleShape badge({w, h});
        badge.setPosition({x, y});
        badge.setFillColor(sf::Color(0, 40, 15, 200));
        badge.setOutlineColor(m_config.radarGreen);
        badge.setOutlineThickness(1.0f);
        window.draw(badge);

        if (m_fontLoaded) {
            // SFML 3: sf::Text(font, string, characterSize)
            sf::Text ver(m_font, "JET-RADAR v0.2", 13);
            ver.setFillColor(m_config.radarGreen);
            ver.setPosition({x + 6.0f, y + 5.0f});
            window.draw(ver);
        }
    }

    if (!m_fontLoaded) return;

    // Key hint (bottom-left)
    {
        sf::Text hint(m_font, "[WASD] Move  [TAB] Lock-On  [SPACE] Fire  [ESC] Clear", 12);
        hint.setFillColor(sf::Color(0, 180, 50, 200));
        hint.setPosition({14.0f, static_cast<float>(m_config.windowHeight) - 26.0f});
        window.draw(hint);
    }

    // Lock-on info (top-left)
    const int lockedIndex = simulation.lockedEnemyIndex();
    if (lockedIndex >= 0 && lockedIndex < static_cast<int>(simulation.enemies().size())) {
        const Enemy&       enemy     = simulation.enemies()[lockedIndex];
        const sf::Vector2f playerPos = simulation.player().position();
        const float        dist      = distanceBetween(enemy.position(), playerPos);

        std::ostringstream oss;
        oss << "LOCKED: TGT-" << (lockedIndex + 1) << "\n"
            << "RANGE : " << std::fixed << std::setprecision(0) << dist << " m";

        if (simulation.missileReady()) {
            oss << "\n>>> MISSILE READY <<<";
        }

        sf::Text info(m_font, oss.str(), 14);
        info.setFillColor(simulation.missileReady() ? m_config.missileReadyColor
                                                     : m_config.lockOnColor);
        info.setPosition({14.0f, 14.0f});
        window.draw(info);
    } else {
        sf::Text info(m_font, "NO LOCK", 14);
        info.setFillColor(sf::Color(160, 160, 160));
        info.setPosition({14.0f, 14.0f});
        window.draw(info);
    }
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

sf::Vector2f RadarRenderer::radarCenter() const
{
    return {
        static_cast<float>(m_config.windowWidth)  * 0.5f,
        static_cast<float>(m_config.windowHeight) * 0.5f,
    };
}

sf::Vector2f RadarRenderer::worldToRadar(sf::Vector2f worldPos,
                                          sf::Vector2f playerPos) const
{
    const sf::Vector2f relative = worldPos - playerPos;
    const float        scale    = m_config.radarRadius / m_config.radarRange;
    return radarCenter() + sf::Vector2f(relative.x * scale, relative.y * scale);
}

bool RadarRenderer::isInsideRadarRange(sf::Vector2f worldPos,
                                        sf::Vector2f playerPos) const
{
    return vecLength(worldPos - playerPos) <= m_config.radarRange;
}

float RadarRenderer::distanceBetween(sf::Vector2f a, sf::Vector2f b) const
{
    return vecLength(a - b);
}
