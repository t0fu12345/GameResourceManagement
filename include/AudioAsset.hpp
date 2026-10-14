#pragma once

#include "IAsset.hpp"
#include "raylib.h"
#include <string>
#include <memory>

using namespace std;

// Concrete Asset: Quan ly Am thanh (Sound Clip)
class AudioAsset : public IAsset {
private:
    string filePath;
    Sound soundData{};
    size_t memorySize{0};
    bool loaded{false};

public:
    AudioAsset();
    ~AudioAsset() override;

    bool load(const string& path) override;
    void unload() override;
    void render(float posX, float posY, float scale = 1.0f) override;
    void play() override;
    bool isLoaded() const override;
    size_t getSizeBytes() const override;
    AssetType getType() const override;
    string getPath() const override;

    // Tao mot am thanh placeholder (im lang hoac tieng bip ngan)
    static shared_ptr<AudioAsset> createSilentPlaceholder();
};
