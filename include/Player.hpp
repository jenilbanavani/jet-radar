#pragma once

#include "Entity.hpp"

#include <SFML/System/Vector2.hpp>

class Player final : public Entity {
public:
    void update(float) override {}

    sf::Vector2f position() const override
    {
        return {0.0f, 0.0f};
    }
};
