#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"

class UI {
public:
    sf::Font font;
    bool     fontLoaded = false;

    void init(const std::string& fontPath);
    void draw(sf::RenderWindow& window, const World& world, bool paused);

private:
    void drawCrosshair(sf::RenderWindow& w);
    void drawWeaponBar(sf::RenderWindow& w, const World& world);
    void drawDestructionMeter(sf::RenderWindow& w, float pct);
    void drawVehicleHUD(sf::RenderWindow& w, const World& world);
    void drawControls(sf::RenderWindow& w, const World& world);
    void drawAchievementPopups(sf::RenderWindow& w, const World& world);
    void drawAchievementList(sf::RenderWindow& w, const World& world);
    void drawPauseMenu(sf::RenderWindow& w, const World& world);
    void drawStats(sf::RenderWindow& w, const World& world);

    sf::Text makeText(const std::string& s, unsigned sz,
                      sf::Color col = sf::Color::White);
    void drawRoundedRect(sf::RenderWindow& w, float x, float y,
                         float width, float height, sf::Color fill,
                         sf::Color outline = sf::Color::Transparent, float thick = 0.f);
};
