#pragma once

#include "Enemy.hpp"
#include "Player.hpp"
#include "RadarConfig.hpp"

#include <vector>

class Simulation {
public:
    explicit Simulation(const RadarConfig& config);

    void update(float deltaSeconds);

    const Player& player() const;
    const std::vector<Enemy>& enemies() const;
    float scanAngleDegrees() const;
    const RadarConfig& config() const;

private:
    void spawnEnemies();

    RadarConfig m_config;
    Player m_player;
    std::vector<Enemy> m_enemies;
    float m_scanAngleDegrees = 0.0f;
};
