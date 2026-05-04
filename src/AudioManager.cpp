#include "pch.h"
#include "AudioManager.h"
#include <algorithm>
#include <iostream>

bool AudioManager::init(const std::string& dir) {
    assetDir = dir;

    // Map logical game sound names -> real extracted APK wav filenames
    nameMap = {
        // Explosions
        {"explosion_large",    "clip_460_Tank_Blast.wav"},
        {"explosion_medium",   "clip_494_Explosion_Medium_3.wav"},
        {"explosion_small",    "clip_502_Explosion_Tiny.wav"},
        {"explosion_barrel",   "clip_451_Explosion_Barrel.wav"},
        {"explosion_nuke",     "clip_503_Nuke.wav"},
        {"explosion_emp",      "clip_485_Explosion_EMP.wav"},
        {"explosion_gravity",  "clip_512_Explosion_Gravity_Bomb.wav"},
        {"nuke_fall",          "clip_473_Nuke_fall.wav"},
        // Weapons
        {"c4_place",           "clip_501_C4_Place.wav"},
        {"c4_detonate",        "clip_482_C4.wav"},
        {"bomb_drop",          "clip_457_Bomb_Drop.wav"},
        {"missile_fire",       "clip_481_Missile_Fire.wav"},
        {"missile_fire2",      "clip_469_Missile_Fire_2.wav"},
        {"railgun",            "clip_491_Railgun.wav"},
        // Tank
        {"tank_idle",          "clip_390_Tank_Idle.wav"},
        {"tank_move",          "clip_462_Tank_Move.wav"},
        {"tank_shoot",         "clip_466_Tank_Shoot.wav"},
        {"tank_blast",         "clip_460_Tank_Blast.wav"},
        {"tank_turret",        "clip_510_Tank_Turret.wav"},
        {"tank_machinegun",    "clip_486_Tank_Machinegun_Shoot.wav"},
        {"tank_reload",        "clip_458_Tank_Shell_Loading.wav"},
        // Vehicles
        {"car_engine",         "clip_490_Car Engine Loop.wav"},
        {"car_engine2",        "clip_386_Car Engine Loop_2.wav"},
        {"car_hit",            "clip_253_Car_Hit.wav"},
        {"car_explode",        "clip_243_Car_explode.wav"},
        {"car_horn",           "clip_236_Car_Horn_1.wav"},
        {"tyre_screech",       "clip_389_Tyre_Screech.wav"},
        // Building / destruction
        {"building_break",     "clip_268_Building_Break_Large_1.wav"},
        {"building_break2",    "clip_249_Building_Break_Large_2.wav"},
        {"glass_break",        "clip_266_Glass_Break_Large_1.wav"},
        {"glass_break_small",  "clip_244_Glass_Break_Small_3.wav"},
        {"metal_hit",          "clip_247_Metal_Hit.wav"},
        {"stone_heavy",        "clip_233_Stone_Heavy_1.wav"},
        // Weather / ambient
        {"rain_heavy",         "clip_452_Heavy Rain.wav"},
        {"rain_light",         "clip_461_Light Rain.wav"},
        {"thunder",            "clip_464_Thunder_1.wav"},
        {"fire",               "clip_250_Fire_1.wav"},
        // UI
        {"ui_click",           "clip_260_Click back sound 7.wav"},
        {"vehicle_placed",     "clip_455_Vehicle_Placed.wav"},
        {"place_mine",         "clip_456_Place_Mine.wav"},
    };

    // Pre-load critical sounds
    std::vector<std::string> preload = {
        "explosion_large","explosion_medium","explosion_small",
        "c4_detonate","tank_shoot","building_break","glass_break",
        "car_explode","tyre_screech","ui_click"
    };
    for (auto& name : preload) {
        auto it = nameMap.find(name);
        if (it != nameMap.end()) getBuffer(it->second);
    }
    return true;
}

sf::SoundBuffer* AudioManager::getBuffer(const std::string& filename) {
    auto it = buffers.find(filename);
    if (it != buffers.end()) return &it->second;

    sf::SoundBuffer buf;
    std::string path = assetDir + "/" + filename;
    if (buf.loadFromFile(path)) {
        buffers[filename] = std::move(buf);
        return &buffers[filename];
    }
    std::cerr << "AudioManager: failed to load " << path << "\n";
    return nullptr;
}

void AudioManager::play(const std::string& name, float volume, bool loop) {
    cleanupFinished();

    auto it = nameMap.find(name);
    if (it == nameMap.end()) return;

    sf::SoundBuffer* buf = getBuffer(it->second);
    if (!buf) return;

    // Limit concurrent sounds to avoid overload
    if (activeSounds.size() >= 32) return;

    auto sound = std::make_unique<sf::Sound>(*buf);
    sound->setVolume(volume);
    sound->setLoop(loop);
    sound->play();
    activeSounds.push_back(std::move(sound));
}

void AudioManager::stopAll() {
    for (auto& s : activeSounds) s->stop();
    activeSounds.clear();
}

void AudioManager::setMasterVolume(float v) {
    sf::Listener::setGlobalVolume(v);
}

void AudioManager::cleanupFinished() {
    activeSounds.erase(
        std::remove_if(activeSounds.begin(), activeSounds.end(),
            [](const std::unique_ptr<sf::Sound>& s) {
                return s->getStatus() == sf::Sound::Status::Stopped;
            }),
        activeSounds.end());
}
