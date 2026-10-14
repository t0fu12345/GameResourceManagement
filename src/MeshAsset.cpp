#include "MeshAsset.hpp"
#include <iostream>
#include <filesystem>

using namespace std;

MeshAsset::MeshAsset() = default;

MeshAsset::~MeshAsset() {
    unload();
}

bool MeshAsset::load(const string& path) {
    filePath = path;

    if (filesystem::exists(path)) {
        model3D = LoadModel(path.c_str());
        loaded = true;
        isCustomModel = true;
        memorySize = 4 * 1024 * 1024; // Uoc tinh 4 MB cho 3D Mesh
        cout << "[MeshAsset] Loaded 3D Model: " << path << "\n";
        return true;
    }

    // Neu khong co file 3D tren dia, tao Mesh 3D hinh hop lap phuong bang Raylib
    Mesh cubeMesh = GenMeshCube(2.0f, 2.0f, 2.0f);
    model3D = LoadModelFromMesh(cubeMesh);
    loaded = true;
    isCustomModel = true;
    memorySize = 3 * 1024 * 1024; // Mo phong 3 MB cho du lieu dinh (vertex buffer)

    cout << "[MeshAsset] Generated 3D Mesh: " << path << " (~3 MB VRAM)\n";
    return true;
}

void MeshAsset::unload() {
    if (loaded && isCustomModel) {
        UnloadModel(model3D);
        loaded = false;
        cout << "[MeshAsset] Unloaded 3D Model: " << filePath << "\n";
    }
}

void MeshAsset::render(float posX, float posY, float scale) {
    if (!loaded) return;

    // Ve bieu dien 3D hinh hop thu nho tren toa do 2D man hinh
    float size = 30.0f * scale;
    DrawRectangle(static_cast<int>(posX), static_cast<int>(posY), static_cast<int>(size), static_cast<int>(size), DARKBLUE);
    DrawRectangleLines(static_cast<int>(posX), static_cast<int>(posY), static_cast<int>(size), static_cast<int>(size), SKYBLUE);
    DrawLine(static_cast<int>(posX), static_cast<int>(posY), static_cast<int>(posX + size), static_cast<int>(posY + size), SKYBLUE);
}

void MeshAsset::play() {
    // Mesh khong co am thanh
}

bool MeshAsset::isLoaded() const {
    return loaded;
}

size_t MeshAsset::getSizeBytes() const {
    return memorySize;
}

AssetType MeshAsset::getType() const {
    return AssetType::MESH_3D;
}

string MeshAsset::getPath() const {
    return filePath;
}

shared_ptr<MeshAsset> MeshAsset::createCubePlaceholder() {
    auto placeholder = make_shared<MeshAsset>();
    placeholder->filePath = "placeholder_3d_cube";
    placeholder->loaded = true;
    placeholder->memorySize = 256; // Chi ton khoang vai bytes
    return placeholder;
}
