#pragma once

#include <string>
#include <memory>
#include <cstddef>

using namespace std;

enum class AssetType {
    TEXTURE_2D,
    MESH_3D,
    AUDIO_CLIP
};

// Interface co so cho moi tai nguyen trong game (Game Asset)
class IAsset {
public:
    virtual ~IAsset() = default;

    // Nap tai nguyen vao bo nho (RAM / VRAM)
    virtual bool load(const string& path) = 0;

    // Giai phong tai nguyen khoi bo nho
    virtual void unload() = 0;

    // Ve hoac hien thi tai vi tri (x, y) voi ti le scale
    virtual void render(float posX, float posY, float scale = 1.0f) = 0;

    // Phat am thanh neu la AudioAsset
    virtual void play() = 0;

    // Kiem tra xem tai nguyen da duoc nap thanh cong chua
    virtual bool isLoaded() const = 0;

    // Dung luong bo nho chiem dung (tinh bang bytes)
    virtual size_t getSizeBytes() const = 0;

    // Loai tai nguyen
    virtual AssetType getType() const = 0;

    // Duong dan file goc
    virtual string getPath() const = 0;
};
