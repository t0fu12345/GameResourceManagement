#pragma once

#include "IAsset.hpp"
#include "raylib.h"
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

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
    size_t getRamSizeBytes() const override;
    size_t getVramSizeBytes() const override;
    AssetType getType() const override;
    string getPath() const override;

    bool loadSamples(const string& path, unsigned int sampleRate, unsigned int channels,
                     const vector<int16_t>& samples);

    // Tao mot am thanh placeholder (im lang hoac tieng bip ngan)
    static shared_ptr<AudioAsset> createSilentPlaceholder();
};
