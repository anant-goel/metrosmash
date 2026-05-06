#include "World.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

static float rnd(float lo, float hi) { return lo + (float)rand()/RAND_MAX*(hi-lo); }

void World::init(const std::string& assetDir) {
    srand(42);

    audio.init(assetDir + "/audio");

    player.init();
    player.position = {0, 1, 15};
    player.body.position = player.position;

    weapons.init();
    weapons.onExplode = [&](Vec3 pos, float radius, float strength,
                             const std::string& sndFire, const std::string& sndExplode) {
        explodeAt(pos, radius, strength, sndFire, sndExplode);
    };

    achievements.init();
    achievements.onUnlock = [&](const Achievement& a) {
        audio.play("ui_click", 80.f);
        // Unlock weapons as rewards
        if (a.id == AchievementID::DEMOLISHER)         weapons.unlockWeapon(WeaponType::RAILGUN);
        if (a.id == AchievementID::HALF_GONE)          weapons.unlockWeapon(WeaponType::AIRSTRIKE);
        if (a.id == AchievementID::CHAIN_REACTION)     weapons.unlockWeapon(WeaponType::CLUSTER_BOMB);
        if (a.id == AchievementID::TANK_COMMANDER)     weapons.unlockWeapon(WeaponType::EMP);
        if (a.id == AchievementID::UNSTOPPABLE)        weapons.unlockWeapon(WeaponType::GRAVITY_BOMB);
        if (a.id == AchievementID::TOTAL_ANNIHILATION) weapons.unlockWeapon(WeaponType::NUKE);
    };

    buildCity();
    spawnVehicles();
}

void World::buildCity() {
    buildings.clear();
    struct Def { int x, z, w, h, d, style; };
    std::vector<Def> defs = {
        {  0,   0, 4, 14, 4, 3}, {-9,   0, 3, 11, 3, 0}, { 9,   0, 3,  9, 3, 0},
        {  0, -12, 4, 10, 4, 3}, {-9, -12, 3,  8, 3, 1}, { 9, -12, 3,  7, 3, 1},
        {-18,   0, 4,  6, 4, 1}, {18,   0, 4,  6, 4, 1}, {-18, -12, 4, 5, 4, 1},
        { 18, -12, 4,  5, 4, 1}, {-27,  -5, 5, 3, 6, 2}, {27,  -5, 5,  3, 6, 2},
        {  0, -24, 6,  3, 5, 2}, { 5,  12, 3,  9, 3, 0}, {-5,  12, 3,  7, 3, 3},
        { 14,  10, 4,  5, 4, 1}, {-14, 10, 4,  4, 4, 0}, {22, -18, 3,  6, 3, 3},
        {-22, -18, 3,  5, 3, 3},
    };
    for (auto& d : defs) {
        Building b;
        b.build(Vec3((float)d.x, 0, (float)d.z), d.w, d.h, d.d, d.style);
        buildings.push_back(std::move(b));
    }
}

void World::spawnVehicles() {
    vehicles.clear();
    auto add = [&](float x, float z, VehicleType t) {
        auto v = std::make_unique<Vehicle>();
        v->init(Vec3(x, v->body.halfSize.y, z), t);
        vehicles.push_back(std::move(v));
    };
    add( 5,  18, VehicleType::CAR);
    add(-5,  18, VehicleType::CAR);
    add(14,   6, VehicleType::TANK);
    add(-22,  0, VehicleType::BULLDOZER);
}

void World::update(float dt) {
    std::vector<RigidBody*> bodies;
    collectAllBodies(bodies);
    physics.update(dt, bodies);

    player.update(dt);
    weapons.update(dt);
    achievements.update(dt);
    anim.update(dt);

    // Vehicle ramming
    for (auto& v : vehicles) {
        if (v->occupied) {
            Vec3 fwd = v->getForward();
            float ramDmg = v->getRamDamage();
            if (ramDmg > 5.f) {
                for (auto& b : buildings) {
                    b.applyDamage(v->position + fwd * v->body.halfSize.z,
                                  v->body.halfSize.x * 1.5f, ramDmg * 0.1f);
                }
                if (ramDmg > 50.f) audio.play("building_break", 60.f);
            }
        }
    }

    // Building loose block physics
    for (auto& b : buildings) {
        std::vector<RigidBody*> bBodies;
        b.collectBodies(bBodies);
        physics.update(dt, bBodies);
        b.update(dt);
    }

    // Chain reaction tracking
    if (chainTimer > 0.f) {
        chainTimer -= dt;
    } else {
        chainCount = 0;
    }

    updateStats();
    checkAchievements();
}

void World::explodeAt(Vec3 pos, float radius, float strength,
                      const std::string& sndFire, const std::string& sndExplode) {
    std::vector<RigidBody*> bodies;
    collectAllBodies(bodies);
    physics.applyExplosion(pos, radius, strength, bodies);

    int glassHit = 0;
    for (auto& b : buildings) {
        int before = 0, after = 0;
        for (auto& blk : b.blocks) if (!blk.destroyed && blk.type == 1) before++;
        b.applyDamage(pos, radius, strength * 0.15f);
        for (auto& blk : b.blocks) if (!blk.destroyed && blk.type == 1) after++;
        glassHit += (before - after);

        if (glassHit > 0) audio.play("glass_break", 70.f);
    }
    glassDestroyed += glassHit;

    for (auto& v : vehicles) {
        Vec3 diff = v->position - pos;
        float dist = diff.length();
        if (dist < radius) {
            float fo = 1.f - dist/radius;
            v->health -= strength * fo * 0.05f;
            if (v->health <= 0 && !v->destroyed) {
                v->destroyed = true;
                audio.play("car_explode", 90.f);
                anim.spawnExplosionFX(v->position, 4.f, {1.f,0.4f,0.1f});
            }
        }
    }

    // Explosion VFX + audio
    audio.play(sndExplode.empty() ? "explosion_large" : sndExplode, 100.f);
    anim.spawnExplosionFX(pos, radius, {1.f, 0.5f, 0.1f});
    anim.triggerShake(radius * 0.3f, 0.5f + radius * 0.05f);
    audio.play("building_break", 65.f);

    explosionCount++;
    chainCount++;
    chainTimer = 3.f;
}

Vehicle* World::getNearbyVehicle(Vec3 pos, float range) {
    for (auto& v : vehicles) {
        if (v->destroyed) continue;
        if ((v->position - pos).length() < range) return v.get();
    }
    return nullptr;
}

void World::rebuild() {
    buildCity();
    spawnVehicles();
    player.position = {0, 1, 15};
    player.body.position = player.position;
    weapons.init();
    explosionCount = 0;
    totalDestructionPct = 0.f;
    chainCount = 0; chainTimer = 0.f;
    glassDestroyed = 0;
    anim.debris.clear();
    anim.shockwaves.clear();
    audio.play("ui_click");
}

void World::collectAllBodies(std::vector<RigidBody*>& out) {
    player.collectBody(out);
    for (auto& v : vehicles) v->collectBody(out);
}

void World::updateStats() {
    if (buildings.empty()) return;
    float total = 0;
    for (auto& b : buildings) total += b.destructionRatio();
    totalDestructionPct = total / buildings.size() * 100.f;
}

void World::checkAchievements() {
    if (explosionCount == 1)          achievements.check(AchievementID::FIRST_BLOOD);
    if (explosionCount >= 1)          achievements.checkValue(AchievementID::UNSTOPPABLE, explosionCount);
    if (totalDestructionPct >= 25.f)  achievements.check(AchievementID::DEMOLISHER);
    if (totalDestructionPct >= 50.f)  achievements.check(AchievementID::HALF_GONE);
    if (totalDestructionPct >= 99.f)  achievements.check(AchievementID::TOTAL_ANNIHILATION);
    achievements.checkValue(AchievementID::GLASS_CANNON, glassDestroyed);
    if (chainCount >= 5)              achievements.check(AchievementID::CHAIN_REACTION);

    int unlocked = weapons.defs[0].unlocked ? 0 : 0;
    int cnt = 0;
    for (auto& d : weapons.defs) if (d.unlocked) cnt++;
    achievements.checkValue(AchievementID::WEAPON_COLLECTOR, cnt);
}
