#pragma once
// NO SFML includes here — SFML/Audio.hpp pulls in miniaudio.h which
// redefines __STDC__ and breaks <cmath> in every file that includes this header.
// All SFML Audio types are kept inside AudioManagerImpl (PIMPL) in the .cpp only.
#include <string>
#include <memory>

struct AudioManagerImpl; // defined in AudioManager.cpp only

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool init(const std::string& assetDir);
    void play(const std::string& name, float volume = 100.f, bool loop = false);
    void stopAll();
    void setMasterVolume(float v);

private:
    std::unique_ptr<AudioManagerImpl> impl;
};
