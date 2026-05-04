#include "UI.h"

struct UI::Impl {
    sf::Font font;
    bool fontLoaded = false;
};
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

UI::UI() : impl(std::make_unique<Impl>()) {}
UI::~UI() = default;

void UI::init(const std::string& fontPath) {
    if (impl->font.loadFromFile(fontPath)) { impl->fontLoaded = true; return; }
    // Fallbacks
    for (auto* p : {"C:/Windows/Fonts/arial.ttf",
                    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"}) {
        if (impl->font.loadFromFile(p)) { impl->fontLoaded = true; return; }
    }
}

sf::Text UI::makeText(const std::string& s, unsigned sz, sf::Color col) {
    sf::Text t;
    t.setFont(impl->font);
    t.setString(s);
    t.setCharacterSize(sz);
    t.setFillColor(col);
    return t;
}

void UI::drawRoundedRect(sf::RenderWindow& w, float x, float y,
                          float width, float height, sf::Color fill,
                          sf::Color outline, float thick) {
    sf::RectangleShape r({width, height});
    r.setPosition({x, y});
    r.setFillColor(fill);
    if (thick > 0.f) { r.setOutlineThickness(thick); r.setOutlineColor(outline); }
    w.draw(r);
}

void UI::draw(sf::RenderWindow& window, const World& world, bool paused) {
    window.pushGLStates();
    if (!paused) {
        drawCrosshair(window);
        drawWeaponBar(window, world);
        drawDestructionMeter(window, world.totalDestructionPct);
        drawVehicleHUD(window, world);
        drawStats(window, world);
        drawControls(window, world);
        drawAchievementPopups(window, world);
    } else {
        drawPauseMenu(window, world);
    }
    window.popGLStates();
}

void UI::drawCrosshair(sf::RenderWindow& w) {
    float cx = w.getSize().x/2.f, cy = w.getSize().y/2.f;
    sf::Color c(255,255,255,200);
    float len = 12.f, gap = 5.f;
    sf::RectangleShape h({len,2.f}); h.setFillColor(c);
    h.setPosition({cx+gap,cy-1.f}); w.draw(h);
    h.setPosition({cx-gap-len,cy-1.f}); w.draw(h);
    sf::RectangleShape v({2.f,len}); v.setFillColor(c);
    v.setPosition({cx-1.f,cy+gap}); w.draw(v);
    v.setPosition({cx-1.f,cy-gap-len}); w.draw(v);
    // Center dot
    sf::CircleShape dot(2.f); dot.setFillColor(c);
    dot.setPosition({cx-2.f,cy-2.f}); w.draw(dot);
}

void UI::drawWeaponBar(sf::RenderWindow& w, const World& world) {
    if (!impl->fontLoaded) return;
    auto& weps = world.weapons;
    float barW = 60.f, barH = 70.f, gap = 6.f;
    int count = (int)weps.defs.size();
    float totalW = count * (barW + gap) - gap;
    float startX = w.getSize().x/2.f - totalW/2.f;
    float y = w.getSize().y - barH - 15.f;

    for (int i = 0; i < count; i++) {
        auto& def = weps.defs[i];
        float x = startX + i * (barW + gap);
        bool  sel = (i == weps.selectedIdx);
        bool  locked = !def.unlocked;

        // Background
        sf::Color bg = locked ? sf::Color(30,30,30,160)
                    : sel     ? sf::Color(220,80,30,230)
                              : sf::Color(20,20,40,190);
        sf::Color border = sel ? sf::Color(255,120,40) : sf::Color(80,80,100);
        drawRoundedRect(w, x, y, barW, barH, bg, border, sel ? 2.f : 1.f);

        if (locked) {
            // Lock icon text
            auto lt = makeText("LOCK", 11, sf::Color(120,120,120));
            lt.setPosition({x + barW/2.f - lt.getLocalBounds().width/2.f, y + barH/2.f - 8.f});
            w.draw(lt);
        } else {
            // Weapon name (abbreviated)
            std::string nm = def.name.substr(0, std::min((int)def.name.size(), 7));
            auto nt = makeText(nm, 11, sel ? sf::Color::White : sf::Color(180,180,180));
            nt.setPosition({x + 3.f, y + 3.f});
            w.draw(nt);

            // Ammo count
            std::string ammoStr = std::to_string(world.weapons.ammo[i]);
            if (world.weapons.ammo[i] == 99) ammoStr = "∞";
            auto at = makeText(ammoStr, 16, sel ? sf::Color(255,220,100) : sf::Color(150,150,200));
            at.setPosition({x + barW/2.f - at.getLocalBounds().width/2.f, y + barH - 22.f});
            w.draw(at);

            // Key number
            auto kt = makeText(std::to_string(i+1), 10, sf::Color(100,100,130));
            kt.setPosition({x + barW - 13.f, y + 3.f});
            w.draw(kt);
        }
    }

    // Selected weapon name above bar
    auto& sel = weps.selected();
    auto nt = makeText(sel.name, 16, sf::Color(255,200,100));
    nt.setPosition({w.getSize().x/2.f - nt.getLocalBounds().width/2.f, y - 22.f});
    w.draw(nt);
}

void UI::drawDestructionMeter(sf::RenderWindow& w, float pct) {
    float barW = 260.f, barH = 20.f;
    float x = w.getSize().x/2.f - barW/2.f, y = 14.f;

    drawRoundedRect(w, x-1, y-1, barW+2, barH+2, sf::Color(0,0,0,180));
    drawRoundedRect(w, x, y, barW, barH, sf::Color(15,15,20,200),
                    sf::Color(80,80,100), 1.f);

    float fw = barW * (pct / 100.f);
    sf::Uint8 r = (sf::Uint8)std::min(255.f, pct * 2.55f);
    sf::Uint8 g = (sf::Uint8)std::max(0.f, 255.f - pct * 2.55f);
    drawRoundedRect(w, x, y, fw, barH, sf::Color(r, g, 30, 220));

    if (!impl->fontLoaded) return;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << pct << "% DESTROYED";
    auto label = makeText(oss.str(), 13, sf::Color::White);
    label.setPosition({x + barW/2.f - label.getLocalBounds().width/2.f, y + 1.f});
    w.draw(label);
}

void UI::drawVehicleHUD(sf::RenderWindow& w, const World& world) {
    if (!impl->fontLoaded || world.player.mode != PlayerMode::IN_VEHICLE) return;
    for (auto& v : world.vehicles) {
        if (!v->occupied) continue;
        float spd = std::abs(v->speed) * 3.6f;
        std::ostringstream ss;
        ss << (int)spd << " km/h";
        auto t = makeText(ss.str(), 28, sf::Color(180,255,180));
        t.setPosition({20.f, 20.f});
        w.draw(t);
        std::string nm = v->type==VehicleType::TANK?"TANK":v->type==VehicleType::BULLDOZER?"BULLDOZER":"CAR";
        auto nt = makeText(nm, 18, sf::Color(120,220,120));
        nt.setPosition({20.f, 52.f});
        w.draw(nt);

        // Health bar
        float hpPct = v->health / 300.f;
        drawRoundedRect(w, 20.f, 76.f, 120.f, 10.f, sf::Color(40,10,10,200));
        drawRoundedRect(w, 20.f, 76.f, 120.f*hpPct, 10.f,
                        sf::Color(40+(int)(215*(1-hpPct)), (int)(200*hpPct), 20, 200));
        break;
    }
}

void UI::drawStats(sf::RenderWindow& w, const World& world) {
    if (!impl->fontLoaded) return;
    std::ostringstream ss;
    ss << "Explosions: " << world.explosionCount;
    auto t = makeText(ss.str(), 14, sf::Color(200,200,255,200));
    t.setPosition({14.f, w.getSize().y - 145.f});
    w.draw(t);

    ss.str("");
    ss << "Achievements: " << world.achievements.unlockedCount()
       << "/" << world.achievements.list.size();
    auto t2 = makeText(ss.str(), 14, sf::Color(255,220,100,200));
    t2.setPosition({14.f, w.getSize().y - 127.f});
    w.draw(t2);
}

void UI::drawControls(sf::RenderWindow& w, const World& world) {
    if (!impl->fontLoaded) return;
    float x = w.getSize().x - 195.f, y = 14.f;
    sf::Color c(200,200,200,150);
    std::vector<std::string> lines =
        world.player.mode == PlayerMode::ON_FOOT ?
        std::vector<std::string>{"WASD - Move","Mouse - Look","Space - Jump",
                                 "E - Fire Weapon","Q - Detonate",
                                 "Tab/Scroll - Switch Weapon",
                                 "1-9 - Select Weapon",
                                 "F - Enter Vehicle","R - Rebuild"}
      : std::vector<std::string>{"WASD - Drive","Mouse - Camera",
                                 "E - Fire Weapon","Q - Detonate",
                                 "Tab - Switch Weapon","F - Exit Vehicle"};
    for (int i = 0; i < (int)lines.size(); i++) {
        auto t = makeText(lines[i], 12, c);
        t.setPosition({x, y + i*16.f});
        w.draw(t);
    }
}

void UI::drawAchievementPopups(sf::RenderWindow& w, const World& world) {
    if (!impl->fontLoaded) return;
    float y = 55.f;
    for (auto& a : world.achievements.list) {
        if (a.showTimer <= 0.f) continue;
        float alpha = std::min(1.f, a.showTimer) * 255.f;
        float panelW = 300.f, panelH = 52.f;
        float x = w.getSize().x - panelW - 14.f;

        drawRoundedRect(w, x, y, panelW, panelH,
                        sf::Color(10,10,20,(sf::Uint8)(alpha*0.88f)),
                        sf::Color(255,180,30,(sf::Uint8)alpha), 2.f);

        auto title = makeText("🏆 " + a.title, 15, sf::Color(255,210,60,(sf::Uint8)alpha));
        title.setPosition({x+10.f, y+5.f});
        w.draw(title);

        auto desc = makeText(a.description, 12, sf::Color(200,200,200,(sf::Uint8)alpha));
        desc.setPosition({x+10.f, y+24.f});
        w.draw(desc);

        y += panelH + 6.f;
    }
}

void UI::drawPauseMenu(sf::RenderWindow& w, const World& world) {
    // Dim overlay
    sf::RectangleShape ov({(float)w.getSize().x, (float)w.getSize().y});
    ov.setFillColor(sf::Color(0,0,0,160)); w.draw(ov);
    if (!impl->fontLoaded) return;

    float cx = w.getSize().x/2.f;

    // Title
    auto title = makeText("METRO SMASH", 54, sf::Color(255,70,40));
    title.setStyle(sf::Text::Bold);
    title.setPosition({cx - title.getLocalBounds().width/2.f, 80.f});
    w.draw(title);

    auto sub = makeText("ULTIMATE DESTRUCTION SANDBOX", 18, sf::Color(200,150,100));
    sub.setPosition({cx - sub.getLocalBounds().width/2.f, 148.f});
    w.draw(sub);

    // Stats box
    float bx = cx - 200.f, by = 200.f;
    drawRoundedRect(w, bx, by, 400.f, 180.f, sf::Color(10,10,20,200),
                    sf::Color(80,60,40), 1.f);
    std::vector<std::string> stats = {
        "Destruction: " + [&]{ std::ostringstream s; s<<std::fixed<<std::setprecision(1)<<world.totalDestructionPct<<"%"; return s.str(); }(),
        "Explosions:  " + std::to_string(world.explosionCount),
        "Achievements: " + std::to_string(world.achievements.unlockedCount()) + " / " + std::to_string(world.achievements.list.size()),
        "Weapons Unlocked: " + [&]{ int c=0; for(auto& d:world.weapons.defs) if(d.unlocked) c++; return std::to_string(c); }() + " / " + std::to_string(world.weapons.defs.size()),
    };
    for (int i=0;i<(int)stats.size();i++) {
        auto t = makeText(stats[i], 16, sf::Color(220,200,170));
        t.setPosition({bx+20.f, by+18.f + i*36.f});
        w.draw(t);
    }

    // Achievement list
    float ay = 400.f;
    auto ah = makeText("ACHIEVEMENTS", 18, sf::Color(255,210,60));
    ah.setPosition({cx - ah.getLocalBounds().width/2.f, ay});
    w.draw(ah);
    ay += 28.f;
    int cols = 2; float colW = 340.f;
    for (int i=0;i<(int)world.achievements.list.size();i++) {
        auto& a = world.achievements.list[i];
        int col = i%cols; int row = i/cols;
        float ax = cx - colW + col * colW;
        float arY = ay + row * 22.f;
        sf::Color ac = a.unlocked ? sf::Color(100,255,100) : sf::Color(100,100,100);
        std::string marker = a.unlocked ? "[X] " : "[ ] ";
        auto at = makeText(marker + a.title, 13, ac);
        at.setPosition({ax, arY});
        w.draw(at);
    }

    // Resume hint
    auto resume = makeText("Press ESC or ENTER to resume   |   R to Rebuild", 16, sf::Color(180,180,180));
    resume.setPosition({cx - resume.getLocalBounds().width/2.f, w.getSize().y - 50.f});
    w.draw(resume);
}
