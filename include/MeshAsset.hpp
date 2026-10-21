#pragma once

#include "IAsset.hpp"
#include "raylib.h"
#include <string>
#include <memory>
#include <vector>

using namespace std;

// Concrete Asset: Quan ly Mesh/Model 3D (Dong vai tro Intrinsic State trong Flyweight)
class MeshAsset : public IAsset {
private:
    string filePath;
    Model model3D{};
    size_t memorySize{0};
    size_t vramSize{0};
    bool loaded{false};
    bool isCustomModel{false};

public:
    MeshAsset();
    ~MeshAsset() override;

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
    bool loadTriangles(const string& path, const vector<float>& triangleVertices);

    // Tao placeholder 3D dang khoi lap phuong hoac wireframe nhe
    static shared_ptr<MeshAsset> createCubePlaceholder();
};
