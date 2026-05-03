#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>

// Maps logical sound names to the real extracted APK filenames
class AudioManager {
public:
    bool init(const std::string& assetDir);
    void play(const std::string& name, float volume = 100.f, bool loop = false);
    void stopAll();
    void setMasterVolume(float v);

private:
    std::string assetDir;
    std::map<std::string, std::string> nameMap; // logical -> filename
    std::map<std::string, sf::SoundBuffer> buffers;
    std::vector<std::unique_ptr<sf::Sound>> activeSounds;

    sf::SoundBuffer* getBuffer(const std::string& filename);
    void cleanupFinished();
};
