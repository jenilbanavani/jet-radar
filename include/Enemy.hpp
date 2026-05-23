#pragma once

#include "Entity.hpp"

#include <SFML/System/Vector2.hpp>

class Enemy final : public Entity {
public:
    Enemy(sf::Vector2f startPosition, sf::Vector2f velocity, float worldLimit);

    void update(float deltaSeconds) override;
    sf::Vector2f position() const override;

    // Detection pulse state — set by Simulation when the sweep touches this enemy
    float pulseTimer() const { return m_pulseTimer; }
    void  triggerPulse(float duration);
    void  tickPulse(float deltaSeconds);

private:
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    float        m_worldLimit;
    float        m_pulseTimer  = 0.0f;   // counts down from pulseDuration to 0
    float        m_pulseDuration = 1.4f;
};
