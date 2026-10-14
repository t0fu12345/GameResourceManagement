#pragma once

#include "IAsset.hpp"
#include <memory>
#include <string>
#include <atomic>
#include <future>
#include <mutex>

using namespace std;

// Proxy Pattern: Dai dien cho tai nguyen chua nap xong (Virtual Proxy / Lazy Loading)
class AssetProxy : public IAsset {
private:
    string assetPath;
    AssetType assetType;
    shared_ptr<IAsset> realAsset;
    shared_ptr<IAsset> placeholderAsset;

    atomic<bool> isLoading{false};
    atomic<bool> isReady{false};
    future<bool> loadFuture;
    mutable mutex swapMutex;

    // Do tre mo phong tai tu Disk/Cloud (tinh bang milliseconds)
    int simulatedDelayMs{1200};

public:
    AssetProxy(const string& path, AssetType type, shared_ptr<IAsset> placeholder = nullptr, int delayMs = 1200);
    ~AssetProxy() override;

    bool load(const string& path) override;
    void unload() override;
    void render(float posX, float posY, float scale = 1.0f) override;
    void play() override;
    bool isLoaded() const override;
    size_t getSizeBytes() const override;
    AssetType getType() const override;
    string getPath() const override;

    // Kiem tra tien trinh tai ngam va trao doi (swap) con tro sang tai nguyen that tren main thread
    void checkAndSwap();

    bool isCurrentlyLoading() const { return isLoading.load(); }
    bool isRealAssetReady() const { return isReady.load(); }
};
