#include "Weapons.h"
#include "Log.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

static float rnd(float lo, float hi){ return lo + (float)rand()/RAND_MAX*(hi-lo); }

// ─────────────────────────────────────────────────────────────────────────────
//  Init — all weapons unlocked, unlimited ammo
// ─────────────────────────────────────────────────────────────────────────────
void WeaponSystem::init() {
    defs = {
        // ── Original 9 ─────────────────────────────────────────────────────
        {WeaponType::C4,           "C4 Charge",      "c4",   8.f,  600.f, 800.f,  999, 0.5f, "c4_place",    "c4_detonate",     true},
        {WeaponType::MISSILE,      "Missile",        "msil", 12.f, 900.f, 1200.f, 999, 1.5f, "missile_fire","explosion_large",  true},
        {WeaponType::RAILGUN,      "Railgun",        "rail", 6.f,  1400.f,2000.f, 999, 2.0f, "railgun",     "explosion_medium", true},
        {WeaponType::NUKE,         "Mini Nuke",      "nuke", 30.f, 3000.f,5000.f, 999, 8.0f, "nuke_fall",   "explosion_nuke",   true},
        {WeaponType::AIRSTRIKE,    "Airstrike",      "air",  15.f, 1200.f,1500.f, 999, 5.0f, "bomb_drop",   "explosion_large",  true},
        {WeaponType::CLUSTER_BOMB, "Cluster Bomb",   "clus", 5.f,  400.f, 600.f,  999, 3.0f, "bomb_drop",   "explosion_small",  true},
        {WeaponType::EMP,          "EMP Blast",      "emp",  20.f, 200.f, 300.f,  999, 4.0f, "missile_fire","explosion_emp",     true},
        {WeaponType::GRAVITY_BOMB, "Gravity Bomb",   "grav", 18.f, 2000.f,2500.f, 999, 6.0f, "bomb_drop",   "explosion_gravity",true},
        {WeaponType::WRECKING_BALL,"Wrecking Ball",  "wrck", 4.f,  800.f, 1000.f, 999, 0.3f, "metal_hit",   "building_break",   true},
        // ── New exotic weapons ──────────────────────────────────────────────
        {WeaponType::METEOR_SHOWER,"Meteor Shower",  "metr", 10.f, 1500.f,1800.f, 999, 6.0f, "nuke_fall",   "explosion_large",  true},
        {WeaponType::BLACK_HOLE,   "Black Hole",     "bkhl", 22.f, 2500.f,3000.f, 999,10.0f, "emp_charge",  "explosion_nuke",   true},
        {WeaponType::WORMHOLE,     "Wormhole",       "wrml", 14.f, 1800.f,2200.f, 999, 8.0f, "missile_fire","explosion_gravity",true},
        {WeaponType::LIGHTNING_STORM,"Lightning",    "ltng", 8.f,  1000.f,1200.f, 999, 5.0f, "railgun",     "explosion_emp",    true},
        {WeaponType::VOLCANO,      "Volcano",        "volc", 18.f, 2000.f,2400.f, 999,12.0f, "nuke_fall",   "explosion_large",  true},
        {WeaponType::TSUNAMI,      "Tsunami",        "tsun", 20.f, 1600.f,2000.f, 999,10.0f, "bomb_drop",   "explosion_large",  true},
        {WeaponType::TORNADO,      "Tornado",        "trnd", 16.f, 1200.f,1500.f, 999, 9.0f, "missile_fire","explosion_medium", true},
        {WeaponType::ACID_RAIN,    "Acid Rain",      "acid", 5.f,  300.f, 400.f,  999, 7.0f, "c4_place",    "explosion_small",  true},
    };

    for (int i = 0; i < (int)WeaponType::COUNT; i++) {
        defs[i].unlocked = true;
        ammo[i] = 999;
    }
    Log::info("WeaponSystem: %d weapons unlocked, unlimited ammo", (int)WeaponType::COUNT);
}

void WeaponSystem::selectNext() {
    int start = selectedIdx;
    do { selectedIdx = (selectedIdx + 1) % (int)defs.size(); }
    while (!defs[selectedIdx].unlocked && selectedIdx != start);
}
void WeaponSystem::selectPrev() {
    int start = selectedIdx;
    do { selectedIdx = ((selectedIdx - 1) + (int)defs.size()) % (int)defs.size(); }
    while (!defs[selectedIdx].unlocked && selectedIdx != start);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Fire
// ─────────────────────────────────────────────────────────────────────────────
void WeaponSystem::fire(Vec3 pos, Vec3 dir) {
    WeaponDef& def = selected();
    if (!def.unlocked) return;
    if (cooldownTimer[selectedIdx] > 0) return;
    if (ammo[selectedIdx] < 5) ammo[selectedIdx] = 999; // refill before empty

    cooldownTimer[selectedIdx] = def.cooldown;
    ammo[selectedIdx]--;

    ActiveWeapon w;
    w.type     = def.type;
    w.position = pos;
    w.id       = nextId++;

    switch (def.type) {
    // ── Original weapons ─────────────────────────────────────────────────
    case WeaponType::C4:
        w.position  = pos + dir * 2.f; w.position.y = 0.3f;
        w.velocity  = {}; w.fuseTime = -1.f; w.armed = true; break;

    case WeaponType::MISSILE:
        w.velocity = dir * 30.f; w.fuseTime = 4.f; break;

    case WeaponType::RAILGUN:
        w.position = pos + dir * 50.f; w.fuseTime = 0.f; break;

    case WeaponType::NUKE:
        w.position.y = 80.f; w.velocity = {0,-20.f,0}; w.fuseTime = 4.f; break;

    case WeaponType::AIRSTRIKE:
        w.position = pos + dir*30.f; w.position.y = 60.f;
        w.velocity = {0,-25.f,0}; w.fuseTime = 3.f; break;

    case WeaponType::CLUSTER_BOMB:
        w.position.y = 40.f; w.velocity = dir*10.f + Vec3(0,-5.f,0);
        w.fuseTime = 2.5f; break;

    case WeaponType::EMP:
        w.position = pos + dir*20.f; w.fuseTime = 0.5f; break;

    case WeaponType::GRAVITY_BOMB:
        w.position = pos + dir*3.f; w.velocity = dir*5.f;
        w.fuseTime = -1.f; break;

    case WeaponType::WRECKING_BALL:
        w.position = pos + dir*5.f; w.fuseTime = 0.f; break;

    // ── New exotic weapons ───────────────────────────────────────────────
    case WeaponType::METEOR_SHOWER:
        // First meteor — rest spawned as sub-steps in triggerExplosion
        w.position = pos + dir*20.f;
        w.position.y = 120.f;
        w.velocity = {rnd(-3.f,3.f), -35.f, rnd(-3.f,3.f)};
        w.fuseTime = 4.f;
        w.duration = 3.f;   // 8 meteors over 3 s
        w.subStep  = 8;     // meteors remaining
        break;

    case WeaponType::BLACK_HOLE:
        // Appears at aim point, sucks everything for 4 s then implodes
        w.position = pos + dir * 25.f;
        w.position.y = 4.f;
        w.fuseTime = 4.f;
        w.duration = 4.f;
        break;

    case WeaponType::WORMHOLE:
        // Opens at aim, teleports debris to exit 40 m away, exit explosion
        w.position = pos + dir * 20.f;
        w.position.y = 1.f;
        w.fuseTime = 1.5f; // entrance flash, then exit boom
        break;

    case WeaponType::LIGHTNING_STORM:
        // Instantaneous — 12 bolts fired over 3 s; handled via subStep ticks
        w.position = pos + dir * 15.f;
        w.position.y = 60.f;
        w.fuseTime = 0.25f; // first bolt
        w.duration = 3.f;
        w.subStep  = 12;
        break;

    case WeaponType::VOLCANO:
        // Eruption centre under target; lava blobs arc for 5 s
        w.position = pos + dir * 20.f;
        w.position.y = 0.f;
        w.fuseTime = 0.5f; // initial ground crack
        w.duration = 5.f;
        w.subStep  = 20; // lava blobs remaining
        break;

    case WeaponType::TSUNAMI:
        // Rolling wave from one side of the city
        w.position = {pos.x + dir.x * 60.f, 0.f, pos.z + dir.z * 60.f};
        w.velocity = dir * (-20.f); // rolls toward player
        w.fuseTime = 0.1f;
        w.duration = 4.f;
        break;

    case WeaponType::TORNADO:
        // Spawns at aim, wanders for 6 s
        w.position = pos + dir * 18.f;
        w.position.y = 0.f;
        w.velocity = {rnd(-2.f,2.f), 0, rnd(-2.f,2.f)};
        w.fuseTime = 6.f;
        w.duration = 6.f;
        break;

    case WeaponType::ACID_RAIN:
        // Area-denial cloud; rains acid drops for 8 s
        w.position = pos + dir * 15.f;
        w.position.y = 40.f;
        w.fuseTime = 8.f;
        w.duration = 8.f;
        w.subStep  = 80; // drops remaining
        break;

    default: break;
    }

    active.push_back(w);
    Log::info("Fired %s", def.name.c_str());
}

void WeaponSystem::detonateManual() {
    for (auto& w : active)
        if (w.armed && !w.detonated &&
            (w.type == WeaponType::C4 || w.type == WeaponType::GRAVITY_BOMB))
            w.fuseTime = 0.f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Update
// ─────────────────────────────────────────────────────────────────────────────
void WeaponSystem::update(float dt) {
    for (auto& t : cooldownTimer) if (t > 0) t -= dt;

    for (auto& w : active) {
        if (w.detonated) continue;
        w.timer += dt;

        // Physics movement
        if (w.velocity.lengthSq() > 0.01f) {
            // Tornado wanders, no gravity
            if (w.type != WeaponType::TORNADO)
                w.velocity.y -= 9.8f * dt;
            w.position += w.velocity * dt;
            if (w.position.y <= 0.f && w.fuseTime > 0.f &&
                w.type != WeaponType::TORNADO) {
                w.position.y = 0.f;
                w.fuseTime = 0.f;
            }
        }

        // Sustained-weapon per-tick effects (before detonation)
        if (w.fuseTime >= 0.f) {
            w.fuseTime -= dt;

            // Meteor shower — spawn additional meteors on a timer
            if (w.type == WeaponType::METEOR_SHOWER && w.subStep > 0) {
                static float meteorTimer = 0.f;
                meteorTimer += dt;
                if (meteorTimer > w.duration / 8.f) {
                    meteorTimer = 0.f;
                    w.subStep--;
                    // Spawn sub-meteor near original target
                    ActiveWeapon sub;
                    sub.type     = WeaponType::METEOR_SHOWER;
                    sub.position = w.position + Vec3(rnd(-15.f,15.f), 0, rnd(-15.f,15.f));
                    sub.position.y = 120.f;
                    sub.velocity = {rnd(-4.f,4.f), -35.f, rnd(-4.f,4.f)};
                    sub.fuseTime = rnd(2.f, 4.f);
                    sub.duration = 0.f; sub.subStep = 0; // terminal meteor
                    sub.id = nextId++;
                    active.push_back(sub);
                }
            }

            // Tornado — continuous multi-pulse damage while alive
            if (w.type == WeaponType::TORNADO) {
                w.velocity.x += rnd(-1.f,1.f) * dt * 5.f;
                w.velocity.z += rnd(-1.f,1.f) * dt * 5.f;
                // Cap wander speed
                float spd = w.velocity.length();
                if (spd > 5.f) { w.velocity.x /= spd/5.f; w.velocity.z /= spd/5.f; }
                w.position += w.velocity * dt;
                // Pulse every 0.4 s
                static float tornadoTimer = 0.f;
                tornadoTimer += dt;
                if (tornadoTimer > 0.4f) {
                    tornadoTimer = 0.f;
                    if (onExplode)
                        onExplode(w.position, 12.f, 400.f, "", "explosion_medium");
                }
            }

            // Acid rain — drops every 0.1 s
            if (w.type == WeaponType::ACID_RAIN && w.subStep > 0) {
                static float acidTimer = 0.f;
                acidTimer += dt;
                if (acidTimer > 0.1f && w.subStep > 0) {
                    acidTimer = 0.f; w.subStep--;
                    Vec3 dropPos = w.position + Vec3(rnd(-12.f,12.f), 0, rnd(-12.f,12.f));
                    ActiveWeapon drop;
                    drop.type = WeaponType::ACID_RAIN;
                    drop.position = dropPos;
                    drop.velocity = {0,-15.f,0};
                    drop.fuseTime = rnd(1.f, 3.f);
                    drop.subStep  = 0; // terminal drop
                    drop.id = nextId++;
                    active.push_back(drop);
                }
            }

            // Volcano lava blobs
            if (w.type == WeaponType::VOLCANO && w.subStep > 0) {
                static float lavaTimer = 0.f;
                lavaTimer += dt;
                if (lavaTimer > w.duration / 20.f) {
                    lavaTimer = 0.f; w.subStep--;
                    float angle = rnd(0, 6.28f);
                    float speed = rnd(12.f, 20.f);
                    ActiveWeapon blob;
                    blob.type = WeaponType::VOLCANO;
                    blob.position = w.position + Vec3(0, 2.f, 0);
                    blob.velocity = {cosf(angle)*speed, rnd(15.f,25.f), sinf(angle)*speed};
                    blob.fuseTime = rnd(1.5f, 3.5f);
                    blob.subStep  = 0;
                    blob.id = nextId++;
                    active.push_back(blob);
                }
            }

            // Lightning storm — bolt every 0.25 s
            if (w.type == WeaponType::LIGHTNING_STORM && w.subStep > 0) {
                static float boltTimer = 0.f;
                boltTimer += dt;
                if (boltTimer > 0.25f) {
                    boltTimer = 0.f; w.subStep--;
                    Vec3 strike = w.position + Vec3(rnd(-20.f,20.f), -60.f, rnd(-20.f,20.f));
                    if (onExplode) onExplode(strike, 6.f, 1000.f, "railgun", "explosion_emp");
                }
            }

            // Black hole — sustained pull handled in World via onExplode pulse
            if (w.type == WeaponType::BLACK_HOLE) {
                static float bhTimer = 0.f;
                bhTimer += dt;
                if (bhTimer > 0.3f) {
                    bhTimer = 0.f;
                    if (onExplode) onExplode(w.position, 22.f, -800.f, "", ""); // negative = pull
                }
            }

            if (w.fuseTime <= 0.f) {
                triggerExplosion(w);
                w.detonated = true;
            }
        }
    }

    active.erase(std::remove_if(active.begin(), active.end(),
        [](const ActiveWeapon& w){ return w.detonated; }), active.end());
}

// ─────────────────────────────────────────────────────────────────────────────
//  triggerExplosion
// ─────────────────────────────────────────────────────────────────────────────
void WeaponSystem::triggerExplosion(const ActiveWeapon& w) {
    const WeaponDef& def = defs[(int)w.type];
    if (!onExplode) return;

    switch (w.type) {
    case WeaponType::CLUSTER_BOMB:
        for (int i = 0; i < 6; i++) {
            float a = i * 3.14159f * 2.f / 6;
            Vec3 off(cosf(a)*6.f, 0, sinf(a)*6.f);
            onExplode(w.position+off, def.blastRadius*0.5f,
                      def.blastStrength*0.5f, def.fireSound, def.explodeSound);
        }
        break;

    case WeaponType::METEOR_SHOWER:
        // Each meteor hits individually
        onExplode(w.position, def.blastRadius, def.blastStrength,
                  def.fireSound, def.explodeSound);
        break;

    case WeaponType::BLACK_HOLE:
        // Final implosion — massive explosion at centre
        onExplode(w.position, 28.f, 5000.f, "nuke_fall", "explosion_nuke");
        break;

    case WeaponType::WORMHOLE:
        // Entrance vortex + displaced exit explosion
        onExplode(w.position, 10.f, 1200.f, def.fireSound, def.explodeSound);
        {
            Vec3 exit = w.position + Vec3(rnd(-30.f,30.f), 0, rnd(-30.f,30.f));
            onExplode(exit, 14.f, 1800.f, def.fireSound, "explosion_nuke");
        }
        break;

    case WeaponType::TSUNAMI:
        // 5 sequential wave pulses in a line
        for (int i = 0; i < 5; i++) {
            Vec3 wavePos = w.position + Vec3(w.velocity.x, 0, w.velocity.z) * (i * -5.f);
            onExplode(wavePos, 20.f, 1600.f/(i+1), def.fireSound, def.explodeSound);
        }
        break;

    case WeaponType::TORNADO:
        // Final dissipation blast
        onExplode(w.position, 16.f, 1200.f, def.fireSound, def.explodeSound);
        break;

    case WeaponType::VOLCANO:
        if (w.subStep == 0)
            // Individual lava blob impact
            onExplode(w.position, 7.f, 900.f, "explosion_small", "explosion_large");
        else
            // Initial ground eruption
            onExplode(w.position, def.blastRadius, def.blastStrength,
                      def.fireSound, def.explodeSound);
        break;

    case WeaponType::LIGHTNING_STORM:
        // Terminal (no remaining bolts)
        break;

    case WeaponType::ACID_RAIN:
        // Terminal acid drop — small but corrosive
        onExplode(w.position, 4.f, 250.f, "", "explosion_small");
        break;

    default:
        onExplode(w.position, def.blastRadius, def.blastStrength,
                  def.fireSound, def.explodeSound);
        break;
    }
}

void WeaponSystem::unlockWeapon(WeaponType t) {
    defs[(int)t].unlocked = true;
    ammo[(int)t] = 999;
}
