#include "Enemy.hpp"

#include <algorithm>

Enemy::Enemy(sf::Vector2f startPosition, sf::Vector2f velocity, float worldLimit)
    : m_position(startPosition)
    , m_velocity(velocity)
    , m_worldLimit(worldLimit)
{
}

void Enemy::update(float deltaSeconds)
{
    m_position += m_velocity * deltaSeconds;

    if (m_position.x < -m_worldLimit || m_position.x > m_worldLimit) {
        m_position.x = std::clamp(m_position.x, -m_worldLimit, m_worldLimit);
        m_velocity.x = -m_velocity.x;
    }

    if (m_position.y < -m_worldLimit || m_position.y > m_worldLimit) {
        m_position.y = std::clamp(m_position.y, -m_worldLimit, m_worldLimit);
        m_velocity.y = -m_velocity.y;
    }
}

sf::Vector2f Enemy::position() const
{
    return m_position;
}
