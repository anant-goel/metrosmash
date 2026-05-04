#include "Weapons.h"
#include <algorithm>
#include <cmath>

void WeaponSystem::init() {
    defs = {
        {WeaponType::C4,           "C4 Charge",      "c4",     8.f,  600.f, 800.f,  10,  0.5f, "c4_place",    "c4_detonate",  true},
        {WeaponType::MISSILE,      "Missile",        "msil",   12.f, 900.f, 1200.f, 5,   1.5f, "missile_fire","explosion_large", true},
        {WeaponType::RAILGUN,      "Railgun",        "rail",   6.f,  1400.f,2000.f, 8,   2.0f, "railgun",     "explosion_medium", false},
        {WeaponType::NUKE,         "Mini Nuke",      "nuke",   30.f, 3000.f,5000.f, 2,   8.0f, "nuke_fall",   "explosion_nuke", false},
        {WeaponType::AIRSTRIKE,    "Airstrike",      "air",    15.f, 1200.f,1500.f, 3,   5.0f, "bomb_drop",   "explosion_large", false},
        {WeaponType::CLUSTER_BOMB, "Cluster Bomb",   "clus",   5.f,  400.f, 600.f,  6,   3.0f, "bomb_drop",   "explosion_small", false},
        {WeaponType::EMP,          "EMP Blast",      "emp",    20.f, 200.f, 300.f,  4,   4.0f, "missile_fire","explosion_emp", false},
        {WeaponType::GRAVITY_BOMB, "Gravity Bomb",   "grav",   18.f, 2000.f,2500.f, 3,   6.0f, "bomb_drop",   "explosion_gravity", false},
        {WeaponType::WRECKING_BALL,"Wrecking Ball",  "wrck",   4.f,  800.f, 1000.f, 99,  0.3f, "metal_hit",   "building_break", false},
    };

    for (int i = 0; i < (int)WeaponType::COUNT; i++) ammo[i] = defs[i].maxAmmo;
}

void WeaponSystem::selectNext() {
    // Only cycle through unlocked weapons
    int start = selectedIdx;
    do {
        selectedIdx = (selectedIdx + 1) % defs.size();
    } while (!defs[selectedIdx].unlocked && selectedIdx != start);
}

void WeaponSystem::selectPrev() {
    int start = selectedIdx;
    do {
        selectedIdx = (selectedIdx - 1 + defs.size()) % defs.size();
    } while (!defs[selectedIdx].unlocked && selectedIdx != start);
}

void WeaponSystem::fire(Vec3 pos, Vec3 dir) {
    WeaponDef& def = selected();
    if (!def.unlocked) return;
    if (ammo[selectedIdx] <= 0) return;
    if (cooldownTimer[selectedIdx] > 0) return;

    cooldownTimer[selectedIdx] = def.cooldown;
    ammo[selectedIdx]--;

    ActiveWeapon w;
    w.type     = def.type;
    w.position = pos;
    w.id       = nextId++;

    switch (def.type) {
    case WeaponType::C4:
        // Placed at feet, manual detonation
        w.position  = pos + dir * 2.f;
        w.position.y = 0.3f;
        w.velocity  = {0,0,0};
        w.fuseTime  = -1.f; // manual
        w.armed     = true;
        break;

    case WeaponType::MISSILE:
        w.velocity = dir * 30.f;
        w.fuseTime = 4.f;
        break;

    case WeaponType::RAILGUN:
        // Instant hit along ray — explode immediately at range
        w.position = pos + dir * 50.f;
        w.fuseTime = 0.f;
        break;

    case WeaponType::NUKE:
        w.position.y = 80.f; // falls from sky
        w.velocity   = {0, -20.f, 0};
        w.fuseTime   = 4.f;
        break;

    case WeaponType::AIRSTRIKE:
        w.position   = pos + dir * 30.f;
        w.position.y = 60.f;
        w.velocity   = {0, -25.f, 0};
        w.fuseTime   = 3.f;
        break;

    case WeaponType::CLUSTER_BOMB:
        w.position.y = 40.f;
        w.velocity   = dir * 10.f + Vec3(0, -5.f, 0);
        w.fuseTime   = 2.5f;
        break;

    case WeaponType::EMP:
        w.position = pos + dir * 20.f;
        w.fuseTime = 0.5f;
        break;

    case WeaponType::GRAVITY_BOMB:
        w.position = pos + dir * 3.f;
        w.velocity = dir * 5.f;
        w.fuseTime = -1.f; // manual
        break;

    case WeaponType::WRECKING_BALL:
        w.position = pos + dir * 5.f;
        w.fuseTime = 0.f;
        break;

    default: break;
    }

    active.push_back(w);
}

void WeaponSystem::detonateManual() {
    for (auto& w : active) {
        if (!w.armed || w.detonated) continue;
        if (w.type == WeaponType::C4 || w.type == WeaponType::GRAVITY_BOMB) {
            w.fuseTime = 0.f;
        }
    }
}

void WeaponSystem::update(float dt) {
    // Update cooldowns
    for (auto& t : cooldownTimer) if (t > 0) t -= dt;

    // Update active weapons
    for (auto& w : active) {
        if (w.detonated) continue;

        w.timer += dt;

        // Physics movement for projectiles
        if (w.velocity.lengthSq() > 0.01f) {
            w.velocity.y -= 9.8f * dt; // gravity on projectiles
            w.position += w.velocity * dt;
            // Ground hit
            if (w.position.y <= 0.f && w.fuseTime > 0.f) {
                w.position.y = 0.f;
                w.fuseTime = 0.f;
            }
        }

        // Fuse countdown
        if (w.fuseTime >= 0.f) {
            w.fuseTime -= dt;
            if (w.fuseTime <= 0.f) {
                triggerExplosion(w);
                w.detonated = true;
            }
        }
    }

    // Clean up detonated
    active.erase(
        std::remove_if(active.begin(), active.end(),
            [](const ActiveWeapon& w){ return w.detonated; }),
        active.end());
}

void WeaponSystem::triggerExplosion(const ActiveWeapon& w) {
    const WeaponDef& def = defs[(int)w.type];

    if (onExplode) {
        if (w.type == WeaponType::CLUSTER_BOMB) {
            // Multiple sub-explosions
            for (int i = 0; i < 6; i++) {
                float angle = i * 3.14159f * 2.f / 6;
                Vec3 off(cosf(angle)*6.f, 0, sinf(angle)*6.f);
                onExplode(w.position + off, def.blastRadius*0.5f,
                          def.blastStrength*0.5f, def.fireSound, def.explodeSound);
            }
        } else {
            onExplode(w.position, def.blastRadius, def.blastStrength,
                      def.fireSound, def.explodeSound);
        }
    }
}

void WeaponSystem::unlockWeapon(WeaponType t) {
    defs[(int)t].unlocked = true;
    ammo[(int)t] = defs[(int)t].maxAmmo;
}
