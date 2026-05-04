#pragma once
// NO SFML includes here - all SFML types forward declared or in .cpp
#include "World.h"
#include "Camera.h"
#include <memory>

namespace sf { class RenderWindow; class Clock; }
// Renderer and UI are included in Game.cpp only
class Renderer;
class UI;

class Game {
public:
    Game();
    ~Game();

    void run();
private:
    // Use unique_ptr for SFML/Renderer/UI types so headers stay clean
    struct Impl;
    std::unique_ptr<Impl> impl;

    World  world;
    Camera camera;

    bool  running     = true;
    bool  paused      = false;
    bool  mouseLocked = true;
    float accumulator = 0.f;
    static constexpr float FIXED_DT = 1.f / 60.f;

    void init();
    void handleEvents();
    void handleInput(float dt);
    void update(float dt);
    void render();
    void lockMouse();
    void unlockMouse();
    void onKeyPressed(int key);
    void onMouseMoved(int dx, int dy);
};
