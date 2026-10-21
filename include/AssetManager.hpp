#pragma once

#include "AssetCache.hpp"
#include "AssetLoadService.hpp"
#include "AssetTranscoder.hpp"
#include "IAsset.hpp"
#include <atomic>
#include <memory>
#include <string>

struct MemoryStats {
    size_t totalAssetMemoryBytes{0};
    size_t totalAssetRamBytes{0};
    size_t totalAssetVramBytes{0};
    size_t totalCachedEntries{0};
    size_t residentEntries{0};
    size_t activeLivingAssets{0};
    size_t deadReferencesCleaned{0};
    size_t cacheHits{0};
    size_t cacheMisses{0};
    size_t evictionCount{0};
    size_t loadingAssets{0};
    size_t maxBudgetBytes{0};
    bool lruTriggered{false};
};

class AssetManager {
private:
    AssetCache cache;
    AssetLoadService loader;
    AssetTranscoder transcoder;
    std::atomic<bool> stopped{false};

    AssetManager();
    ~AssetManager();
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    static AssetKey makeKey(const std::string& path, AssetType type, LoadMode mode);

public:
    static AssetManager& getInstance();

    std::shared_ptr<IAsset> getAsset(const std::string& path, AssetType type,
                                     bool useProxy = true, bool useFlyweight = true,
                                     int delayMs = 1500);
    void updateAsyncLoads(size_t maxUploads = 2, double budgetMs = 2.0);
    void evictLRU(size_t neededBytes = 0);
    size_t collectUnused();
    void cleanDeadReferences();
    void clearAllCache();
    void shutdown();

    void setMemoryBudget(size_t budgetBytes);
    size_t getMemoryBudget() const;
    MemoryStats getMemoryStats() const;
    AssetTranscoder& getTranscoder() { return transcoder; }
};
