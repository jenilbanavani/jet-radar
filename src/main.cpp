// =============================================================================
//  main.cpp  —  JET-RADAR v0.3
//  SFML 3.x compatible
//
//  Controls:
//    WASD / Arrow Keys — move the player
//    TAB  — cycle lock-on through visible enemies
//    SPACE — fire missile (when locked)
//    ESC  — clear lock (or close if no lock active)
// =============================================================================

#include "RadarConfig.hpp"
#include "RadarRenderer.hpp"
#include "Simulation.hpp"

#include <SFML/Graphics.hpp>
#include <cmath>

int main()
{
    const RadarConfig config;

    // SFML 3: VideoMode takes sf::Vector2u{width, height}
    sf::RenderWindow window(
        sf::VideoMode({config.windowWidth, config.windowHeight}),
        "JET-RADAR v0.3",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    Simulation    simulation(config);
    RadarRenderer renderer(config);

    // Try to load a monospace font from common Windows paths
    const std::vector<std::string> fontPaths = {
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/cour.ttf",
        "C:/Windows/Fonts/arial.ttf",
    };
    for (const auto& path : fontPaths) {
        if (renderer.loadFont(path)) break;
    }

    sf::Clock clock;

    while (window.isOpen()) {
        // SFML 3: pollEvent returns std::optional<sf::Event>
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Tab) {
                    simulation.cycleLockOn();
                } else if (kp->code == sf::Keyboard::Key::Space) {
                    simulation.fireMissile();
                } else if (kp->code == sf::Keyboard::Key::Escape) {
                    if (simulation.lockedEnemyIndex() >= 0) {
                        simulation.clearLockOn();
                    } else {
                        window.close();
                    }
                }
            }
        }

        // --- Continuous input: WASD / Arrow keys for player movement ---
        sf::Vector2f moveDir{0.0f, 0.0f};

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            moveDir.y -= 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            moveDir.y += 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            moveDir.x -= 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            moveDir.x += 1.0f;
        }

        // Normalise diagonal movement so speed is consistent
        const float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (len > 0.0f) {
            moveDir.x = (moveDir.x / len) * config.playerSpeed;
            moveDir.y = (moveDir.y / len) * config.playerSpeed;
        }
        simulation.setPlayerVelocity(moveDir);

        const float deltaSeconds = clock.restart().asSeconds();
        simulation.update(deltaSeconds);

        window.clear(sf::Color::Black);
        renderer.render(window, simulation);
        window.display();
    }

    return 0;
}
