#include "AssetProxy.hpp"
#include "AudioAsset.hpp"
#include "MeshAsset.hpp"
#include "TextureAsset.hpp"
#include "raylib.h"
#include <iostream>

using namespace std;

AssetProxy::AssetProxy(const string& path, AssetType type, AssetLoadService* service,
                       shared_ptr<IAsset> placeholder, int delayMs)
    : assetPath(path), assetType(type), placeholderAsset(move(placeholder)),
      loadService(service) {
    (void)delayMs; // Retained for source compatibility; real disk I/O replaces simulated delay.
    if (!placeholderAsset) {
        if (assetType == AssetType::TEXTURE_2D) placeholderAsset = TextureAsset::createPlaceholder(48, 48, GRAY);
        else if (assetType == AssetType::MESH_3D) placeholderAsset = MeshAsset::createCubePlaceholder();
        else placeholderAsset = AudioAsset::createSilentPlaceholder();
    }
}

AssetProxy::~AssetProxy() { unload(); }

bool AssetProxy::load(const string& path) {
    assetPath = path;
    // Lazy loading: load() only configures the proxy. render()/play() starts I/O.
    return loadService != nullptr;
}

void AssetProxy::ensureLoadStarted() {
    AssetLoadState expected = AssetLoadState::UNLOADED;
    if (!state.compare_exchange_strong(expected, AssetLoadState::QUEUED)) return;
    if (!loadService) {
        state.store(AssetLoadState::FAILED);
        return;
    }
    loadTicket = loadService->submit(assetPath, assetType);
    state.store(loadTicket->state.load());
}

void AssetProxy::unload() {
    lock_guard<mutex> lock(swapMutex);
    realAsset.reset();
    placeholderAsset.reset();
    loadTicket.reset();
    state.store(AssetLoadState::CANCELLED);
}

void AssetProxy::checkAndSwap() {
    ensureLoadStarted();
    shared_ptr<AssetLoadTicket> ticket;
    {
        lock_guard<mutex> lock(swapMutex);
        ticket = loadTicket;
    }
    if (!ticket) return;
    AssetLoadState ticketState = ticket->state.load();
    state.store(ticketState);
    if (ticketState != AssetLoadState::READY) return;

    lock_guard<mutex> lock(swapMutex);
    if (!realAsset) {
        lock_guard<mutex> ticketLock(ticket->mutex);
        realAsset = ticket->asset;
        if (realAsset) {
            placeholderAsset.reset();
            cout << "[AssetProxy] Main-thread upload ready: " << assetPath << "\n";
        }
    }
}

void AssetProxy::render(float posX, float posY, float scale) {
    checkAndSwap();
    lock_guard<mutex> lock(swapMutex);
    if (realAsset) {
        realAsset->render(posX, posY, scale);
    } else if (placeholderAsset) {
        placeholderAsset->render(posX, posY, scale);
        AssetLoadState current = state.load();
        if (current == AssetLoadState::QUEUED || current == AssetLoadState::LOADING ||
            current == AssetLoadState::CPU_READY) {
            DrawCircle(static_cast<int>(posX + 24 * scale), static_cast<int>(posY - 4), 3, YELLOW);
            DrawText("LOAD...", static_cast<int>(posX + 4), static_cast<int>(posY - 12), 9, YELLOW);
        } else if (current == AssetLoadState::FAILED) {
            DrawText("FAILED", static_cast<int>(posX + 2), static_cast<int>(posY - 12), 9, RED);
        }
    }
}

void AssetProxy::play() {
    checkAndSwap();
    lock_guard<mutex> lock(swapMutex);
    if (realAsset) realAsset->play();
    else if (placeholderAsset) placeholderAsset->play();
}

bool AssetProxy::isLoaded() const { return state.load() == AssetLoadState::READY; }

size_t AssetProxy::getSizeBytes() const { return getRamSizeBytes() + getVramSizeBytes(); }

size_t AssetProxy::getRamSizeBytes() const {
    lock_guard<mutex> lock(swapMutex);
    return realAsset ? realAsset->getRamSizeBytes() : (placeholderAsset ? placeholderAsset->getRamSizeBytes() : sizeof(*this));
}

size_t AssetProxy::getVramSizeBytes() const {
    lock_guard<mutex> lock(swapMutex);
    return realAsset ? realAsset->getVramSizeBytes() : (placeholderAsset ? placeholderAsset->getVramSizeBytes() : 0);
}

AssetType AssetProxy::getType() const { return assetType; }
string AssetProxy::getPath() const { return assetPath; }

bool AssetProxy::isCurrentlyLoading() const {
    AssetLoadState current = state.load();
    return current == AssetLoadState::QUEUED || current == AssetLoadState::LOADING ||
           current == AssetLoadState::CPU_READY;
}
