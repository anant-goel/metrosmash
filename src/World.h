#pragma once
#include "Building.h"
#include "Vehicle.h"
#include "Player.h"
#include "Physics.h"
#include "Weapons.h"
#include "Achievements.h"
#include "Animation.h"
#include "AudioManager.h"
#include <vector>
#include <memory>
#include <string>

// ── Pedestrian AI ────────────────────────────────────────────────────────────
struct Pedestrian {
    Vec3  position;
    Vec3  targetPos;
    float yaw       = 0.f;
    float speed     = 1.4f;   // m/s walking
    float panicSpeed= 4.5f;
    bool  panicking = false;
    float panicTimer= 0.f;
    float idleTimer = 0.f;    // time before picking new waypoint

    void update(float dt);
    void pickNewTarget();
    void panic(Vec3 fromPos);
};

// ── Fire cell ─────────────────────────────────────────────────────────────────
struct FireCell {
    Vec3  position;
    float intensity = 1.f;    // 0–1
    float lifetime  = 0.f;
    float maxLife   = 8.f + (float)(rand() % 6);
    float spreadTimer = 0.f;
    bool  dying     = false;
    bool  done() const { return dying && intensity <= 0.f; }
};

// ── Patrol AI for unoccupied vehicles ────────────────────────────────────────
struct VehicleAI {
    int   vehicleIdx  = -1;
    Vec3  waypoints[4];
    int   waypointIdx = 0;
    float speed       = 6.f;
    float stuckTimer  = 0.f;
    bool  active      = true;
};

class World {
public:
    std::vector<Building>                 buildings;
    std::vector<std::unique_ptr<Vehicle>> vehicles;
    std::vector<ParticleEffect>           particles;   // visual FX particles

    // City life
    std::vector<Pedestrian>    pedestrians;
    std::vector<FireCell>      fires;
    std::vector<VehicleAI>     vehicleAIs;
    std::vector<VegetationNode> vegetation;  // trees, bushes, lamps

    Player           player;
    Physics          physics;
    WeaponSystem     weapons;
    AchievementSystem achievements;
    AnimationSystem  anim;
    AudioManager     audio;

    // Stats
    float totalDestructionPct = 0.f;
    int   explosionCount      = 0;
    float chainTimer          = 0.f;
    int   chainCount          = 0;
    int   glassDestroyed      = 0;

    void init(const std::string& assetDir);
    void update(float dt);
    void rebuild();

    void explodeAt(Vec3 pos, float radius, float strength,
                   const std::string& sndFire, const std::string& sndExplode);

    Vehicle* getNearbyVehicle(Vec3 pos, float range = 3.f);

    // Spawn fire at position (called by explosions / building damage)
    void igniteAt(Vec3 pos, float intensity = 1.f);

private:
    void buildCity();
    void spawnVehicles();
    void spawnPedestrians();
    void spawnVehicleAIs();
    void spawnVegetation();
    void updateCityAI(float dt);
    void updateFires(float dt);
    void updateParticles(float dt);
    void collectAllBodies(std::vector<RigidBody*>& out);
    void updateStats();
    void checkAchievements();
};
