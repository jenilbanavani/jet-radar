// =============================================================================
//  main.cpp  —  ROCKET TRACKING SYSTEM v1.0
//  SFML 3.x compatible
//
//  Controls:
//    TAB   — cycle through rockets
//    SPACE — launch selected rocket
//    L     — launch all rockets
//    ESC   — exit
// =============================================================================

#include "RadarConfig.hpp"
#include "TelemetryRenderer.hpp"
#include "RocketSimulation.hpp"

#include <SFML/Graphics.hpp>

int main()
{
    const RadarConfig config;

    // SFML 3: VideoMode takes sf::Vector2u{width, height}
    sf::RenderWindow window(
        sf::VideoMode({config.windowWidth, config.windowHeight}),
        "ROCKET TRACKING SYSTEM v1.0",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    RocketSimulation  simulation(config);
    TelemetryRenderer renderer(config);

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
                    simulation.cycleSelection();
                } else if (kp->code == sf::Keyboard::Key::Space) {
                    simulation.launchSelected();
                } else if (kp->code == sf::Keyboard::Key::L) {
                    simulation.launchAll();
                } else if (kp->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }

        const float deltaSeconds = clock.restart().asSeconds();
        simulation.update(deltaSeconds);

        window.clear(sf::Color::Black);
        renderer.render(window, simulation);
        window.display();
    }

    return 0;
}
