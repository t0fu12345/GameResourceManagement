#include "AssetManager.hpp"
#include "AssetProxy.hpp"
#include "AudioAsset.hpp"
#include "MeshAsset.hpp"
#include "TextureAsset.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

using namespace std;

AssetManager::AssetManager() : cache(128ULL * 1024ULL * 1024ULL), loader(2) {
    cout << "[AssetManager] Singleton initialized with two loader workers.\n";
}

AssetManager::~AssetManager() { shutdown(); }

AssetManager& AssetManager::getInstance() {
    static AssetManager instance;
    return instance;
}

AssetKey AssetManager::makeKey(const string& path, AssetType type, LoadMode mode) {
    filesystem::path normalized = filesystem::absolute(filesystem::path(path)).lexically_normal();
    return AssetKey{normalized.generic_string(), type, mode};
}

shared_ptr<IAsset> AssetManager::getAsset(const string& path, AssetType type,
                                          bool useProxy, bool useFlyweight, int delayMs) {
    if (stopped.load()) return nullptr;
    LoadMode mode = useProxy ? LoadMode::ASYNCHRONOUS : LoadMode::SYNCHRONOUS;
    AssetKey key = makeKey(path, type, mode);

    if (useFlyweight) {
        auto existing = cache.find(key);
        if (existing) return existing;
    }

    shared_ptr<IAsset> asset;
    if (useProxy) {
        asset = make_shared<AssetProxy>(path, type, &loader, nullptr, delayMs);
        asset->load(path); // Configures only; first render/play starts the worker job.
    } else {
        this_thread::sleep_for(chrono::milliseconds(25));
        if (type == AssetType::TEXTURE_2D) asset = make_shared<TextureAsset>();
        else if (type == AssetType::MESH_3D) asset = make_shared<MeshAsset>();
        else asset = make_shared<AudioAsset>();
        if (!asset->load(path)) asset.reset();
    }

    if (useFlyweight && asset) cache.insert(key, asset);
    return asset;
}

void AssetManager::updateAsyncLoads(size_t maxUploads, double budgetMs) {
    if (!stopped.load()) {
        loader.pumpMainThreadUploads(maxUploads, budgetMs);
        // Proxy size changes after main-thread upload; re-evaluate the live budget.
        cache.evictLRU(0);
    }
}

void AssetManager::evictLRU(size_t neededBytes) { cache.evictLRU(neededBytes); }
size_t AssetManager::collectUnused() { return cache.collectUnused(); }
void AssetManager::cleanDeadReferences() { collectUnused(); }
void AssetManager::clearAllCache() { cache.clear(); }

void AssetManager::shutdown() {
    bool expected = false;
    if (!stopped.compare_exchange_strong(expected, true)) return;
    loader.shutdown();
    cache.clear();
    cout << "[AssetManager] Shutdown completed before Raylib contexts closed.\n";
}

void AssetManager::setMemoryBudget(size_t budgetBytes) { cache.setBudget(budgetBytes); }
size_t AssetManager::getMemoryBudget() const { return cache.getStats().budgetBytes; }

MemoryStats AssetManager::getMemoryStats() const {
    AssetCacheStats cacheStats = cache.getStats();
    MemoryStats stats;
    stats.totalAssetRamBytes = cacheStats.ramBytes;
    stats.totalAssetVramBytes = cacheStats.vramBytes;
    stats.totalAssetMemoryBytes = cacheStats.ramBytes + cacheStats.vramBytes;
    stats.totalCachedEntries = cacheStats.residentEntries;
    stats.residentEntries = cacheStats.residentEntries;
    stats.activeLivingAssets = cacheStats.liveEntries;
    stats.deadReferencesCleaned = cacheStats.cleanedEntries;
    stats.cacheHits = cacheStats.hits;
    stats.cacheMisses = cacheStats.misses;
    stats.evictionCount = cacheStats.evictions;
    stats.loadingAssets = loader.getLoadingCount();
    stats.maxBudgetBytes = cacheStats.budgetBytes;
    stats.lruTriggered = cacheStats.evictions > 0;
    return stats;
}
