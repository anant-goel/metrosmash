#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"

class UI {
public:
    sf::Font font;
    bool     fontLoaded = false;

    void init();
    void draw(sf::RenderWindow& window, const World& world, bool paused);

private:
    void drawHUD(sf::RenderWindow& w, const World& world);
    void drawCrosshair(sf::RenderWindow& w);
    void drawExplosiveBar(sf::RenderWindow& w, int count);
    void drawDestructionMeter(sf::RenderWindow& w, float pct);
    void drawVehicleInfo(sf::RenderWindow& w, const World& world);
    void drawControls(sf::RenderWindow& w, const World& world);
    void drawPauseMenu(sf::RenderWindow& w);

    // SFML 3: font is required in Text constructor
    sf::Text makeText(const std::string& s, unsigned size,
                      sf::Color color = sf::Color::White);
};
