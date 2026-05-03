#pragma once
#include <functional>
#include "Math.h"
#include "Physics.h"

enum class PlayerMode { ON_FOOT, IN_VEHICLE };

struct Explosive {
    Vec3  position;
    float fuseTime;    // seconds until detonation
    float radius;
    float strength;
    bool  armed = true;
    bool  detonated = false;
};

class Player {
public:
    Vec3        position    {0, 1, 0};
    float       yaw         = 0.f;
    float       pitch       = 0.f;
    float       speed       = 8.f;
    float       jumpForce   = 10.f;
    float       health      = 100.f;
    PlayerMode  mode        = PlayerMode::ON_FOOT;
    RigidBody   body;

    // Explosive inventory
    int         explosiveCount = 10;
    std::vector<Explosive> placedExplosives;

    // Input state
    bool  moveF=false, moveB=false, moveL=false, moveR=false;
    bool  jumping = false;
    bool  wantsPlace = false;
    bool  wantsDetonate = false;

    void init();
    void update(float dt);

    void placeExplosive();
    void detonateAll(std::vector<RigidBody*>& bodies,
                     std::function<void(Vec3,float,float)> onExplode);

    void enterVehicle(Vec3 vehiclePos);
    void exitVehicle();

    Vec3 getForward() const {
        return {sinf(yaw), 0, -cosf(yaw)};
    }
    Vec3 getRight() const {
        return {cosf(yaw), 0, sinf(yaw)};
    }

    void collectBody(std::vector<RigidBody*>& out) {
        if (mode == PlayerMode::ON_FOOT) out.push_back(&body);
    }
};
