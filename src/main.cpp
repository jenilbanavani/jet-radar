#include "RadarConfig.hpp"
#include "RadarRenderer.hpp"
#include "Simulation.hpp"

#include <SFML/Graphics.hpp>

int main()
{
    const RadarConfig config;

    sf::RenderWindow window(
        sf::VideoMode(config.windowWidth, config.windowHeight),
        "2D Radar Simulation",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    Simulation simulation(config);
    RadarRenderer renderer(config);
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event event {};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
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
