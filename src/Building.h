#pragma once
#include "GameMath.h"
#include "Physics.h"
#include <vector>
#include <array>

// A single destructible block within a building
struct Block {
    Vec3        position;
    Vec3        color;
    float       health     = 100.f;
    bool        destroyed  = false;
    bool        loose      = false;   // detached, falls with physics
    RigidBody   body;
    int         type       = 0;       // 0=concrete, 1=glass, 2=metal, 3=wood
};

class Building {
public:
    Vec3 origin;              // world-space base center
    int  width, height, depth;// block dimensions
    bool fullyDestroyed = false;

    std::vector<Block> blocks;

    // Build a rectangular building at origin with given block dims
    void build(Vec3 pos, int w, int h, int d, int style = 0);

    // Apply damage at world position within radius
    void applyDamage(Vec3 worldPos, float radius, float damage);

    // Update loose blocks (gravity, settling)
    void update(float dt);

    // Check structural integrity — detach unsupported blocks
    void checkSupport();

    // Collect all active physics bodies
    void collectBodies(std::vector<RigidBody*>& out);

    // Returns 0..1 destruction ratio
    float destructionRatio() const;

    int totalBlocks() const { return (int)blocks.size(); }

private:
    int  idx(int x, int y, int z) const { return x + width*(y + height*z); }
    bool isSupported(int x, int y, int z) const;

    static Vec3 styleColor(int style, int x, int y, int z);
};
