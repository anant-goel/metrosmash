#include "pch.h"
#include "Animation.h"
#include <cstdlib>
#include <algorithm>

static float rnd(float lo, float hi) { return lo + (float)rand()/RAND_MAX*(hi-lo); }

void AnimationSystem::spawnExplosionFX(Vec3 origin, float radius, Vec3 color) {
    // Primary shockwave
    ShockwaveAnim sw;
    sw.origin    = origin;
    sw.maxRadius = radius * 1.5f;
    sw.life = sw.maxLife = 0.6f;
    sw.color     = color;
    shockwaves.push_back(sw);

    // Secondary smaller shockwave
    sw.maxRadius = radius * 0.8f;
    sw.life = sw.maxLife = 0.4f;
    sw.color = {1.f, 0.8f, 0.3f};
    shockwaves.push_back(sw);

    spawnDebris(origin, color, (int)(radius * 3));
}

void AnimationSystem::spawnDebris(Vec3 origin, Vec3 color, int count) {
    for (int i = 0; i < count; i++) {
        DebrisAnim d;
        d.pos      = origin + Vec3(rnd(-1,1), rnd(0,1), rnd(-1,1));
        float spd  = rnd(4.f, 18.f);
        float ang  = rnd(0, 6.28f);
        float elev = rnd(0.3f, 1.0f);
        d.vel      = Vec3(cosf(ang)*spd*cosf(elev), sinf(elev)*spd, sinf(ang)*spd*cosf(elev));
        d.color    = {color.x * rnd(0.7f,1.f), color.y * rnd(0.7f,1.f), color.z * rnd(0.7f,1.f)};
        d.rot      = rnd(0, 6.28f);
        d.rotSpeed = rnd(-5.f, 5.f);
        d.size     = rnd(0.1f, 0.5f);
        d.life = d.maxLife = rnd(1.f, 3.5f);
        debris.push_back(d);
    }
}

void AnimationSystem::triggerShake(float intensity, float duration) {
    shake.trigger(intensity, duration);
}

void AnimationSystem::update(float dt) {
    shake.update(dt);

    for (auto& sw : shockwaves) sw.update(dt);
    shockwaves.erase(std::remove_if(shockwaves.begin(), shockwaves.end(),
        [](const ShockwaveAnim& s){ return s.done(); }), shockwaves.end());

    for (auto& d : debris) d.update(dt);
    debris.erase(std::remove_if(debris.begin(), debris.end(),
        [](const DebrisAnim& d){ return d.done(); }), debris.end());

    // Cap debris for performance
    if (debris.size() > 600) debris.resize(600);
}
