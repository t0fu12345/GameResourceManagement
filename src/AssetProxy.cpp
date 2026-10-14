#include "AssetProxy.hpp"
#include "TextureAsset.hpp"
#include "MeshAsset.hpp"
#include "AudioAsset.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

AssetProxy::AssetProxy(const string& path, AssetType type, shared_ptr<IAsset> placeholder, int delayMs)
    : assetPath(path), assetType(type), placeholderAsset(placeholder), simulatedDelayMs(delayMs) {

    // Neu chua co placeholder, tao placeholder tuong ung voi loai tai nguyen
    if (!placeholderAsset) {
        if (assetType == AssetType::TEXTURE_2D) {
            placeholderAsset = TextureAsset::createPlaceholder(48, 48, GRAY);
        } else if (assetType == AssetType::MESH_3D) {
            placeholderAsset = MeshAsset::createCubePlaceholder();
        } else {
            placeholderAsset = AudioAsset::createSilentPlaceholder();
        }
    }
}

AssetProxy::~AssetProxy() {
    unload();
}

bool AssetProxy::load(const string& path) {
    assetPath = path;

    if (isLoading.load() || isReady.load()) {
        return true;
    }

    isLoading.store(true);

    // Luong ngam (Background Worker Thread) mo phong thoi gian doc du lieu I/O / Mang Cloud
    loadFuture = async(launch::async, [this]() -> bool {
        if (simulatedDelayMs > 0) {
            this_thread::sleep_for(chrono::milliseconds(simulatedDelayMs));
        }
        return true;
    });

    return true;
}

void AssetProxy::unload() {
    lock_guard<mutex> lock(swapMutex);
    if (realAsset) {
        realAsset->unload();
        realAsset.reset();
    }
    if (placeholderAsset) {
        placeholderAsset->unload();
        placeholderAsset.reset();
    }
    isReady.store(false);
    isLoading.store(false);
}

void AssetProxy::checkAndSwap() {
    if (isReady.load()) return;

    if (loadFuture.valid()) {
        auto status = loadFuture.wait_for(chrono::milliseconds(0));
        if (status == future_status::ready) {
            loadFuture.get(); // Hoan tat future

            lock_guard<mutex> lock(swapMutex);
            // THUC HIEN KHOI TAO VA NAP TEXTURE TREN MAIN THREAD (Bat buoc cho OpenGL / Raylib)
            if (assetType == AssetType::TEXTURE_2D) {
                realAsset = make_shared<TextureAsset>();
            } else if (assetType == AssetType::MESH_3D) {
                realAsset = make_shared<MeshAsset>();
            } else {
                realAsset = make_shared<AudioAsset>();
            }

            realAsset->load(assetPath);
            isReady.store(true);
            isLoading.store(false);
            cout << "[AssetProxy] SWAPPED on Main Thread: Placeholder -> Real Asset for [" << assetPath << "]\n";
        }
    }
}

void AssetProxy::render(float posX, float posY, float scale) {
    checkAndSwap();

    lock_guard<mutex> lock(swapMutex);
    if (isReady.load() && realAsset) {
        realAsset->render(posX, posY, scale);
    } else if (placeholderAsset) {
        placeholderAsset->render(posX, posY, scale);

        // Hien thi bieu tuong LOADING truc quan tren dau quai vat
        if (isLoading.load()) {
            DrawCircle(static_cast<int>(posX + 24 * scale), static_cast<int>(posY - 4), 3, YELLOW);
            DrawText("LOAD...", static_cast<int>(posX + 4), static_cast<int>(posY - 12), 9, YELLOW);
        }
    }
}

void AssetProxy::play() {
    checkAndSwap();

    lock_guard<mutex> lock(swapMutex);
    if (isReady.load() && realAsset) {
        realAsset->play();
    } else if (placeholderAsset) {
        placeholderAsset->play();
    }
}

bool AssetProxy::isLoaded() const {
    return isReady.load();
}

size_t AssetProxy::getSizeBytes() const {
    lock_guard<mutex> lock(swapMutex);
    if (isReady.load() && realAsset) {
        return realAsset->getSizeBytes();
    }
    if (placeholderAsset) {
        return placeholderAsset->getSizeBytes();
    }
    return sizeof(*this);
}

AssetType AssetProxy::getType() const {
    return assetType;
}

string AssetProxy::getPath() const {
    return assetPath;
}
