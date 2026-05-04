#include "pch.h"
#include "Game.h"
#include <SFML/Graphics.hpp>
#include <GL/gl.h>
#include <functional>
#include <cmath>

void Game::run() {
    init();
    while (running && window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;
        handleEvents();
        if (!paused) {
            accumulator += dt;
            while (accumulator >= FIXED_DT) {
                handleInput(FIXED_DT);
                update(FIXED_DT);
                accumulator -= FIXED_DT;
            }
        }
        render();
    }
}

void Game::init() {
    sf::ContextSettings settings;
    settings.depthBits = 24; settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 2; settings.minorVersion = 1;

    window.create(sf::VideoMode({1280, 720}),
                  "METRO SMASH v3.0 -- Ultimate Destruction Sandbox",
                  sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(120);

    renderer.init(1280, 720);
    camera.aspect = 1280.f / 720.f;
    ui.init("assets/fonts/GameFont.ttf");

    world.init("assets");
    lockMouse();
}

void Game::lockMouse() {
    window.setMouseCursorVisible(false);
    sf::Mouse::setPosition(sf::Vector2i(window.getSize().x/2, window.getSize().y/2), window);
    mouseLocked = true;
}
void Game::unlockMouse() { window.setMouseCursorVisible(true); mouseLocked = false; }

void Game::handleEvents() {
    while (auto ev = window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) { running = false; }
        else if (const auto* r = ev->getIf<sf::Event::Resized>()) {
            renderer.resize(r->size.x, r->size.y);
            camera.aspect = (float)r->size.x / (float)r->size.y;
        }
        else if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) { onKeyPressed(k->code); }
        else if (ev->is<sf::Event::FocusLost>()) { unlockMouse(); paused = true; }
        else if (const auto* m = ev->getIf<sf::Event::MouseMoved>()) {
            if (mouseLocked) {
                int cx = window.getSize().x/2, cy = window.getSize().y/2;
                int dx = m->position.x - cx, dy = m->position.y - cy;
                if (dx || dy) { onMouseMoved(dx, dy); sf::Mouse::setPosition(sf::Vector2i(cx, cy), window); }
            }
        }
        else if (const auto* w = ev->getIf<sf::Event::MouseWheelScrolled>()) {
            if (w->delta > 0) world.weapons.selectNext();
            else              world.weapons.selectPrev();
            world.audio.play("ui_click", 60.f);
        }
    }
}

void Game::onKeyPressed(sf::Keyboard::Key key) {
    using K = sf::Keyboard::Key;
    switch (key) {
    case K::Escape: paused=!paused; paused?unlockMouse():lockMouse(); break;
    case K::Enter:  if(paused){paused=false;lockMouse();} break;
    case K::R:      world.rebuild(); break;

    // Fire current weapon
    case K::E: {
        if (!paused) {
            Vec3 dir = world.player.getForward();
            world.weapons.fire(world.player.position, dir);
            world.audio.play(world.weapons.selected().fireSound, 90.f);
        }
        break;
    }
    // Detonate manual weapons (C4, gravity bomb)
    case K::Q:
        if (!paused) {
            world.weapons.detonateManual();
            world.audio.play("c4_detonate", 100.f);
        }
        break;

    // Weapon cycle
    case K::Tab:
        world.weapons.selectNext();
        world.audio.play("ui_click", 60.f);
        break;

    // Vehicle
    case K::F:
        if (!paused) {
            if (world.player.mode == PlayerMode::ON_FOOT) {
                Vehicle* v = world.getNearbyVehicle(world.player.position);
                if (v && !v->occupied) {
                    v->occupied = true;
                    world.player.enterVehicle(v->position);
                    world.player.position = v->position;
                    world.audio.play("vehicle_placed", 80.f);
                    world.achievements.check(AchievementID::ENTER_VEHICLE);
                    if (v->type == VehicleType::TANK)
                        world.achievements.check(AchievementID::TANK_COMMANDER);
                }
            } else {
                for (auto& v : world.vehicles) {
                    if (v->occupied) {
                        v->occupied = false;
                        Vec3 exitPos = v->position + v->getRight() * 3.f;
                        exitPos.y = 1.f;
                        world.player.position = exitPos;
                        world.player.body.position = exitPos;
                        world.player.exitVehicle();
                        world.audio.play("car_engine", 40.f);
                        break;
                    }
                }
            }
        }
        break;

    // Number keys for direct weapon select
    case K::Num1: world.weapons.selectedIdx = 0; break;
    case K::Num2: if(world.weapons.defs[1].unlocked) world.weapons.selectedIdx = 1; break;
    case K::Num3: if(world.weapons.defs[2].unlocked) world.weapons.selectedIdx = 2; break;
    case K::Num4: if(world.weapons.defs[3].unlocked) world.weapons.selectedIdx = 3; break;
    case K::Num5: if(world.weapons.defs[4].unlocked) world.weapons.selectedIdx = 4; break;
    case K::Num6: if(world.weapons.defs[5].unlocked) world.weapons.selectedIdx = 5; break;
    case K::Num7: if(world.weapons.defs[6].unlocked) world.weapons.selectedIdx = 6; break;
    case K::Num8: if(world.weapons.defs[7].unlocked) world.weapons.selectedIdx = 7; break;
    case K::Num9: if(world.weapons.defs[8].unlocked) world.weapons.selectedIdx = 8; break;

    default: break;
    }
}

void Game::onMouseMoved(int dx, int dy) {
    float sensitivity = 0.002f;
    camera.rotate(dx * sensitivity, dy * sensitivity);
    if (world.player.mode == PlayerMode::ON_FOOT)
        world.player.yaw = camera.yaw;
}

void Game::handleInput(float dt) {
    if (paused) return;
    using K = sf::Keyboard::Key;
    Player& p = world.player;
    if (p.mode == PlayerMode::ON_FOOT) {
        p.moveF   = sf::Keyboard::isKeyPressed(K::W);
        p.moveB   = sf::Keyboard::isKeyPressed(K::S);
        p.moveL   = sf::Keyboard::isKeyPressed(K::A);
        p.moveR   = sf::Keyboard::isKeyPressed(K::D);
        p.jumping = sf::Keyboard::isKeyPressed(K::Space);
    } else {
        for (auto& v : world.vehicles) {
            if (!v->occupied) continue;
            bool accel = sf::Keyboard::isKeyPressed(K::W);
            bool brake = sf::Keyboard::isKeyPressed(K::S);
            float steer = 0.f;
            if (sf::Keyboard::isKeyPressed(K::A)) steer = -1.f;
            if (sf::Keyboard::isKeyPressed(K::D)) steer =  1.f;
            v->update(dt, accel, brake, steer);
            p.position = v->position; p.body.position = v->position;

            if (std::abs(v->speed) > v->maxSpeed * 0.95f)
                world.achievements.check(AchievementID::SPEED_DEMON);
            break;
        }
    }
}

void Game::update(float dt) {
    world.update(dt);
    Vec3 target = world.player.mode == PlayerMode::ON_FOOT ? world.player.position :
        [&]() -> Vec3 {
            for (auto& v : world.vehicles) if (v->occupied) return v->position;
            return world.player.position;
        }();

    // Apply screen shake to camera offset
    Vec3 shakeOff = world.anim.shake.getOffset();
    camera.followTarget(target);
    camera.position += shakeOff;
}

void Game::render() {
    window.setActive(true);
    renderer.render(world, camera, window);
    ui.draw(window, world, paused);
    window.display();
}
