#pragma once
#include "GameMath.h"
#include <vector>

struct RigidBody {
    Vec3  position;
    Vec3  velocity;
    Vec3  acceleration;
    float mass       = 1.f;
    float restitution= 0.3f;   // bounciness
    float friction   = 0.85f;  // damping per frame on ground
    bool  onGround   = false;
    bool  active     = true;
    bool  isStatic   = false;

    // AABB half-extents
    Vec3 halfSize {0.5f, 0.5f, 0.5f};

    void applyForce(Vec3 f) {
        if (!isStatic) acceleration += f * (1.f / mass);
    }
    void applyImpulse(Vec3 impulse) {
        if (!isStatic) velocity += impulse * (1.f / mass);
    }
};

struct ExplosionForce {
    Vec3  origin;
    float radius;
    float strength;
    float timeLeft; // seconds to live (for VFX)
};

class Physics {
public:
    static constexpr float GRAVITY = -18.f;
    static constexpr float GROUND_Y = 0.f;

    void update(float dt, std::vector<RigidBody*>& bodies);
    void applyExplosion(Vec3 origin, float radius, float strength,
                        std::vector<RigidBody*>& bodies);

    std::vector<ExplosionForce> activeExplosions;

private:
    void resolveGroundCollision(RigidBody& b);
    void resolveBodyCollision(RigidBody& a, RigidBody& b);
    bool aabbOverlap(const RigidBody& a, const RigidBody& b);
    Vec3 aabbPenetration(const RigidBody& a, const RigidBody& b);
};
