#include "TextureAsset.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>

using namespace std;

TextureAsset::TextureAsset() = default;

TextureAsset::~TextureAsset() {
    unload();
}

bool TextureAsset::load(const string& path) {
    filePath = path;

    // Phan loai quai vat theo ten duong dan
    if (path.find("zombie") != string::npos) {
        monsterColor = DARKGREEN;
        monsterName = "Zombie";
    } else if (path.find("demon") != string::npos) {
        monsterColor = RED;
        monsterName = "Demon";
    } else if (path.find("dragon") != string::npos) {
        monsterColor = GOLD;
        monsterName = "Dragon";
    } else if (path.find("robot") != string::npos) {
        monsterColor = SKYBLUE;
        monsterName = "Robot";
    } else {
        monsterColor = PURPLE;
        monsterName = "Monster";
    }

    // 1. CAP PHAT BO NHO RAM THUC TE (4 MB moi ban sao Texture)
    // Giup do luong RAM he thong tang/giam ro ret tren Task Manager va Profiler khi bat/tat Flyweight
    size_t allocatedBytes = 4 * 1024 * 1024; // 4 MB
    realMemoryBuffer.resize(allocatedBytes);
    // Cham vao tung trang bo nho (4KB) de he dieu hanh Windows thuc su commit RAM vat ly
    for (size_t i = 0; i < allocatedBytes; i += 4096) {
        realMemoryBuffer[i] = static_cast<uint8_t>(i & 0xFF);
    }
    memorySize = allocatedBytes;

    // 2. Doc file that neu co; neu khong thi tao sprite procedural de demo van chay doc lap.
    int width = 48;
    int height = 48;
    Image img{};
    bool loadedFromDisk = false;
    if (filesystem::exists(path)) {
        img = LoadImage(path.c_str());
        if (img.data) {
            width = img.width;
            height = img.height;
            loadedFromDisk = true;
        }
    }
    if (!img.data) img = GenImageColor(width, height, BLANK);

    if (!loadedFromDisk) {
        // Than quai vat
        ImageDrawRectangle(&img, 4, 8, width - 8, height - 12, monsterColor);
        ImageDrawRectangleLines(&img, Rectangle{4, 8, (float)width - 8, (float)height - 12}, 2, BLACK);

        // Mat va dau
        ImageDrawCircle(&img, width / 2 - 8, 20, 5, WHITE);
    ImageDrawCircle(&img, width / 2 + 8, 20, 5, WHITE);
    ImageDrawCircle(&img, width / 2 - 8, 20, 2, BLACK);
    ImageDrawCircle(&img, width / 2 + 8, 20, 2, BLACK);

        // Mieng / sung / chi tiet
        if (monsterName == "Demon") {
        ImageDrawRectangle(&img, 6, 2, 6, 8, BLACK); // Sung trai
        ImageDrawRectangle(&img, width - 12, 2, 6, 8, BLACK); // Sung phai
        ImageDrawRectangle(&img, width / 2 - 6, 30, 12, 4, BLACK);
    } else if (monsterName == "Zombie") {
        ImageDrawRectangle(&img, width / 2 - 8, 30, 16, 5, MAROON); // Mieng zombie
    } else if (monsterName == "Robot") {
        ImageDrawRectangle(&img, width / 2 - 2, 2, 4, 7, DARKGRAY); // Ang-ten
        ImageDrawCircle(&img, width / 2, 2, 3, RED);
        ImageDrawRectangle(&img, width / 2 - 12, 18, 24, 6, DARKBLUE); // Kinh visor
        } else {
        // Dragon
        ImageDrawRectangle(&img, 1, 16, 5, 14, ORANGE); // Canh trai
        ImageDrawRectangle(&img, width - 6, 16, 5, 14, ORANGE); // Canh phai
        }
    }

    textureData = LoadTextureFromImage(img);
    UnloadImage(img);

    loaded = (textureData.id > 0);
    vramSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4U;
    cout << "[TextureAsset] Loaded: " << monsterName << " (" << memorySize / (1024 * 1024) 
         << " MB RAM, Texture ID: " << textureData.id << ")\n";

    return loaded;
}

bool TextureAsset::loadPixels(const string& path, int width, int height, const vector<uint8_t>& rgbaPixels) {
    if (width <= 0 || height <= 0 || rgbaPixels.size() < static_cast<size_t>(width * height * 4)) return false;
    filePath = path;
    Image image{};
    image.data = const_cast<uint8_t*>(rgbaPixels.data());
    image.width = width;
    image.height = height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    textureData = LoadTextureFromImage(image);
    loaded = textureData.id > 0;
    memorySize = rgbaPixels.size();
    realMemoryBuffer.assign(rgbaPixels.begin(), rgbaPixels.end());
    vramSize = rgbaPixels.size();
    return loaded;
}

void TextureAsset::unload() {
    if (loaded && textureData.id > 0) {
        UnloadTexture(textureData);
        textureData.id = 0;
    }
    // Giai phong RAM thuc
    realMemoryBuffer.clear();
    realMemoryBuffer.shrink_to_fit();
    loaded = false;
    memorySize = 0;
    vramSize = 0;
    cout << "[TextureAsset] Unloaded VRAM & RAM for: " << monsterName << "\n";
}

void TextureAsset::render(float posX, float posY, float scale) {
    if (textureData.id > 0) {
        DrawTextureEx(textureData, Vector2{posX, posY}, 0.0f, scale, WHITE);
    } else {
        // Fallback hien thi neu texture chua san sang
        float size = 48.0f * scale;
        DrawRectangle(static_cast<int>(posX), static_cast<int>(posY), static_cast<int>(size), static_cast<int>(size), monsterColor);
        DrawRectangleLines(static_cast<int>(posX), static_cast<int>(posY), static_cast<int>(size), static_cast<int>(size), WHITE);
    }
}

void TextureAsset::play() {
}

bool TextureAsset::isLoaded() const {
    return loaded;
}

size_t TextureAsset::getSizeBytes() const {
    return memorySize + vramSize;
}

size_t TextureAsset::getRamSizeBytes() const { return memorySize; }
size_t TextureAsset::getVramSizeBytes() const { return vramSize; }

AssetType TextureAsset::getType() const {
    return AssetType::TEXTURE_2D;
}

string TextureAsset::getPath() const {
    return filePath;
}

Texture2D TextureAsset::getRawTexture() const {
    return textureData;
}

shared_ptr<TextureAsset> TextureAsset::createPlaceholder(int width, int height, Color color) {
    auto placeholder = make_shared<TextureAsset>();
    placeholder->filePath = "placeholder_ghost";
    placeholder->monsterName = "Loading...";
    placeholder->monsterColor = Color{color.r, color.g, color.b, 160};

    Image img = GenImageColor(width, height, BLANK);
    // Ve bong ma nhe (Ghost/Placeholder)
    ImageDrawRectangle(&img, 4, 4, width - 8, height - 8, placeholder->monsterColor);
    ImageDrawRectangleLines(&img, Rectangle{4, 4, (float)width - 8, (float)height - 8}, 2, YELLOW);

    placeholder->textureData = LoadTextureFromImage(img);
    UnloadImage(img);

    placeholder->loaded = true;
    placeholder->memorySize = 1024; // Chi khoang 1 KB, khong cap phat 4 MB RAM
    placeholder->vramSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4U;
    return placeholder;
}
