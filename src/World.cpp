#include "World.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <functional>

static float rnd(float lo, float hi) {
    return lo + (float)rand() / RAND_MAX * (hi - lo);
}

void World::init() {
    srand(42);
    player.init();
    player.position = {0, 1, 10};
    player.body.position = player.position;
    buildCity();
    spawnVehicles();
}

void World::buildCity() {
    buildings.clear();

    // Grid of city blocks
    // Central district — tall buildings
    struct BuildingDef { int x, z, w, h, d, style; };
    std::vector<BuildingDef> defs = {
        // Downtown towers
        { 0,  0,  4, 12, 4, 3},
        {-8,  0,  3, 10, 3, 0},
        { 8,  0,  3,  8, 3, 0},
        { 0, -10, 4,  9, 4, 3},
        {-8, -10, 3,  7, 3, 1},
        { 8, -10, 3,  6, 3, 1},
        // Mid-range apartments
        {-16,  0,  4,  5, 4, 1},
        { 16,  0,  4,  5, 4, 1},
        {-16,-10,  4,  4, 4, 1},
        { 16,-10,  4,  4, 4, 1},
        // Warehouses on outskirts
        {-24, -5,  5,  3, 6, 2},
        { 24, -5,  5,  3, 6, 2},
        {  0, -20,  6,  2, 5, 2},
        // Extra towers
        { 4,  10,  3,  8, 3, 0},
        {-4,  10,  3,  6, 3, 3},
    };

    for (auto& def : defs) {
        Building b;
        b.build(Vec3((float)def.x, 0, (float)def.z),
                def.w, def.h, def.d, def.style);
        buildings.push_back(std::move(b));
    }
}

void World::spawnVehicles() {
    vehicles.clear();

    auto addVehicle = [&](float x, float z, VehicleType t) {
        auto v = std::make_unique<Vehicle>();
        v->init(Vec3(x, v->body.halfSize.y, z), t);
        vehicles.push_back(std::move(v));
    };

    addVehicle( 5,  15, VehicleType::CAR);
    addVehicle(-5,  15, VehicleType::CAR);
    addVehicle(12,   5, VehicleType::TANK);
    addVehicle(-20,  0, VehicleType::BULLDOZER);
}

void World::update(float dt) {
    // Collect all physics bodies
    std::vector<RigidBody*> bodies;
    collectAllBodies(bodies);

    // Physics step
    physics.update(dt, bodies);

    // Player update
    player.update(dt);

    // Vehicle updates (driving handled in Game.cpp input)
    for (auto& v : vehicles) {
        if (v->occupied) vehicleRamBuildings(*v);
    }

    // Building updates
    for (auto& b : buildings) {
        std::vector<RigidBody*> bBodies;
        b.collectBodies(bBodies);
        physics.update(dt, bBodies);
        b.update(dt);
    }

    // Particle updates
    for (auto& p : particles) {
        p.velocity.y -= 15.f * dt;
        p.position   += p.velocity * dt;
        p.life       -= dt;
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const ParticleEffect& p){ return p.life <= 0; }),
        particles.end());

    updateStats();
}

void World::explodeAt(Vec3 pos, float radius, float strength) {
    // Collect all bodies
    std::vector<RigidBody*> bodies;
    collectAllBodies(bodies);

    // Physics explosion impulse
    physics.applyExplosion(pos, radius, strength, bodies);

    // Damage buildings
    for (auto& b : buildings) {
        b.applyDamage(pos, radius, strength * 0.15f);
        // Spawn debris at explosion site
        for (auto& blk : b.blocks) {
            if (!blk.destroyed) continue;
            Vec3 diff = blk.position - pos;
            if (diff.length() < radius * 1.2f) {
                spawnDebris(blk.position, blk.color, 3);
            }
        }
    }

    // Damage nearby vehicles
    for (auto& v : vehicles) {
        Vec3 diff = v->position - pos;
        float dist = diff.length();
        if (dist < radius) {
            float falloff = 1.f - dist/radius;
            v->health -= strength * falloff * 0.05f;
            if (v->health <= 0) v->destroyed = true;
        }
    }

    spawnDebris(pos, {1.f, 0.5f, 0.1f}, 30); // fire/smoke particles
    explosionsDetonated++;
}

void World::vehicleRamBuildings(Vehicle& v) {
    Vec3 fwd = v.getForward();
    float ramDmg = v.getRamDamage();
    if (ramDmg < 5.f) return;

    for (auto& b : buildings) {
        b.applyDamage(v.position + fwd * v.body.halfSize.z,
                      v.body.halfSize.x * 1.5f, ramDmg * 0.1f);
    }
}

Vehicle* World::getNearbyVehicle(Vec3 pos, float range) {
    for (auto& v : vehicles) {
        if (v->destroyed) continue;
        Vec3 diff = v->position - pos;
        if (diff.length() < range) return v.get();
    }
    return nullptr;
}

void World::spawnDebris(Vec3 pos, Vec3 color, int count) {
    for (int i = 0; i < count; i++) {
        ParticleEffect p;
        p.position = pos;
        p.velocity = Vec3(rnd(-5,5), rnd(2,10), rnd(-5,5));
        p.color    = color;
        p.maxLife  = p.life = rnd(0.8f, 2.5f);
        p.size     = rnd(0.15f, 0.5f);
        particles.push_back(p);
    }
}

void World::rebuild() {
    buildCity();
    spawnVehicles();
    player.position = {0, 1, 10};
    player.body.position = player.position;
    player.explosiveCount = 10;
    player.placedExplosives.clear();
    explosionsDetonated = 0;
    totalDestructionPct = 0.f;
    particles.clear();
}

void World::collectAllBodies(std::vector<RigidBody*>& out) {
    player.collectBody(out);
    for (auto& v : vehicles) v->collectBody(out);
}

void World::updateStats() {
    if (buildings.empty()) { totalDestructionPct = 0; return; }
    float total = 0;
    for (auto& b : buildings) total += b.destructionRatio();
    totalDestructionPct = total / buildings.size() * 100.f;
}
