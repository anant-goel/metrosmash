#include "Game.h"
#include <SFML/OpenGL.hpp>
#include <functional>
#include <cmath>

void Game::run() {
    init();

    while (running && window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f; // cap dt

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
    settings.depthBits   = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 2;
    settings.minorVersion = 1;

    window.create(sf::VideoMode(1280, 720), "METRO SMASH v3.0 -- Ultimate Destruction Sandbox",
                  sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(120);

    renderer.init(1280, 720);
    camera.aspect = 1280.f / 720.f;
    ui.init();

    world.init();
    lockMouse();
}

void Game::lockMouse() {
    window.setMouseCursorVisible(false);
    sf::Mouse::setPosition(
        sf::Vector2i(window.getSize().x/2, window.getSize().y/2), window);
    mouseLocked = true;
}

void Game::unlockMouse() {
    window.setMouseCursorVisible(true);
    mouseLocked = false;
}

void Game::handleEvents() {
    sf::Event ev;
    while (window.pollEvent(ev)) {
        switch (ev.type) {
        case sf::Event::Closed:
            running = false;
            break;

        case sf::Event::Resized:
            renderer.resize(ev.size.width, ev.size.height);
            camera.aspect = (float)ev.size.width / (float)ev.size.height;
            break;

        case sf::Event::KeyPressed:
            onKeyPressed(ev.key.code);
            break;

        case sf::Event::LostFocus:
            unlockMouse();
            paused = true;
            break;

        case sf::Event::GainedFocus:
            break;

        case sf::Event::MouseMoved:
            if (mouseLocked) {
                int cx = window.getSize().x / 2;
                int cy = window.getSize().y / 2;
                int dx = ev.mouseMove.x - cx;
                int dy = ev.mouseMove.y - cy;
                if (dx != 0 || dy != 0) {
                    onMouseMoved(dx, dy);
                    sf::Mouse::setPosition(sf::Vector2i(cx, cy), window);
                }
            }
            break;

        default:
            break;
        }
    }
}

void Game::onKeyPressed(sf::Keyboard::Key key) {
    using K = sf::Keyboard;

    switch (key) {
    case K::Escape:
        paused = !paused;
        if (paused) unlockMouse();
        else        lockMouse();
        break;

    case K::Enter:
        if (paused) { paused = false; lockMouse(); }
        break;

    case K::R:
        world.rebuild();
        break;

    case K::E:
        if (!paused) world.player.placeExplosive();
        break;

    case K::Q:
        if (!paused) {
            std::vector<RigidBody*> bodies;
            // Collect bodies from all buildings
            for (auto& b : world.buildings) b.collectBodies(bodies);
            for (auto& v : world.vehicles)  v->collectBody(bodies);

            world.player.detonateAll(bodies,
                [&](Vec3 pos, float radius, float strength){
                    world.explodeAt(pos, radius, strength);
                });
        }
        break;

    case K::F:
        if (!paused) {
            if (world.player.mode == PlayerMode::ON_FOOT) {
                Vehicle* v = world.getNearbyVehicle(world.player.position);
                if (v && !v->occupied) {
                    v->occupied = true;
                    world.player.enterVehicle(v->position);
                    world.player.position = v->position;
                }
            } else {
                // Exit vehicle
                for (auto& v : world.vehicles) {
                    if (v->occupied) {
                        v->occupied = false;
                        Vec3 exitPos = v->position + v->getRight() * 3.f;
                        exitPos.y = 1.f;
                        world.player.position = exitPos;
                        world.player.body.position = exitPos;
                        world.player.exitVehicle();
                        break;
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}

void Game::onMouseMoved(int dx, int dy) {
    float sensitivity = 0.002f;
    if (world.player.mode == PlayerMode::IN_VEHICLE) {
        camera.rotate(dx * sensitivity, dy * sensitivity);
    } else {
        camera.rotate(dx * sensitivity, dy * sensitivity);
        world.player.yaw = camera.yaw;
    }
}

void Game::handleInput(float dt) {
    if (paused) return;
    using K = sf::Keyboard;

    Player& p = world.player;

    if (p.mode == PlayerMode::ON_FOOT) {
        p.moveF = sf::Keyboard::isKeyPressed(K::W);
        p.moveB = sf::Keyboard::isKeyPressed(K::S);
        p.moveL = sf::Keyboard::isKeyPressed(K::A);
        p.moveR = sf::Keyboard::isKeyPressed(K::D);
        p.jumping = sf::Keyboard::isKeyPressed(K::Space);
    } else {
        // Find occupied vehicle
        for (auto& v : world.vehicles) {
            if (!v->occupied) continue;
            bool accel = sf::Keyboard::isKeyPressed(K::W);
            bool brake = sf::Keyboard::isKeyPressed(K::S);
            float steer = 0.f;
            if (sf::Keyboard::isKeyPressed(K::A)) steer = -1.f;
            if (sf::Keyboard::isKeyPressed(K::D)) steer =  1.f;

            v->update(dt, accel, brake, steer);
            p.position = v->position;
            p.body.position = v->position;
            break;
        }
    }
}

void Game::update(float dt) {
    world.update(dt);

    // Camera follow
    if (world.player.mode == PlayerMode::ON_FOOT) {
        camera.followTarget(world.player.position);
    } else {
        // Follow occupied vehicle
        for (const auto& v : world.vehicles) {
            if (v->occupied) {
                camera.followTarget(v->position);
                break;
            }
        }
    }
}

void Game::render() {
    window.setActive(true);
    renderer.render(world, camera, window);
    ui.draw(window, world, paused);
    window.display();
}
