// ALL SFML includes go here first, before anything else in this TU
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/OpenGL.hpp>

#include "Game.h"
#include "Renderer.h"
#include "UI.h"
#include "GfxShaders.h"
#include "Log.h"
#include <cmath>
#include <functional>

// PIMPL struct owns SFML window, clock, renderer, UI
struct Game::Impl {
    sf::RenderWindow window;
    sf::Clock        clock;
    Renderer         renderer;
    UI               ui;
};

// Destructor (and constructor) defined here so Impl is complete when
// unique_ptr<Impl>'s deleter is instantiated — never in a TU that only
// sees the forward declaration in Game.h.
Game::Game()  = default;
Game::~Game() = default;

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
    Log::info("Game::init — creating window");
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

    Log::info("Window created — initialising renderer");
    impl->renderer.init(1280, 720);
    camera.aspect = 1280.f / 720.f;
    impl->ui.init("assets/fonts/GameFont.ttf");
    Log::info("Loading world assets");
    world.init("assets");
    Log::info("Game::init complete");
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
        else if (ev.type == sf::Event::MouseButtonPressed) {
            if (ev.mouseButton.button == sf::Mouse::Right) {
                // Right-click toggles mouse lock (lets you use UI/graphics panel)
                if (mouseLocked) unlockMouse();
                else if (!paused) lockMouse();
            } else if (ev.mouseButton.button == sf::Mouse::Left) {
                // Left-click re-locks if not paused
                if (!mouseLocked && !paused) lockMouse();
            }
        }
        else if (ev.type == sf::Event::MouseWheelScrolled) {
            if (ev.mouseWheelScroll.delta > 0) world.weapons.selectNext();
            else                               world.weapons.selectPrev();
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

    // ── Graphics panel (G) ────────────────────────────────────────────────
    case K::G:
        impl->renderer.gfxPanel.visible = !impl->renderer.gfxPanel.visible;
        break;

    // ── Camera mode cycle (C) ─────────────────────────────────────────────
    case K::C:
        camera.toggleMode();
        if (camera.mode == CameraMode::FREE) {
            unlockMouse();          // allow cursor for free-cam
            lockMouse();            // re-lock so delta still works
        }
        break;

    // ── Time of day (T cycles through: day/sunset/night) ──────────────────
    case K::T:
        {
            float& tod = impl->renderer.gfxPanel.timeOfDay;
            tod = (tod >= 0.8f) ? 0.0f : (tod >= 0.4f) ? 0.85f : 0.5f;
            gGfx.settings.timeOfDay = tod;
        }
        break;

    // ── Rain toggle (P) ───────────────────────────────────────────────────
    case K::P:
        impl->renderer.gfxPanel.rainEnabled = !impl->renderer.gfxPanel.rainEnabled;
        break;
    case K::Num1: world.weapons.selectedIdx=0; break;
    case K::Num2: if(world.weapons.defs[1].unlocked) world.weapons.selectedIdx=1; break;
    case K::Num3: if(world.weapons.defs[2].unlocked) world.weapons.selectedIdx=2; break;
    case K::Num4: if(world.weapons.defs[3].unlocked) world.weapons.selectedIdx=3; break;
    case K::Num5: if(world.weapons.defs[4].unlocked) world.weapons.selectedIdx=4; break;
    case K::Num6: if(world.weapons.defs[5].unlocked) world.weapons.selectedIdx=5; break;
    case K::Num7: if(world.weapons.defs[6].unlocked) world.weapons.selectedIdx=6; break;
    case K::Num8: if(world.weapons.defs[7].unlocked) world.weapons.selectedIdx=7; break;
    case K::Num9: if(world.weapons.defs[8].unlocked) world.weapons.selectedIdx=8; break;
    case K::F1:
        impl->renderer.gfxPanel.shadowsEnabled = !impl->renderer.gfxPanel.shadowsEnabled;
        break;
    case K::F2:
        impl->renderer.gfxPanel.ssaoEnabled = !impl->renderer.gfxPanel.ssaoEnabled;
        break;
    case K::F3:
        impl->renderer.gfxPanel.bloomEnabled = !impl->renderer.gfxPanel.bloomEnabled;
        break;
    case K::F4:
        impl->renderer.gfxPanel.vignetteOn = !impl->renderer.gfxPanel.vignetteOn;
        break;
    case K::F5:
        impl->renderer.gfxPanel.grainOn = !impl->renderer.gfxPanel.grainOn;
        break;
    case K::F6:
        impl->renderer.gfxPanel.chromaticOn = !impl->renderer.gfxPanel.chromaticOn;
        break;
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
    // ── Free camera movement ─────────────────────────────────────────────
    if (camera.mode == CameraMode::FREE) {
        camera.freeCamFast = sf::Keyboard::isKeyPressed(K::LShift);
        Vec3 delta(0,0,0);
        if (sf::Keyboard::isKeyPressed(K::W)) delta.z += dt;
        if (sf::Keyboard::isKeyPressed(K::S)) delta.z -= dt;
        if (sf::Keyboard::isKeyPressed(K::A)) delta.x -= dt;
        if (sf::Keyboard::isKeyPressed(K::D)) delta.x += dt;
        if (sf::Keyboard::isKeyPressed(K::Q)) delta.y -= dt;
        if (sf::Keyboard::isKeyPressed(K::E)) delta.y += dt;
        camera.moveFree(delta);
        return;  // skip player/vehicle input in free-cam
    }

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
    camera.update(dt);
    Vec3 shakeOff = world.anim.shake.getOffset();
    camera.followTarget(target);
    camera.position += shakeOff;
}

void Game::render() {
    impl->window.setActive(true);
    impl->renderer.dt = impl->clock.getElapsedTime().asSeconds();
    impl->renderer.render(world, camera, impl->window);
    impl->ui.draw(impl->window, world, paused);

    // ── Graphics settings overlay panel ───────────────────────────────────
    if (impl->renderer.gfxPanel.visible) {
        impl->window.pushGLStates();
        float W = (float)impl->window.getSize().x;
        float H = (float)impl->window.getSize().y;

        // Load font once
        static sf::Font panelFont;
        static bool fontLoaded = false;
        if (!fontLoaded) {
            fontLoaded = panelFont.loadFromFile("assets/fonts/GameFont.ttf");
        }

        // Background panel
        sf::RectangleShape bg(sf::Vector2f(300.f, 370.f));
        bg.setFillColor(sf::Color(0,0,0,180));
        bg.setOutlineColor(sf::Color(80,80,80,220));
        bg.setOutlineThickness(1.f);
        bg.setPosition(W - 315.f, 80.f);
        impl->window.draw(bg);

        auto& gp = impl->renderer.gfxPanel;
        auto& gs = gGfx.settings;
        struct Row { const char* label; bool* toggle; };
        Row rows[] = {
            {"[G] GRAPHICS PANEL",    nullptr},
            {"Shaders  (Tab)",        &gp.shadersEnabled},
            {"Shadows  (F1)",         &gp.shadowsEnabled},
            {"SSAO     (F2)",         &gp.ssaoEnabled},
            {"Bloom    (F3)",         &gp.bloomEnabled},
            {"Vignette (F4)",         &gp.vignetteOn},
            {"Film Grain (F5)",       &gp.grainOn},
            {"Chromatic Ab. (F6)",    &gp.chromaticOn},
            {"Rain     [P]",          &gp.rainEnabled},
            {nullptr, nullptr},
            {"Time of Day [T]:",      nullptr},
            {"Fog density:",          nullptr},
            {nullptr, nullptr},
            {"[C] Cycle Camera Mode", nullptr},
        };

        float x = W - 308.f, y = 90.f;
        for (auto& row : rows) {
            if (!row.label) { y += 8.f; continue; }
            sf::Text txt;
            txt.setFont(panelFont);
            txt.setCharacterSize(13);
            txt.setPosition(x, y);
            if (row.toggle == nullptr) {
                txt.setFillColor(sf::Color(200,200,60,255));
                txt.setString(row.label);
            } else {
                bool on = *row.toggle;
                txt.setFillColor(on ? sf::Color(80,255,80,255) : sf::Color(200,80,80,255));
                txt.setString(std::string(row.label) + (on ? "  [ON]" : " [OFF]"));
            }
            impl->window.draw(txt);
            y += 22.f;
        }

        // Time of day slider
        {
            sf::Text lbl; lbl.setFont(panelFont); lbl.setCharacterSize(12);
            lbl.setFillColor(sf::Color(180,180,180,255));
            const char* todNames[] = {"NIGHT","SUNRISE","DAY","SUNSET"};
            int ti = (gp.timeOfDay < 0.25f) ? 0 :
                     (gp.timeOfDay < 0.5f)  ? 1 :
                     (gp.timeOfDay < 0.75f) ? 2 : 3;
            lbl.setString(std::string("  -> ") + todNames[ti]);
            lbl.setPosition(x, y); impl->window.draw(lbl); y += 18.f;
        }
        // Camera mode label
        {
            sf::Text lbl; lbl.setFont(panelFont); lbl.setCharacterSize(12);
            lbl.setFillColor(sf::Color(180,180,180,255));
            const char* modeNames[] = {"THIRD PERSON","FREE FLY","FIRST PERSON","CINEMATIC"};
            int mi = (int)camera.mode;
            lbl.setString(std::string("  -> ") + modeNames[mi < 4 ? mi : 0]);
            lbl.setPosition(x, y); impl->window.draw(lbl);
        }

        impl->window.popGLStates();
    }
    impl->window.display();
}
