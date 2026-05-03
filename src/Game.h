#pragma once
#include <SFML/Graphics.hpp>
#include "World.h"
#include "Renderer.h"
#include "Camera.h"
#include "UI.h"

class Game {
public:
    void run();

private:
    sf::RenderWindow window;
    World            world;
    Renderer         renderer;
    Camera           camera;
    UI               ui;

    bool  running = true;
    bool  paused  = false;
    bool  mouseLocked = true;

    sf::Clock clock;
    float     accumulator = 0.f;
    static constexpr float FIXED_DT = 1.f / 60.f;

    // Driving state
    bool   driveAccel = false, driveBrake = false;
    float  driveSteering = 0.f;

    void init();
    void handleEvents();
    void handleInput(float dt);
    void update(float dt);
    void render();

    void lockMouse();
    void unlockMouse();

    // Input helpers
    void onKeyPressed(sf::Keyboard::Key key);
    void onMouseMoved(int dx, int dy);
};
