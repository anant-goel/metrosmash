#pragma once
#include "GameMath.h"
#include "Physics.h"
#include <vector>
#include <string>
#include <functional>

// Visual particle spawned by explosions / FX
struct ParticleEffect {
    Vec3  position;
    Vec3  velocity;
    Vec3  color;
    float size    = 0.1f;
    float life    = 1.f;
    float maxLife = 1.f;
    bool  done() const { return life <= 0.f; }
    void  update(float dt) {
        velocity.y -= 9.8f * dt;
        position   += velocity * dt;
        life       -= dt;
    }
};

// ── Vegetation node (tree / bush / lamp) — rendered & animated in Renderer ──
struct VegetationNode {
    enum class Kind { TREE, BUSH, LAMP } kind = Kind::TREE;
    Vec3  position;
    float height    = 4.f;     // trunk height for trees
    float radius    = 1.8f;    // canopy radius
    Vec3  leafColor = {0.15f, 0.55f, 0.12f};
    float swayPhase = 0.f;     // randomised per-node for wind offset
    float swayAmp   = 0.04f;   // max lean angle (radians)
    bool  burning   = false;
    float burnTimer = 0.f;
};

enum class WeaponType {
    C4,
    MISSILE,
    RAILGUN,
    NUKE,
    AIRSTRIKE,
    CLUSTER_BOMB,
    EMP,
    GRAVITY_BOMB,
    WRECKING_BALL,
    // ── New exotic weapons ───────────────────────────────────────────────
    METEOR_SHOWER,   // Rain of 8 flaming rocks from sky
    BLACK_HOLE,      // Pulls everything inward for 4 s then collapses
    WORMHOLE,        // Teleports a chunk of city, spawns exit explosion
    LIGHTNING_STORM, // 12 forked lightning bolts over 3 s
    VOLCANO,         // Eruption cone; lava blobs arc outward
    TSUNAMI,         // Rolling wave that topples everything in a line
    TORNADO,         // Spinning vortex that throws objects
    ACID_RAIN,       // Persistent corrosive drizzle that dissolves blocks
    COUNT
};

struct WeaponDef {
    WeaponType  type;
    std::string name;
    std::string icon;
    float       blastRadius;
    float       blastStrength;
    float       damage;
    int         maxAmmo;
    float       cooldown;
    std::string fireSound;
    std::string explodeSound;
    bool        unlocked = false;
};

struct ActiveWeapon {
    WeaponType  type;
    Vec3        position;
    Vec3        velocity;
    float       fuseTime  = 0.f;
    float       timer     = 0.f;
    float       duration  = 0.f;   // for sustained weapons (black hole, tornado…)
    bool        detonated = false;
    bool        armed     = true;
    int         id        = 0;
    int         subStep   = 0;     // internal counter for multi-pulse weapons
};

class WeaponSystem {
public:
    std::vector<WeaponDef>    defs;
    std::vector<ActiveWeapon> active;
    int                       selectedIdx = 0;
    int                       nextId      = 0;

    int ammo[(int)WeaponType::COUNT];

    std::function<void(Vec3, float, float, const std::string&, const std::string&)> onExplode;

    void init();
    void selectNext();
    void selectPrev();
    void fire(Vec3 pos, Vec3 dir);
    void detonateManual();
    void update(float dt);

    WeaponDef&       selected()       { return defs[selectedIdx]; }
    const WeaponDef& selected() const { return defs[selectedIdx]; }
    int currentAmmo() const           { return ammo[selectedIdx]; }

    void unlockWeapon(WeaponType t);

private:
    void triggerExplosion(const ActiveWeapon& w);
    float cooldownTimer[(int)WeaponType::COUNT] = {};
};
