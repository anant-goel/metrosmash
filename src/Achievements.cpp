#include "pch.h"
#include "Achievements.h"

void AchievementSystem::init() {
    list = {
        {AchievementID::FIRST_BLOOD,        "First Blood",         "Trigger your first explosion",         false, 0.f, 0, 1},
        {AchievementID::DEMOLISHER,         "Demolisher",          "Destroy 25% of the city",              false, 0.f, 0, 1},
        {AchievementID::HALF_GONE,          "Half Gone",           "Destroy 50% of the city",              false, 0.f, 0, 1},
        {AchievementID::TOTAL_ANNIHILATION, "Total Annihilation",  "Level the entire city (100%)",         false, 0.f, 0, 1},
        {AchievementID::NUKE_USER,          "Nuclear Option",      "Detonate a Mini Nuke",                 false, 0.f, 0, 1},
        {AchievementID::SPEED_DEMON,        "Speed Demon",         "Hit max speed in a vehicle",           false, 0.f, 0, 1},
        {AchievementID::CHAIN_REACTION,     "Chain Reaction",      "5 explosions within 3 seconds",        false, 0.f, 0, 5},
        {AchievementID::GLASS_CANNON,       "Glass Cannon",        "Shatter 20 glass blocks",              false, 0.f, 0, 20},
        {AchievementID::ENTER_VEHICLE,      "Behind the Wheel",    "Enter any vehicle",                    false, 0.f, 0, 1},
        {AchievementID::TANK_COMMANDER,     "Tank Commander",      "Drive the tank",                       false, 0.f, 0, 1},
        {AchievementID::CLUSTER_STRIKE,     "Cluster Strike",      "Deploy a cluster bomb",                false, 0.f, 0, 1},
        {AchievementID::EMP_PULSE,          "EMP Pulse",           "Disable an area with EMP",             false, 0.f, 0, 1},
        {AchievementID::GRAVITY_MASTER,     "Gravity Master",      "Use the gravity bomb",                 false, 0.f, 0, 1},
        {AchievementID::WEAPON_COLLECTOR,   "Arsenal",             "Unlock 5 different weapons",           false, 0.f, 0, 5},
        {AchievementID::UNSTOPPABLE,        "Unstoppable",         "Trigger 10 explosions without rebuild",false, 0.f, 0, 10},
    };
}

void AchievementSystem::check(AchievementID id, int delta) {
    auto& a = list[(int)id];
    if (a.unlocked) return;
    a.progress += delta;
    if (a.progress >= a.goal) unlock(a);
}

void AchievementSystem::checkValue(AchievementID id, int value) {
    auto& a = list[(int)id];
    if (a.unlocked) return;
    a.progress = value;
    if (a.progress >= a.goal) unlock(a);
}

void AchievementSystem::unlock(Achievement& a) {
    a.unlocked   = true;
    a.showTimer  = 4.f;
    if (onUnlock) onUnlock(a);
}

void AchievementSystem::update(float dt) {
    for (auto& a : list) {
        if (a.showTimer > 0.f) a.showTimer -= dt;
    }
}

int AchievementSystem::unlockedCount() const {
    int c = 0;
    for (auto& a : list) if (a.unlocked) c++;
    return c;
}
