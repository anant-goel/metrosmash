#pragma once
// NO SFML includes here - forward declare only
#include "World.h"
#include <string>

namespace sf { class RenderWindow; class Font; }

class UI {
public:
    void init(const std::string& fontPath);
    void draw(sf::RenderWindow& window, const World& world, bool paused);
private:
    // Font stored via PIMPL to avoid SFML header in this file
    struct Impl;
    std::unique_ptr<Impl> impl;
};
