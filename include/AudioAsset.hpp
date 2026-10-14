#pragma once

#include "IAsset.hpp"
#include "raylib.h"

// Concrete Asset: Quan ly Am thanh (Sound Clip)
class AudioAsset : public IAsset {
private:
    std::string filePath;
    Sound soundData{};
    size_t memorySize{0};
    bool loaded{false};

public:
    AudioAsset();
    ~AudioAsset() override;

    bool load(const std::string& path) override;
    void unload() override;
    void render(float posX, float posY, float scale = 1.0f) override; // Khong hoat dong voi Audio
    void play() override;
    bool isLoaded() const override;
    size_t getSizeBytes() const override;
    AssetType getType() const override;
    std::string getPath() const override;

    // Tao mot am thanh placeholder (im lang hoac tieng bip ngan)
    static std::shared_ptr<AudioAsset> createSilentPlaceholder();
};
