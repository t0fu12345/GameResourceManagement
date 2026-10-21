#pragma once

#include "IAsset.hpp"
#include "raylib.h"
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

using namespace std;

// Concrete Asset: Quan ly Texture 2D trong VRAM (Dong vai tro Intrinsic State trong Flyweight)
class TextureAsset : public IAsset {
private:
    string filePath;
    Texture2D textureData{};
    size_t memorySize{0};
    size_t vramSize{0};
    bool loaded{false};

    // Bo nho RAM thuc su cap phat de chung minh su chenh lech RAM khi bat/tat Flyweight
    vector<uint8_t> realMemoryBuffer;

    // Thong tin nhan dien do hoa fallback
    Color monsterColor{DARKGREEN};
    string monsterName{"zombie"};

public:
    TextureAsset();
    ~TextureAsset() override;

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

    Texture2D getRawTexture() const;
    bool loadPixels(const string& path, int width, int height, const vector<uint8_t>& rgbaPixels);
    static shared_ptr<TextureAsset> createPlaceholder(int width, int height, Color color);
};
