// ALL SFML includes go here first, before anything else in this TU
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/OpenGL.hpp>

#include "Game.h"
#include "Renderer.h"
#include "UI.h"
#include <cmath>
#include <functional>

// PIMPL struct owns SFML window, clock, renderer, UI
struct Game::Impl {
    sf::RenderWindow window;
    sf::Clock        clock;
    Renderer         renderer;
    UI               ui;
};

void Game::run() {
    init();
    while (running && impl->window.isOpen()) {
        float dt = impl->clock.restart().asSeconds();
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
    impl = std::make_unique<Impl>();

    sf::ContextSettings settings;
    settings.depthBits = 24; settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 2; settings.minorVersion = 1;

    impl->window.create(sf::VideoMode(1280, 720),
        "METRO SMASH v3.0 -- Ultimate Destruction Sandbox",
        sf::Style::Default, settings);
    impl->window.setVerticalSyncEnabled(true);
    impl->window.setFramerateLimit(120);

    impl->renderer.init(1280, 720);
    camera.aspect = 1280.f / 720.f;
    impl->ui.init("assets/fonts/GameFont.ttf");
    world.init("assets");
    lockMouse();
}

void Game::lockMouse() {
    impl->window.setMouseCursorVisible(false);
    sf::Mouse::setPosition(
        sf::Vector2i(impl->window.getSize().x/2, impl->window.getSize().y/2),
        impl->window);
    mouseLocked = true;
}
void Game::unlockMouse() {
    impl->window.setMouseCursorVisible(true);
    mouseLocked = false;
}

void Game::handleEvents() {
    sf::Event ev;
    while (impl->window.pollEvent(ev)) {
        if (ev.type == sf::Event::Closed) {
            running = false;
        }
        else if (ev.type == sf::Event::Resized) {
            impl->renderer.resize(ev.size.width, ev.size.height);
            camera.aspect = (float)ev.size.width / (float)ev.size.height;
        }
        else if (ev.type == sf::Event::KeyPressed) {
            onKeyPressed((int)ev.key.code);
        }
        else if (ev.type == sf::Event::LostFocus) {
            unlockMouse();
            paused = true;
        }
        else if (ev.type == sf::Event::MouseMoved) {
            if (mouseLocked) {
                int cx = impl->window.getSize().x / 2;
                int cy = impl->window.getSize().y / 2;
                int dx = ev.mouseMove.x - cx;
                int dy = ev.mouseMove.y - cy;
                if (dx || dy) {
                    onMouseMoved(dx, dy);
                    sf::Mouse::setPosition(sf::Vector2i(cx, cy), impl->window);
                }
            }
        }
        else if (ev.type == sf::Event::MouseWheelScrolled) {
            if (ev.mouseWheelScroll.delta > 0) world.weapons.selectNext();
            else                              world.weapons.selectPrev();
            world.audio.play("ui_click", 60.f);
        }
    }
}

void Game::onKeyPressed(int k) {
    using K = sf::Keyboard::Key;
    auto key = (K)k;
    switch (key) {
    case K::Escape: paused=!paused; paused?unlockMouse():lockMouse(); break;
    case K::Enter:  if(paused){paused=false;lockMouse();} break;
    case K::R:      world.rebuild(); break;
    case K::E:
        if (!paused) {
            world.weapons.fire(world.player.position, world.player.getForward());
            world.audio.play(world.weapons.selected().fireSound, 90.f);
        }
        break;
    case K::Q:
        if (!paused) { world.weapons.detonateManual(); world.audio.play("c4_detonate",100.f); }
        break;
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
                        Vec3 ep = v->position + v->getRight()*3.f; ep.y=1.f;
                        world.player.position = ep;
                        world.player.body.position = ep;
                        world.player.exitVehicle();
                        break;
                    }
                }
            }
        }
        break;
    case K::Tab: world.weapons.selectNext(); world.audio.play("ui_click",60.f); break;
    case K::Num1: world.weapons.selectedIdx=0; break;
    case K::Num2: if(world.weapons.defs[1].unlocked) world.weapons.selectedIdx=1; break;
    case K::Num3: if(world.weapons.defs[2].unlocked) world.weapons.selectedIdx=2; break;
    case K::Num4: if(world.weapons.defs[3].unlocked) world.weapons.selectedIdx=3; break;
    case K::Num5: if(world.weapons.defs[4].unlocked) world.weapons.selectedIdx=4; break;
    case K::Num6: if(world.weapons.defs[5].unlocked) world.weapons.selectedIdx=5; break;
    case K::Num7: if(world.weapons.defs[6].unlocked) world.weapons.selectedIdx=6; break;
    case K::Num8: if(world.weapons.defs[7].unlocked) world.weapons.selectedIdx=7; break;
    case K::Num9: if(world.weapons.defs[8].unlocked) world.weapons.selectedIdx=8; break;
    default: break;
    }
}

void Game::onMouseMoved(int dx, int dy) {
    camera.rotate(dx * 0.002f, dy * 0.002f);
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
            v->update(dt,
                sf::Keyboard::isKeyPressed(K::W),
                sf::Keyboard::isKeyPressed(K::S),
                sf::Keyboard::isKeyPressed(K::A) ? -1.f :
                sf::Keyboard::isKeyPressed(K::D) ?  1.f : 0.f);
            p.position = v->position; p.body.position = v->position;
            if (std::abs(v->speed) > v->maxSpeed * 0.95f)
                world.achievements.check(AchievementID::SPEED_DEMON);
            break;
        }
    }
}

void Game::update(float dt) {
    world.update(dt);
    Vec3 target = world.player.position;
    for (auto& v : world.vehicles) if (v->occupied) { target = v->position; break; }
    Vec3 shakeOff = world.anim.shake.getOffset();
    camera.followTarget(target);
    camera.position += shakeOff;
}

void Game::render() {
    impl->window.setActive(true);
    impl->renderer.render(world, camera, impl->window);
    impl->ui.draw(impl->window, world, paused);
    impl->window.display();
}
