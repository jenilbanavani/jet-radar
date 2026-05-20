#include "RadarRenderer.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>
#include <cmath>

namespace {
constexpr float pi = 3.14159265359f;

float length(sf::Vector2f value)
{
    return std::sqrt(value.x * value.x + value.y * value.y);
}

float degreesToRadians(float degrees)
{
    return degrees * pi / 180.0f;
}
}

RadarRenderer::RadarRenderer(RadarConfig config)
    : m_config(config)
{
}

void RadarRenderer::render(sf::RenderWindow& window, const Simulation& simulation) const
{
    const auto center = radarCenter();

    sf::CircleShape outer(m_config.radarRadius);
    outer.setOrigin(m_config.radarRadius, m_config.radarRadius);
    outer.setPosition(center);
    outer.setFillColor(sf::Color::Transparent);
    outer.setOutlineColor(m_config.radarGreen);
    outer.setOutlineThickness(2.0f);
    window.draw(outer);

    for (int ring = 1; ring <= 3; ++ring) {
        const float radius = m_config.radarRadius * static_cast<float>(ring) / 4.0f;
        sf::CircleShape circle(radius);
        circle.setOrigin(radius, radius);
        circle.setPosition(center);
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineColor(sf::Color(0, 120, 35, 150));
        circle.setOutlineThickness(1.0f);
        window.draw(circle);
    }

    sf::RectangleShape horizontal({m_config.radarRadius * 2.0f, 1.0f});
    horizontal.setOrigin(m_config.radarRadius, 0.5f);
    horizontal.setPosition(center);
    horizontal.setFillColor(sf::Color(0, 130, 40, 150));
    window.draw(horizontal);

    sf::RectangleShape vertical({1.0f, m_config.radarRadius * 2.0f});
    vertical.setOrigin(0.5f, m_config.radarRadius);
    vertical.setPosition(center);
    vertical.setFillColor(sf::Color(0, 130, 40, 150));
    window.draw(vertical);

    const float scanRadians = degreesToRadians(simulation.scanAngleDegrees());
    sf::RectangleShape scanLine({m_config.radarRadius, 3.0f});
    scanLine.setOrigin(0.0f, 1.5f);
    scanLine.setPosition(center);
    scanLine.setRotation(simulation.scanAngleDegrees());
    scanLine.setFillColor(sf::Color(80, 255, 120, 220));
    window.draw(scanLine);

    sf::CircleShape scanGlow(m_config.radarRadius);
    scanGlow.setOrigin(m_config.radarRadius, m_config.radarRadius);
    scanGlow.setPosition(center);
    scanGlow.setPointCount(96);
    scanGlow.setFillColor(sf::Color::Transparent);
    scanGlow.setOutlineThickness(1.0f);
    scanGlow.setOutlineColor(sf::Color(
        50,
        static_cast<sf::Uint8>(170 + 60 * std::max(0.0f, std::sin(scanRadians))),
        75,
        90));
    window.draw(scanGlow);

    sf::CircleShape playerBlip(m_config.playerBlipRadius);
    playerBlip.setOrigin(m_config.playerBlipRadius, m_config.playerBlipRadius);
    playerBlip.setPosition(center);
    playerBlip.setFillColor(sf::Color(120, 255, 150));
    window.draw(playerBlip);

    const auto playerPosition = simulation.player().position();
    for (const Enemy& enemy : simulation.enemies()) {
        if (!isInsideRadarRange(enemy.position(), playerPosition)) {
            continue;
        }

        sf::CircleShape blip(m_config.enemyBlipRadius);
        blip.setOrigin(m_config.enemyBlipRadius, m_config.enemyBlipRadius);
        blip.setPosition(worldToRadar(enemy.position(), playerPosition));
        blip.setFillColor(sf::Color(255, 75, 75));
        window.draw(blip);
    }
}

sf::Vector2f RadarRenderer::radarCenter() const
{
    return {
        static_cast<float>(m_config.windowWidth) * 0.5f,
        static_cast<float>(m_config.windowHeight) * 0.5f,
    };
}

sf::Vector2f RadarRenderer::worldToRadar(sf::Vector2f worldPosition, sf::Vector2f playerPosition) const
{
    const sf::Vector2f relative = worldPosition - playerPosition;
    const float scale = m_config.radarRadius / m_config.radarRange;
    return radarCenter() + sf::Vector2f(relative.x * scale, relative.y * scale);
}

bool RadarRenderer::isInsideRadarRange(sf::Vector2f worldPosition, sf::Vector2f playerPosition) const
{
    return length(worldPosition - playerPosition) <= m_config.radarRange;
}
