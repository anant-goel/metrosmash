#pragma once
#include "Math.h"
#include <vector>
#include <string>
#include <functional>

// A single keyframe for any float property
struct Keyframe {
    float time;
    float value;
};

struct AnimTrack {
    std::string         name;
    std::vector<Keyframe> keys;
    float               duration;
    bool                loop = false;
    float               currentTime = 0.f;
    bool                playing = false;

    void play(bool fromStart = true) {
        if (fromStart) currentTime = 0.f;
        playing = true;
    }
    void stop() { playing = false; }

    float evaluate() const {
        if (keys.empty()) return 0.f;
        if (keys.size() == 1) return keys[0].value;
        for (int i = 0; i < (int)keys.size()-1; i++) {
            if (currentTime >= keys[i].time && currentTime <= keys[i+1].time) {
                float t = (currentTime - keys[i].time) / (keys[i+1].time - keys[i].time);
                // Smooth step
                t = t * t * (3.f - 2.f * t);
                return keys[i].value + (keys[i+1].value - keys[i].value) * t;
            }
        }
        return keys.back().value;
    }

    void update(float dt) {
        if (!playing) return;
        currentTime += dt;
        if (currentTime >= duration) {
            if (loop) currentTime = fmodf(currentTime, duration);
            else { currentTime = duration; playing = false; }
        }
    }
};

// Shockwave ring animation spawned on explosion
struct ShockwaveAnim {
    Vec3  origin;
    float radius    = 0.f;
    float maxRadius;
    float life;
    float maxLife;
    Vec3  color;

    bool done() const { return life <= 0; }
    void update(float dt) {
        life -= dt;
        float t = 1.f - (life / maxLife);
        radius = maxRadius * t;
    }
};

// Screen shake
struct ScreenShake {
    float intensity = 0.f;
    float duration  = 0.f;
    float timer     = 0.f;

    void trigger(float i, float d) {
        intensity = std::max(intensity, i);
        duration  = std::max(duration, d);
        timer     = duration;
    }

    Vec3 getOffset() const {
        if (timer <= 0) return {};
        float t = intensity * (timer / duration);
        return Vec3(
            ((float)rand()/RAND_MAX - 0.5f) * t,
            ((float)rand()/RAND_MAX - 0.5f) * t * 0.5f,
            0.f);
    }

    void update(float dt) { if (timer > 0) timer -= dt; }
};

// Debris chunk animation (flying piece after destruction)
struct DebrisAnim {
    Vec3  pos, vel;
    Vec3  color;
    float rot, rotSpeed;
    float size;
    float life, maxLife;
    bool  done() const { return life <= 0; }
    void update(float dt) {
        vel.y -= 15.f * dt;
        pos   += vel * dt;
        rot   += rotSpeed * dt;
        life  -= dt;
        if (pos.y < 0) { pos.y = 0; vel.y = -vel.y * 0.3f; vel.x *= 0.6f; vel.z *= 0.6f; }
    }
};

class AnimationSystem {
public:
    std::vector<ShockwaveAnim> shockwaves;
    std::vector<DebrisAnim>    debris;
    ScreenShake                shake;

    void spawnExplosionFX(Vec3 origin, float radius, Vec3 color);
    void spawnDebris(Vec3 origin, Vec3 color, int count);
    void triggerShake(float intensity, float duration);
    void update(float dt);
};
