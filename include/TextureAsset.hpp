#pragma once

#include "IAsset.hpp"
#include "raylib.h"
#include <vector>
#include <cstdint>
#include <string>

// Concrete Asset: Quan ly Texture 2D trong VRAM (Dong vai tro Intrinsic State trong Flyweight)
class TextureAsset : public IAsset {
private:
    std::string filePath;
    Texture2D textureData{};
    size_t memorySize{0};
    bool loaded{false};

    // Bo nho RAM thuc su cap phat de chung minh su chenh lech RAM khi bat/tat Flyweight
    std::vector<uint8_t> realMemoryBuffer;

    // Thong tin nhan dien do hoa fallback
    Color monsterColor{DARKGREEN};
    std::string monsterName{"zombie"};

public:
    TextureAsset();
    ~TextureAsset() override;

    bool load(const std::string& path) override;
    void unload() override;
    void render(float posX, float posY, float scale = 1.0f) override;
    void play() override;
    bool isLoaded() const override;
    size_t getSizeBytes() const override;
    AssetType getType() const override;
    std::string getPath() const override;

    Texture2D getRawTexture() const;
    static std::shared_ptr<TextureAsset> createPlaceholder(int width, int height, Color color);
};
