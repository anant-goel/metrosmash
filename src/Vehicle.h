#pragma once
#include "GameMath.h"
#include "Physics.h"

enum class VehicleType { CAR, TANK, BULLDOZER };

class Vehicle {
public:
    Vec3        position {0,0.8f,0};
    float       yaw      = 0.f;
    float       speed    = 0.f;
    float       maxSpeed = 18.f;
    float       health   = 300.f;
    VehicleType type     = VehicleType::CAR;
    RigidBody   body;
    bool        occupied = false;
    bool        destroyed= false;

    // Visual colors
    Vec3 bodyColor  {0.2f, 0.6f, 0.2f};
    Vec3 accentColor{0.1f, 0.3f, 0.1f};

    void init(Vec3 pos, VehicleType t);
    void update(float dt, bool accel, bool brake, float steer);
    void ram(Vec3 dir, float force); // apply ram impulse

    Vec3 getForward() const { return {sinf(yaw), 0, -cosf(yaw)}; }
    Vec3 getRight()   const { return {cosf(yaw), 0,  sinf(yaw)}; }

    void collectBody(std::vector<RigidBody*>& out) {
        out.push_back(&body);
    }

    float getRamDamage() const {
        return std::abs(speed) * (type == VehicleType::TANK ? 80.f :
                                  type == VehicleType::BULLDOZER ? 120.f : 40.f);
    }
};
