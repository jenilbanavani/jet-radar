#pragma once

#include "Entity.hpp"

#include <SFML/System/Vector2.hpp>

class Player final : public Entity {
public:
    void update(float deltaSeconds) override
    {
        m_position += m_velocity * deltaSeconds;
    }

    sf::Vector2f position() const override
    {
        return m_position;
    }

    void setVelocity(sf::Vector2f velocity) { m_velocity = velocity; }
    sf::Vector2f velocity() const { return m_velocity; }

private:
    sf::Vector2f m_position{0.0f, 0.0f};
    sf::Vector2f m_velocity{0.0f, 0.0f};
};
