#pragma once

#include <SFML/System/Vector2.hpp>

class Entity {
public:
    virtual ~Entity() = default;

    virtual void update(float deltaSeconds) = 0;
    virtual sf::Vector2f position() const = 0;
};
