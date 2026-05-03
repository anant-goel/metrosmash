#pragma once
#include "Math.h"
#include "Physics.h"
#include <vector>
#include <string>
#include <functional>

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
    float       cooldown;       // seconds between shots
    std::string fireSound;
    std::string explodeSound;
    bool        unlocked = false;
};

struct ActiveWeapon {
    WeaponType  type;
    Vec3        position;
    Vec3        velocity;
    float       fuseTime;       // time until explosion (or -1 for manual)
    float       timer = 0.f;
    bool        detonated = false;
    bool        armed = true;
    int         id;
};

class WeaponSystem {
public:
    std::vector<WeaponDef>    defs;
    std::vector<ActiveWeapon> active;
    int                       selectedIdx = 0;
    int                       nextId = 0;

    // Ammo per weapon type
    int ammo[(int)WeaponType::COUNT];

    // Callbacks
    std::function<void(Vec3, float, float, const std::string&, const std::string&)> onExplode;

    void init();
    void selectNext();
    void selectPrev();
    void fire(Vec3 pos, Vec3 dir);
    void detonateManual();         // detonate C4 / gravity bombs
    void update(float dt);

    WeaponDef& selected() { return defs[selectedIdx]; }
    const WeaponDef& selected() const { return defs[selectedIdx]; }
    int currentAmmo() const { return ammo[selectedIdx]; }

    void unlockWeapon(WeaponType t);

private:
    void triggerExplosion(const ActiveWeapon& w);
    float cooldownTimer[(int)WeaponType::COUNT] = {};
};
