#pragma once

#include "Entity.hpp"

#include <SFML/System/Vector2.hpp>

class Enemy final : public Entity {
public:
    Enemy(sf::Vector2f startPosition, sf::Vector2f velocity, float worldLimit);

    void update(float deltaSeconds) override;
    sf::Vector2f position() const override;

private:
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    float m_worldLimit;
};
