#pragma once
#include "GameMath.h"

struct ParticleEffect {
    Vec3 position{};
    Vec3 velocity{};
    Vec3 color{1.f, 0.6f, 0.1f};
    float size = 0.08f;
    float life = 1.f;
    float maxLife = 1.f;

    bool done() const { return life <= 0.f; }
    void update(float dt) {
        life -= dt;
        position += velocity * dt;
        velocity.y -= 9.8f * dt;
    }
};
