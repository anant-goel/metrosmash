#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"
#include <memory>
#include <string>

class UI {
public:
    UI();
    ~UI();

    void init(const std::string& fontPath);
    void draw(sf::RenderWindow& window, const World& world, bool paused);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    sf::Text makeText(const std::string& s, unsigned sz, sf::Color col);
    void drawRoundedRect(sf::RenderWindow& w, float x, float y,
                         float width, float height, sf::Color fill,
                         sf::Color outline = sf::Color::Transparent,
                         float thick = 0.f);
    void drawCrosshair(sf::RenderWindow& w);
    void drawWeaponBar(sf::RenderWindow& w, const World& world);
    void drawDestructionMeter(sf::RenderWindow& w, float pct);
    void drawVehicleHUD(sf::RenderWindow& w, const World& world);
    void drawStats(sf::RenderWindow& w, const World& world);
    void drawControls(sf::RenderWindow& w, const World& world);
    void drawAchievementPopups(sf::RenderWindow& w, const World& world);
    void drawPauseMenu(sf::RenderWindow& w, const World& world);
};
