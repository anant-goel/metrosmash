#pragma once
#include <string>
#include <vector>
#include <functional>

enum class AchievementID {
    FIRST_BLOOD,        // first explosion
    DEMOLISHER,         // destroy 25% of city
    HALF_GONE,          // destroy 50% of city
    TOTAL_ANNIHILATION, // destroy 100% of city
    NUKE_USER,          // use nuke
    SPEED_DEMON,        // reach max speed in vehicle
    CHAIN_REACTION,     // 5 explosions in 3 seconds
    GLASS_CANNON,       // destroy 20 glass blocks
    ENTER_VEHICLE,      // enter any vehicle
    TANK_COMMANDER,     // drive tank
    CLUSTER_STRIKE,     // use cluster bomb
    EMP_PULSE,          // use EMP
    GRAVITY_MASTER,     // use gravity bomb
    WEAPON_COLLECTOR,   // unlock 5 weapons
    UNSTOPPABLE,        // 10 explosions without rebuilding
    COUNT
};

struct Achievement {
    AchievementID id;
    std::string   title;
    std::string   description;
    bool          unlocked    = false;
    float         showTimer   = 0.f;  // >0 = currently showing popup
    int           progress    = 0;
    int           goal        = 1;    // 1 = binary unlock
};

class AchievementSystem {
public:
    std::vector<Achievement> list;
    std::function<void(const Achievement&)> onUnlock;

    void init();
    void check(AchievementID id, int progressDelta = 1);
    void checkValue(AchievementID id, int value);
    void update(float dt);

    int unlockedCount() const;

private:
    void unlock(Achievement& a);
};
