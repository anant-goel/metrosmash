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

class World {
public:
    std::vector<Building>                 buildings;
    std::vector<std::unique_ptr<Vehicle>> vehicles;
    std::vector<ParticleEffect>           particles;   // visual FX particles

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

private:
    void buildCity();
    void spawnVehicles();
    void collectAllBodies(std::vector<RigidBody*>& out);
    void updateStats();
    void checkAchievements();
};
