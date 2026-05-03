#pragma once
#include "Building.h"
#include "Vehicle.h"
#include "Player.h"
#include "Physics.h"
#include <vector>
#include <memory>

struct ParticleEffect {
    Vec3  position;
    Vec3  velocity;
    Vec3  color;
    float life;       // remaining seconds
    float maxLife;
    float size;
};

class World {
public:
    std::vector<Building>                 buildings;
    std::vector<std::unique_ptr<Vehicle>> vehicles;
    std::vector<ParticleEffect>           particles;

    Player  player;
    Physics physics;

    // Score / stats
    float   totalDestructionPct = 0.f;
    int     explosionsDetonated = 0;

    void init();
    void update(float dt);
    void rebuild(); // reset map

    // Trigger explosion at world position
    void explodeAt(Vec3 pos, float radius, float strength);

    // Check vehicle entry/exit
    Vehicle* getNearbyVehicle(Vec3 pos, float range = 3.f);

    // Spawn debris particles
    void spawnDebris(Vec3 pos, Vec3 color, int count);

    // Ram buildings with vehicle
    void vehicleRamBuildings(Vehicle& v);

private:
    void buildCity();
    void spawnVehicles();
    void collectAllBodies(std::vector<RigidBody*>& out);
    void updateStats();
};
