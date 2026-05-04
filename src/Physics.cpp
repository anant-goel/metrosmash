#include "Physics.h"
#include <cmath>

void Physics::update(float dt, std::vector<RigidBody*>& bodies) {
    // Update explosion timers
    for (auto& e : activeExplosions) e.timeLeft -= dt;
    activeExplosions.erase(
        std::remove_if(activeExplosions.begin(), activeExplosions.end(),
            [](const ExplosionForce& e){ return e.timeLeft <= 0; }),
        activeExplosions.end());

    for (auto* b : bodies) {
        if (!b->active || b->isStatic) continue;

        // Gravity
        b->acceleration += Vec3(0, GRAVITY, 0);

        // Integrate velocity
        b->velocity += b->acceleration * dt;
        b->position += b->velocity * dt;

        // Reset acceleration (forces re-applied each frame)
        b->acceleration = {0,0,0};

        // Ground collision
        resolveGroundCollision(*b);
    }

    // Body-body collisions (simple O(n^2) for sandbox scale)
    for (size_t i = 0; i < bodies.size(); i++) {
        for (size_t j = i+1; j < bodies.size(); j++) {
            RigidBody* a = bodies[i];
            RigidBody* b = bodies[j];
            if (!a->active || !b->active) continue;
            if (a->isStatic && b->isStatic) continue;
            if (aabbOverlap(*a, *b)) {
                resolveBodyCollision(*a, *b);
            }
        }
    }
}

void Physics::applyExplosion(Vec3 origin, float radius, float strength,
                              std::vector<RigidBody*>& bodies) {
    activeExplosions.push_back({origin, radius, strength, 0.5f});

    for (auto* b : bodies) {
        if (!b->active || b->isStatic) continue;
        Vec3 dir = b->position - origin;
        float dist = dir.length();
        if (dist < radius && dist > 0.1f) {
            float falloff = 1.f - (dist / radius);
            falloff = falloff * falloff; // quadratic falloff
            Vec3 impulse = dir.normalized() * (strength * falloff / b->mass);
            impulse.y = std::abs(impulse.y) + strength * falloff * 0.3f; // upward bias
            b->applyImpulse(impulse);
            b->onGround = false;
        }
    }
}

void Physics::resolveGroundCollision(RigidBody& b) {
    float bottom = b.position.y - b.halfSize.y;
    if (bottom < GROUND_Y) {
        b.position.y = GROUND_Y + b.halfSize.y;
        if (b.velocity.y < 0) {
            b.velocity.y = -b.velocity.y * b.restitution;
            if (std::abs(b.velocity.y) < 0.5f) b.velocity.y = 0;
        }
        // Friction
        b.velocity.x *= b.friction;
        b.velocity.z *= b.friction;
        b.onGround = true;
    } else {
        b.onGround = false;
    }
}

bool Physics::aabbOverlap(const RigidBody& a, const RigidBody& b) {
    return (std::abs(a.position.x - b.position.x) < a.halfSize.x + b.halfSize.x) &&
           (std::abs(a.position.y - b.position.y) < a.halfSize.y + b.halfSize.y) &&
           (std::abs(a.position.z - b.position.z) < a.halfSize.z + b.halfSize.z);
}

void Physics::resolveBodyCollision(RigidBody& a, RigidBody& b) {
    Vec3 delta = b.position - a.position;
    Vec3 overlap(
        (a.halfSize.x + b.halfSize.x) - std::abs(delta.x),
        (a.halfSize.y + b.halfSize.y) - std::abs(delta.y),
        (a.halfSize.z + b.halfSize.z) - std::abs(delta.z)
    );

    // Resolve along smallest overlap axis
    Vec3 normal;
    float pen;
    if (overlap.x < overlap.y && overlap.x < overlap.z) {
        pen = overlap.x;
        normal = {delta.x < 0 ? -1.f : 1.f, 0, 0};
    } else if (overlap.y < overlap.z) {
        pen = overlap.y;
        normal = {0, delta.y < 0 ? -1.f : 1.f, 0};
    } else {
        pen = overlap.z;
        normal = {0, 0, delta.z < 0 ? -1.f : 1.f};
    }

    // Separate
    float totalMass = a.mass + b.mass;
    if (!a.isStatic) a.position -= normal * pen * (b.mass / totalMass);
    if (!b.isStatic) b.position += normal * pen * (a.mass / totalMass);

    // Impulse response
    Vec3 relVel = b.velocity - a.velocity;
    float velAlongNormal = relVel.dot(normal);
    if (velAlongNormal > 0) return;

    float e = std::min(a.restitution, b.restitution);
    float j = -(1.f + e) * velAlongNormal / (1.f/a.mass + 1.f/b.mass);
    Vec3 impulse = normal * j;
    if (!a.isStatic) a.velocity -= impulse * (1.f / a.mass);
    if (!b.isStatic) b.velocity += impulse * (1.f / b.mass);
}
