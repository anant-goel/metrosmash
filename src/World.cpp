#include "World.h"
#include "Log.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

static float rnd(float lo, float hi) { return lo + (float)rand() / RAND_MAX * (hi - lo); }

// ─────────────────────────────────────────────────────────────────────────────
//  Pedestrian AI
// ─────────────────────────────────────────────────────────────────────────────
void Pedestrian::pickNewTarget() {
    targetPos = {rnd(-28.f, 28.f), 0.f, rnd(-28.f, 28.f)};
    idleTimer = rnd(1.5f, 4.f);
}

void Pedestrian::panic(Vec3 fromPos) {
    panicking  = true;
    panicTimer = rnd(6.f, 12.f);
    Vec3 away = position - fromPos;
    float len = away.length();
    if (len > 0.1f) away = away / len;
    targetPos = position + away * rnd(20.f, 40.f);
    targetPos.y = 0.f;
}

void Pedestrian::update(float dt) {
    if (panicTimer > 0.f) panicTimer -= dt;
    else panicking = false;

    float spd = panicking ? panicSpeed : speed;
    Vec3 diff = targetPos - position;
    diff.y = 0.f;
    float dist = diff.length();

    if (dist < 0.8f || idleTimer <= 0.f) {
        idleTimer -= dt;
        if (idleTimer <= 0.f || panicking) pickNewTarget();
        return;
    }

    Vec3 dir = diff / dist;
    yaw = atan2f(dir.x, -dir.z);
    position.x += dir.x * spd * dt;
    position.z += dir.z * spd * dt;
    position.y = 0.f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  World::init
// ─────────────────────────────────────────────────────────────────────────────
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
        Log::info("Achievement unlocked: id=%d", (int)a.id);
    };

    buildCity();
    spawnVehicles();
    spawnPedestrians();
    spawnVehicleAIs();
    spawnVegetation();

    Log::info("World init: %d buildings, %d vehicles, %d pedestrians, %d vegetation",
              (int)buildings.size(), (int)vehicles.size(), (int)pedestrians.size(), (int)vegetation.size());
}

// ─────────────────────────────────────────────────────────────────────────────
//  City generation
// ─────────────────────────────────────────────────────────────────────────────
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
    add( 8,  -5, VehicleType::CAR);
    add(-8,  -5, VehicleType::CAR);
    add( 5, -20, VehicleType::CAR);
    add(-5, -20, VehicleType::CAR);
    add(22,  -8, VehicleType::CAR);
    add(-22, -8, VehicleType::CAR);
    add(14,   6, VehicleType::TANK);
    add(-22,  0, VehicleType::BULLDOZER);
}

void World::spawnPedestrians() {
    pedestrians.clear();
    for (int i = 0; i < 30; i++) {
        Pedestrian p;
        p.position = {rnd(-28.f, 28.f), 0.f, rnd(-28.f, 28.f)};
        p.speed    = rnd(1.0f, 1.8f);
        p.pickNewTarget();
        pedestrians.push_back(p);
    }
}

void World::spawnVehicleAIs() {
    vehicleAIs.clear();
    for (int i = 0; i < (int)vehicles.size(); i++) {
        if (vehicles[i]->type != VehicleType::CAR) continue;
        if (vehicleAIs.size() >= 6) break;
        VehicleAI ai;
        ai.vehicleIdx = i;
        Vec3 c = vehicles[i]->position;
        float r = rnd(8.f, 16.f);
        ai.waypoints[0] = {c.x + r, 0, c.z + r};
        ai.waypoints[1] = {c.x - r, 0, c.z + r};
        ai.waypoints[2] = {c.x - r, 0, c.z - r};
        ai.waypoints[3] = {c.x + r, 0, c.z - r};
        ai.speed = rnd(5.f, 9.f);
        vehicleAIs.push_back(ai);
    }
}

void World::spawnVegetation() {
    vegetation.clear();

    // Helper lambdas
    auto addTree = [&](float x, float z, float h = 0.f, float r = 0.f) {
        VegetationNode n;
        n.kind      = VegetationNode::Kind::TREE;
        n.position  = {x, 0.f, z};
        n.height    = (h > 0 ? h : rnd(3.5f, 7.f));
        n.radius    = (r > 0 ? r : rnd(1.4f, 2.8f));
        // Seasonal variety — mostly green, occasional autumn
        int variety = rand() % 6;
        if      (variety == 0) n.leafColor = {0.6f,  0.35f, 0.05f}; // autumn orange
        else if (variety == 1) n.leafColor = {0.25f, 0.5f,  0.1f};  // lime
        else if (variety == 2) n.leafColor = {0.08f, 0.45f, 0.08f}; // dark green
        else                   n.leafColor = {0.15f, 0.55f, 0.12f}; // standard
        n.swayPhase = rnd(0.f, 6.28f);
        n.swayAmp   = rnd(0.025f, 0.06f);
        vegetation.push_back(n);
    };

    auto addBush = [&](float x, float z) {
        VegetationNode n;
        n.kind      = VegetationNode::Kind::BUSH;
        n.position  = {x, 0.f, z};
        n.height    = rnd(0.5f, 1.2f);
        n.radius    = rnd(0.6f, 1.1f);
        n.leafColor = {0.1f, 0.45f, 0.08f};
        n.swayPhase = rnd(0.f, 6.28f);
        n.swayAmp   = rnd(0.01f, 0.03f);
        vegetation.push_back(n);
    };

    auto addLamp = [&](float x, float z) {
        VegetationNode n;
        n.kind      = VegetationNode::Kind::LAMP;
        n.position  = {x, 0.f, z};
        n.height    = 5.5f;
        n.radius    = 0.3f;
        n.leafColor = {0.9f, 0.85f, 0.5f}; // warm light colour
        vegetation.push_back(n);
    };

    // ── Park in the open spaces between buildings ─────────────────────────
    // Central plaza (0, 0 area — between buildings)
    for (int i = 0; i < 6; i++)
        addTree(rnd(-7.f,-4.f), rnd(-8.f, 8.f));
    for (int i = 0; i < 6; i++)
        addTree(rnd( 4.f, 7.f), rnd(-8.f, 8.f));

    // Sidewalk trees along main axis
    static const float treeX[] = {-3.5f, 3.5f};
    for (float tx : treeX)
        for (float tz = -22.f; tz <= 22.f; tz += 5.5f)
            addTree(tx, tz, 4.5f, 1.6f);

    // East/west boulevard
    for (float tz : {-3.f, -9.f, -15.f}) {
        for (float tx = -32.f; tx <= 32.f; tx += 6.f) {
            addTree(tx, tz + rnd(-0.5f, 0.5f), 4.f, 1.5f);
        }
    }

    // Scattered park trees
    for (int i = 0; i < 20; i++)
        addTree(rnd(-35.f, 35.f), rnd(-30.f, 5.f));

    // Bushes along building bases
    for (auto& b : buildings) {
        float bx = b.origin.x, bz = b.origin.z;
        for (int i = 0; i < 4; i++)
            addBush(bx + rnd(-b.width*0.6f, b.width*0.6f),
                    bz + rnd(-b.depth*0.6f, b.depth*0.6f));
    }

    // Street lamps on sidewalks
    static const float lampZ[] = {-6.f, 0.f, 6.f, 12.f, -12.f, -18.f, 18.f};
    for (float lz : lampZ) {
        addLamp(-4.5f, lz);
        addLamp( 4.5f, lz);
    }
    for (float lx = -30.f; lx <= 30.f; lx += 10.f) {
        addLamp(lx, -6.5f);
        addLamp(lx,  6.5f);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Update
// ─────────────────────────────────────────────────────────────────────────────
void World::update(float dt) {
    std::vector<RigidBody*> bodies;
    collectAllBodies(bodies);
    physics.update(dt, bodies);

    player.update(dt);
    weapons.update(dt);
    achievements.update(dt);
    anim.update(dt);

    for (auto& v : vehicles) {
        if (v->occupied) {
            Vec3 fwd = v->getForward();
            float ramDmg = v->getRamDamage();
            if (ramDmg > 5.f) {
                for (auto& b : buildings)
                    b.applyDamage(v->position + fwd * v->body.halfSize.z,
                                  v->body.halfSize.x * 1.5f, ramDmg * 0.1f);
                if (ramDmg > 50.f) audio.play("building_break", 60.f);
            }
        }
    }

    for (auto& b : buildings) {
        std::vector<RigidBody*> bBodies;
        b.collectBodies(bBodies);
        physics.update(dt, bBodies);
        b.update(dt);
    }

    if (chainTimer > 0.f) chainTimer -= dt;
    else chainCount = 0;

    updateCityAI(dt);
    updateFires(dt);
    updateParticles(dt);
    updateStats();
    checkAchievements();
}

// ─────────────────────────────────────────────────────────────────────────────
//  City AI
// ─────────────────────────────────────────────────────────────────────────────
void World::updateCityAI(float dt) {
    for (auto& p : pedestrians)
        p.update(dt);

    for (auto& ai : vehicleAIs) {
        if (!ai.active || ai.vehicleIdx < 0 || ai.vehicleIdx >= (int)vehicles.size()) continue;
        Vehicle& v = *vehicles[ai.vehicleIdx];
        if (v.destroyed || v.occupied) continue;

        Vec3 target = ai.waypoints[ai.waypointIdx];
        Vec3 diff   = target - v.position;
        diff.y = 0;
        float dist  = diff.length();

        if (dist < 2.f) {
            ai.waypointIdx = (ai.waypointIdx + 1) % 4;
            ai.stuckTimer  = 0.f;
        } else {
            Vec3 dir = diff / dist;
            float targetYaw = atan2f(dir.x, -dir.z);
            float yawDiff   = targetYaw - v.yaw;
            while (yawDiff >  3.14159f) yawDiff -= 6.28318f;
            while (yawDiff < -3.14159f) yawDiff += 6.28318f;
            float steer = std::max(-1.f, std::min(1.f, yawDiff * 2.f));
            v.update(dt, true, false, steer);

            ai.stuckTimer += dt;
            if (ai.stuckTimer > 4.f) {
                ai.waypointIdx = (ai.waypointIdx + 1) % 4;
                ai.stuckTimer  = 0.f;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Fire system
// ─────────────────────────────────────────────────────────────────────────────
void World::igniteAt(Vec3 pos, float intensity) {
    if ((int)fires.size() >= 60) return;
    FireCell f;
    f.position  = pos;
    f.intensity = intensity;
    f.maxLife   = rnd(6.f, 14.f);
    fires.push_back(f);
}

void World::updateFires(float dt) {
    for (auto& f : fires) {
        if (f.done()) continue;
        f.lifetime += dt;

        if (f.lifetime > f.maxLife * 0.6f) {
            f.intensity -= dt * 0.3f;
            if (f.intensity <= 0.f) { f.dying = true; f.intensity = 0.f; }
        }

        // Fire particles
        if (!f.dying && (rand() % 4 == 0)) {
            if ((int)particles.size() < 400) {
                ParticleEffect p;
                p.position = f.position + Vec3(rnd(-0.3f,0.3f), 0.f, rnd(-0.3f,0.3f));
                p.velocity = {rnd(-0.5f,0.5f), rnd(2.f,5.f), rnd(-0.5f,0.5f)};
                p.color    = {1.f, rnd(0.2f,0.5f), 0.f};
                p.size     = rnd(0.15f, 0.4f) * f.intensity;
                p.life     = p.maxLife = rnd(0.6f, 1.4f);
                particles.push_back(p);
            }
            // Smoke
            if ((int)particles.size() < 400) {
                ParticleEffect s;
                s.position = f.position + Vec3(rnd(-0.2f,0.2f), rnd(0.5f,1.5f), rnd(-0.2f,0.2f));
                s.velocity = {rnd(-0.3f,0.3f), rnd(1.f,2.5f), rnd(-0.3f,0.3f)};
                s.color    = {0.25f, 0.25f, 0.25f};
                s.size     = rnd(0.3f, 0.8f);
                s.life     = s.maxLife = rnd(1.5f, 3.f);
                particles.push_back(s);
            }
        }

        // Spread to nearby blocks
        f.spreadTimer += dt;
        if (f.spreadTimer > 2.5f && f.intensity > 0.5f) {
            f.spreadTimer = 0.f;
            for (auto& b : buildings) {
                for (auto& blk : b.blocks) {
                    if (blk.destroyed) continue;
                    if ((blk.position - f.position).length() < 2.5f && blk.type != 1) {
                        blk.health -= 8.f * f.intensity;
                        if (blk.health <= 0) {
                            blk.destroyed = true;
                            blk.body.active = false;
                            igniteAt(blk.position, 0.7f);
                        }
                    }
                }
            }
        }
    }

    fires.erase(std::remove_if(fires.begin(), fires.end(),
        [](const FireCell& f){ return f.done(); }), fires.end());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Particle update
// ─────────────────────────────────────────────────────────────────────────────
void World::updateParticles(float dt) {
    for (auto& p : particles) p.update(dt);
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const ParticleEffect& p){ return p.done(); }), particles.end());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Explosion
// ─────────────────────────────────────────────────────────────────────────────
void World::explodeAt(Vec3 pos, float radius, float strength,
                      const std::string& sndFire, const std::string& sndExplode) {
    Log::info("Explosion at (%.1f,%.1f,%.1f) r=%.1f str=%.0f",
              pos.x, pos.y, pos.z, radius, strength);

    std::vector<RigidBody*> bodies;
    collectAllBodies(bodies);
    physics.applyExplosion(pos, radius, strength, bodies);

    int glassHit = 0;
    for (auto& b : buildings) {
        int before = 0;
        for (auto& blk : b.blocks) if (!blk.destroyed && blk.type == 1) before++;

        b.applyDamage(pos, radius, strength * 0.15f);

        int after = 0;
        for (auto& blk : b.blocks) if (!blk.destroyed && blk.type == 1) after++;
        glassHit += before - after;

        // Start fires inside blast radius on strong explosions
        if (strength > 500.f) {
            for (auto& blk : b.blocks) {
                if (blk.destroyed) continue;
                if ((blk.position - pos).length() < radius * 0.6f)
                    igniteAt(blk.position, std::min(1.f, strength / 3000.f));
            }
        }
    }
    if (glassHit > 0) audio.play("glass_break", 70.f);
    glassDestroyed += glassHit;

    // Vehicle damage + secondary fires
    for (auto& v : vehicles) {
        Vec3 diff = v->position - pos;
        float dist = diff.length();
        if (dist < radius) {
            float fo = 1.f - dist / radius;
            v->health -= strength * fo * 0.05f;
            if (v->health <= 0 && !v->destroyed) {
                v->destroyed = true;
                audio.play("car_explode", 90.f);
                anim.spawnExplosionFX(v->position, 4.f, {1.f,0.4f,0.1f});
                igniteAt(v->position, 1.f);
                Log::info("Vehicle destroyed by explosion");
            }
        }
    }

    // Pedestrians flee
    for (auto& p : pedestrians) {
        if ((p.position - pos).length() < radius * 2.5f)
            p.panic(pos);
    }

    // Set nearby trees on fire
    for (auto& v : vegetation) {
        if (v.burning) continue;
        if ((v.position - pos).length() < radius * 1.2f) {
            v.burning   = true;
            v.burnTimer = rnd(4.f, 10.f);
            igniteAt(v.position + Vec3(0, v.height * 0.5f, 0), 0.8f);
        }
    }

    // Burning trees decay
    for (auto& v : vegetation) {
        if (!v.burning) continue;
    }

    audio.play(sndExplode.empty() ? "explosion_large" : sndExplode, 100.f);
    anim.spawnExplosionFX(pos, radius, {1.f, 0.5f, 0.1f});
    anim.triggerShake(radius * 0.3f, 0.5f + radius * 0.05f);
    audio.play("building_break", 65.f);

    // Ground-zero fire + scatter fires
    igniteAt(pos, 1.f);
    for (int i = 0; i < 4; i++) {
        Vec3 off = {rnd(-radius*0.5f, radius*0.5f), 0.f, rnd(-radius*0.5f, radius*0.5f)};
        igniteAt(pos + off, rnd(0.4f, 0.9f));
    }

    // Debris particles
    for (int i = 0; i < 20 && (int)particles.size() < 400; i++) {
        ParticleEffect p;
        p.position = pos + Vec3(0, 1.f, 0);
        p.velocity = {rnd(-8.f,8.f), rnd(5.f,15.f), rnd(-8.f,8.f)};
        p.color    = {rnd(0.4f,0.7f), rnd(0.3f,0.5f), 0.25f};
        p.size     = rnd(0.1f, 0.4f);
        p.life     = p.maxLife = rnd(1.f, 3.f);
        particles.push_back(p);
    }

    explosionCount++;
    chainCount++;
    chainTimer = 3.f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────
Vehicle* World::getNearbyVehicle(Vec3 pos, float range) {
    for (auto& v : vehicles) {
        if (v->destroyed) continue;
        if ((v->position - pos).length() < range) return v.get();
    }
    return nullptr;
}

void World::rebuild() {
    Log::info("World rebuild");
    fires.clear();
    particles.clear();
    vegetation.clear();
    buildCity();
    spawnVehicles();
    spawnPedestrians();
    spawnVehicleAIs();
    spawnVegetation();
    player.position = {0, 1, 15};
    player.body.position = player.position;
    weapons.init();
    explosionCount = 0; totalDestructionPct = 0.f;
    chainCount = 0;     chainTimer = 0.f;
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

    int cnt = 0;
    for (auto& d : weapons.defs) if (d.unlocked) cnt++;
    achievements.checkValue(AchievementID::WEAPON_COLLECTOR, cnt);
}
