#include "UI.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdint>

void UI::init() {
    const char* fallbacks[] = {
        "assets/fonts/font.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    };
    for (auto* path : fallbacks) {
        if (font.openFromFile(path)) { fontLoaded = true; break; }
    }
}

// SFML 3: sf::Text requires font in constructor
sf::Text UI::makeText(const std::string& s, unsigned size, sf::Color color) {
    if (fontLoaded) {
        sf::Text t(font, s, size);
        t.setFillColor(color);
        return t;
    }
    // Fallback: default font not possible in SFML3 without a font object
    // Create a dummy text — will show nothing but won't crash
    sf::Text t(font, s, size);
    t.setFillColor(color);
    return t;
}

void UI::draw(sf::RenderWindow& window, const World& world, bool paused) {
    window.pushGLStates();
    drawHUD(window, world);
    if (paused) drawPauseMenu(window);
    window.popGLStates();
}

void UI::drawHUD(sf::RenderWindow& w, const World& world) {
    drawCrosshair(w);
    drawExplosiveBar(w, world.player.explosiveCount);
    drawDestructionMeter(w, world.totalDestructionPct);
    drawVehicleInfo(w, world);
    drawControls(w, world);
}

void UI::drawCrosshair(sf::RenderWindow& w) {
    float cx = w.getSize().x / 2.f;
    float cy = w.getSize().y / 2.f;
    sf::Color c(255, 255, 255, 200);
    float len = 10.f, gap = 4.f;

    sf::RectangleShape h({len, 2.f});
    h.setFillColor(c);
    h.setPosition({cx + gap, cy - 1.f});
    w.draw(h);
    h.setPosition({cx - gap - len, cy - 1.f});
    w.draw(h);

    sf::RectangleShape v({2.f, len});
    v.setFillColor(c);
    v.setPosition({cx - 1.f, cy + gap});
    w.draw(v);
    v.setPosition({cx - 1.f, cy - gap - len});
    w.draw(v);
}

void UI::drawExplosiveBar(sf::RenderWindow& w, int count) {
    if (!fontLoaded) return;
    float x = 20.f, y = w.getSize().y - 60.f;

    auto label = makeText("EXPLOSIVES", 14, sf::Color(255, 80, 80));
    label.setPosition({x, y - 20.f});
    w.draw(label);

    for (int i = 0; i < 10; i++) {
        sf::RectangleShape box({18.f, 24.f});
        box.setPosition({x + i * 22.f, y});
        box.setOutlineThickness(1.f);
        box.setOutlineColor(sf::Color(200, 60, 60));
        box.setFillColor(i < count ? sf::Color(220, 50, 50, 220)
                                   : sf::Color(50, 20, 20, 150));
        w.draw(box);
    }
}

void UI::drawDestructionMeter(sf::RenderWindow& w, float pct) {
    float barW = 220.f, barH = 18.f;
    float x = w.getSize().x / 2.f - barW / 2.f;
    float y = 18.f;

    sf::RectangleShape bg({barW, barH});
    bg.setPosition({x, y});
    bg.setFillColor(sf::Color(20, 20, 20, 180));
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(150, 150, 150));
    w.draw(bg);

    float fillW = barW * (pct / 100.f);
    sf::RectangleShape fill({fillW, barH});
    fill.setPosition({x, y});
    fill.setFillColor(sf::Color(
        (uint8_t)(pct * 2.5f),
        (uint8_t)(255 - pct * 2.5f),
        0, 220));
    w.draw(fill);

    if (!fontLoaded) return;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << pct << "% DESTROYED";
    auto label = makeText(oss.str(), 13, sf::Color::White);
    label.setPosition({x + barW / 2.f - label.getLocalBounds().size.x / 2.f, y + 1.f});
    w.draw(label);
}

void UI::drawVehicleInfo(sf::RenderWindow& w, const World& world) {
    if (!fontLoaded) return;
    if (world.player.mode != PlayerMode::IN_VEHICLE) return;
    for (const auto& v : world.vehicles) {
        if (!v->occupied) continue;
        std::ostringstream oss;
        oss << "SPEED: " << (int)(std::abs(v->speed) * 3.6f) << " km/h";
        auto t = makeText(oss.str(), 16, sf::Color(200, 255, 200));
        t.setPosition({20.f, 20.f});
        w.draw(t);
        std::string vtype = (v->type == VehicleType::TANK) ? "TANK" :
                            (v->type == VehicleType::BULLDOZER) ? "BULLDOZER" : "CAR";
        auto t2 = makeText(vtype, 20, sf::Color(150, 255, 150));
        t2.setPosition({20.f, 40.f});
        w.draw(t2);
        break;
    }
}

void UI::drawControls(sf::RenderWindow& w, const World& world) {
    if (!fontLoaded) return;
    float x = w.getSize().x - 180.f, y = 20.f;
    sf::Color c(255, 255, 255, 170);
    std::vector<std::string> lines =
        (world.player.mode == PlayerMode::ON_FOOT)
        ? std::vector<std::string>{"WASD - Move","Mouse - Look","Space - Jump",
                                   "F - Enter Vehicle","E - Place Explosive",
                                   "Q - Detonate All","R - Rebuild City"}
        : std::vector<std::string>{"WASD - Drive","Mouse - Camera",
                                   "F - Exit Vehicle","E - Place Explosive",
                                   "Q - Detonate All"};
    for (int i = 0; i < (int)lines.size(); i++) {
        auto t = makeText(lines[i], 13, c);
        t.setPosition({x, y + i * 18.f});
        w.draw(t);
    }
}

void UI::drawPauseMenu(sf::RenderWindow& w) {
    sf::RectangleShape overlay({(float)w.getSize().x, (float)w.getSize().y});
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    w.draw(overlay);
    if (!fontLoaded) return;
    auto title = makeText("METRO SMASH", 48, sf::Color(255, 80, 60));
    title.setPosition({w.getSize().x / 2.f - 140.f, w.getSize().y / 2.f - 80.f});
    w.draw(title);
    auto sub = makeText("Press ESC or ENTER to resume", 22, sf::Color(220, 220, 220));
    sub.setPosition({w.getSize().x / 2.f - 160.f, w.getSize().y / 2.f + 10.f});
    w.draw(sub);
}
