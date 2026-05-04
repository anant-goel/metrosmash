// ── AudioManager.cpp ─────────────────────────────────────────────────────────
// SFML/Audio.hpp pulls in miniaudio.h which under MSVC redefines __STDC__ to 0.
// This causes MSVC's <cmath> to fail (tanhf, tgammaf etc become "not a member").
//
// THE FIX: Include <cmath> and all standard headers FIRST so they are
// fully parsed before miniaudio.h can touch __STDC__.
// Then undef __STDC__ right before SFML/Audio.hpp so miniaudio uses the
// POSIX/C99 path it needs without corrupting the already-parsed cmath.
// ─────────────────────────────────────────────────────────────────────────────

// Step 1 – pull in every standard math/STL header BEFORE miniaudio touches anything
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <algorithm>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <iostream>

// Step 2 – MSVC + miniaudio workaround: suppress the __STDC__ redefinition
//           by defining MA_NO_RUNTIME_LINKING and pretending we're not strict C.
#ifdef _MSC_VER
#  ifndef _CRT_DECLARE_NONSTDC_NAMES
#    define _CRT_DECLARE_NONSTDC_NAMES 0
#  endif
#  pragma warning(push)
#  pragma warning(disable: 4244 4267 4996 4018 4305 4100 4127 4245 4701)
#  ifdef __STDC__
#    undef __STDC__
#    define METRO_RESTORE_STDC
#  endif
#endif

// Step 3 – Now include SFML Audio (which includes miniaudio.h internally)
#include <SFML/Audio.hpp>

// Step 4 – Restore MSVC warning state
#ifdef _MSC_VER
#  pragma warning(pop)
#  ifdef METRO_RESTORE_STDC
#    undef METRO_RESTORE_STDC
#    define __STDC__ 1
#  endif
#endif

// Step 5 – our own header (no SFML includes inside, pure PIMPL)
#include "AudioManager.h"

// ─────────────────────────────────────────────────────────────────────────────

struct AudioManagerImpl {
    std::string assetDir;
    std::map<std::string, std::string>    nameMap;
    std::map<std::string, sf::SoundBuffer> buffers;
    std::vector<std::unique_ptr<sf::Sound>> activeSounds;

    sf::SoundBuffer* getBuffer(const std::string& filename) {
        auto it = buffers.find(filename);
        if (it != buffers.end()) return &it->second;
        sf::SoundBuffer buf;
        std::string fullPath = assetDir + "/" + filename;
        if (buf.loadFromFile(fullPath)) {
            buffers[filename] = std::move(buf);
            return &buffers[filename];
        }
        std::cerr << "AudioManager: failed to load " << fullPath << "\n";
        return nullptr;
    }

    void cleanupFinished() {
        activeSounds.erase(
            std::remove_if(activeSounds.begin(), activeSounds.end(),
                [](const std::unique_ptr<sf::Sound>& s) {
                    return s->getStatus() == sf::Sound::Stopped;
                }),
            activeSounds.end());
    }
};

AudioManager::AudioManager()  : impl(std::make_unique<AudioManagerImpl>()) {}
AudioManager::~AudioManager() = default;

bool AudioManager::init(const std::string& dir) {
    impl->assetDir = dir;
    impl->nameMap = {
        {"explosion_large",   "clip_460_Tank_Blast.wav"},
        {"explosion_medium",  "clip_494_Explosion_Medium_3.wav"},
        {"explosion_small",   "clip_502_Explosion_Tiny.wav"},
        {"explosion_barrel",  "clip_451_Explosion_Barrel.wav"},
        {"explosion_nuke",    "clip_503_Nuke.wav"},
        {"explosion_emp",     "clip_485_Explosion_EMP.wav"},
        {"explosion_gravity", "clip_512_Explosion_Gravity_Bomb.wav"},
        {"nuke_fall",         "clip_473_Nuke_fall.wav"},
        {"c4_place",          "clip_501_C4_Place.wav"},
        {"c4_detonate",       "clip_482_C4.wav"},
        {"bomb_drop",         "clip_457_Bomb_Drop.wav"},
        {"missile_fire",      "clip_481_Missile_Fire.wav"},
        {"missile_fire2",     "clip_469_Missile_Fire_2.wav"},
        {"railgun",           "clip_491_Railgun.wav"},
        {"tank_idle",         "clip_390_Tank_Idle.wav"},
        {"tank_move",         "clip_462_Tank_Move.wav"},
        {"tank_shoot",        "clip_466_Tank_Shoot.wav"},
        {"tank_blast",        "clip_460_Tank_Blast.wav"},
        {"tank_turret",       "clip_510_Tank_Turret.wav"},
        {"tank_machinegun",   "clip_486_Tank_Machinegun_Shoot.wav"},
        {"tank_reload",       "clip_458_Tank_Shell_Loading.wav"},
        {"car_engine",        "clip_490_Car Engine Loop.wav"},
        {"car_engine2",       "clip_386_Car Engine Loop_2.wav"},
        {"car_hit",           "clip_253_Car_Hit.wav"},
        {"car_explode",       "clip_243_Car_explode.wav"},
        {"car_horn",          "clip_236_Car_Horn_1.wav"},
        {"tyre_screech",      "clip_389_Tyre_Screech.wav"},
        {"building_break",    "clip_268_Building_Break_Large_1.wav"},
        {"building_break2",   "clip_249_Building_Break_Large_2.wav"},
        {"glass_break",       "clip_266_Glass_Break_Large_1.wav"},
        {"glass_break_small", "clip_244_Glass_Break_Small_3.wav"},
        {"metal_hit",         "clip_247_Metal_Hit.wav"},
        {"stone_heavy",       "clip_233_Stone_Heavy_1.wav"},
        {"rain_heavy",        "clip_452_Heavy Rain.wav"},
        {"rain_light",        "clip_461_Light Rain.wav"},
        {"thunder",           "clip_464_Thunder_1.wav"},
        {"fire",              "clip_250_Fire_1.wav"},
        {"ui_click",          "clip_260_Click back sound 7.wav"},
        {"vehicle_placed",    "clip_455_Vehicle_Placed.wav"},
        {"place_mine",        "clip_456_Place_Mine.wav"},
        {"tornado",           "clip_459_Tornado.wav"},
        {"meteor_fire",       "clip_463_Meteor_Fire.wav"},
        {"meteor_impact",     "clip_514_Meteor_Impact.wav"},
        {"volcano",           "clip_508_Volcano_Explode.wav"},
        {"earthquake",        "clip_477_Earthquake.wav"},
        {"black_hole",        "clip_500_Black_Hole.wav"},
        {"gravity_bomb",      "clip_495_Gravity_Bomb_Start.wav"},
        {"ufo_beam",          "clip_489_Tractor_Beam.wav"},
        {"ufo_shoot",         "clip_471_UFO_Shoot_Large.wav"},
        {"thunder1",          "clip_464_Thunder_1.wav"},
        {"thunder2",          "clip_505_Thunder_2.wav"},
        {"thunder3",          "clip_496_Thunder_3.wav"},
    };

    // Pre-load the most-used sounds to avoid first-hit stutter
    for (auto& n : {"explosion_large","explosion_medium","c4_detonate",
                    "tank_shoot","building_break","glass_break",
                    "car_explode","ui_click","bomb_drop"}) {
        auto it = impl->nameMap.find(n);
        if (it != impl->nameMap.end()) impl->getBuffer(it->second);
    }
    return true;
}

void AudioManager::play(const std::string& name, float volume, bool loop) {
    impl->cleanupFinished();
    auto it = impl->nameMap.find(name);
    if (it == impl->nameMap.end()) return;
    sf::SoundBuffer* buf = impl->getBuffer(it->second);
    if (!buf || impl->activeSounds.size() >= 32) return;
    auto sound = std::make_unique<sf::Sound>(*buf);
    sound->setVolume(volume);
    sound->setLoop(loop);
    sound->play();
    impl->activeSounds.push_back(std::move(sound));
}

void AudioManager::stopAll() {
    for (auto& s : impl->activeSounds) s->stop();
    impl->activeSounds.clear();
}

void AudioManager::setMasterVolume(float v) {
    sf::Listener::setGlobalVolume(v);
}
