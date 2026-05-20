#include "Simulation.hpp"

#include <cmath>

Simulation::Simulation(const RadarConfig& config)
    : m_config(config)
{
    spawnEnemies();
}

void Simulation::update(float deltaSeconds)
{
    m_player.update(deltaSeconds);

    for (Enemy& enemy : m_enemies) {
        enemy.update(deltaSeconds);
    }

    m_scanAngleDegrees += m_config.scanDegreesPerSecond * deltaSeconds;
    if (m_scanAngleDegrees >= 360.0f) {
        m_scanAngleDegrees = std::fmod(m_scanAngleDegrees, 360.0f);
    }
}

const Player& Simulation::player() const
{
    return m_player;
}

const std::vector<Enemy>& Simulation::enemies() const
{
    return m_enemies;
}

float Simulation::scanAngleDegrees() const
{
    return m_scanAngleDegrees;
}

const RadarConfig& Simulation::config() const
{
    return m_config;
}

void Simulation::spawnEnemies()
{
    constexpr float worldLimit = 1400.0f;

    m_enemies.emplace_back(sf::Vector2f(-850.0f, -460.0f), sf::Vector2f(120.0f, 65.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f(260.0f, -720.0f), sf::Vector2f(-75.0f, 135.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f(1030.0f, 190.0f), sf::Vector2f(-95.0f, -45.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f(-380.0f, 880.0f), sf::Vector2f(80.0f, -115.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f(1180.0f, -1030.0f), sf::Vector2f(-140.0f, 90.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f(-1220.0f, 640.0f), sf::Vector2f(105.0f, -70.0f), worldLimit);
}
