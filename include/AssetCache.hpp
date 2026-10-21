#pragma once

#include "IAsset.hpp"
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct AssetKey {
    std::string canonicalPath;
    AssetType type{AssetType::TEXTURE_2D};
    LoadMode mode{LoadMode::SYNCHRONOUS};

    bool operator==(const AssetKey& other) const {
        return canonicalPath == other.canonicalPath && type == other.type && mode == other.mode;
    }
};

struct AssetKeyHash {
    size_t operator()(const AssetKey& key) const;
};

struct AssetCacheStats {
    size_t ramBytes{0};
    size_t vramBytes{0};
    size_t residentEntries{0};
    size_t liveEntries{0};
    size_t hits{0};
    size_t misses{0};
    size_t evictions{0};
    size_t cleanedEntries{0};
    size_t budgetBytes{0};
};

// Pure C++ cache policy. The weak registry observes reference-counted assets while
// residentAssets gives LRU a real ownership stake that can be evicted.
class AssetCache {
public:
    explicit AssetCache(size_t budgetBytes = 128ULL * 1024ULL * 1024ULL);

    std::shared_ptr<IAsset> find(const AssetKey& key);
    void insert(const AssetKey& key, const std::shared_ptr<IAsset>& asset);
    size_t evictLRU(size_t neededBytes = 0);
    size_t collectUnused();
    void clear();
    void setBudget(size_t budgetBytes);
    AssetCacheStats getStats() const;

private:
    using LruList = std::list<AssetKey>;

    mutable std::mutex mutex;
    std::unordered_map<AssetKey, std::weak_ptr<IAsset>, AssetKeyHash> registry;
    std::unordered_map<AssetKey, std::shared_ptr<IAsset>, AssetKeyHash> residentAssets;
    LruList lru;
    std::unordered_map<AssetKey, LruList::iterator, AssetKeyHash> lruPositions;
    size_t budgetBytes;
    size_t hits{0};
    size_t misses{0};
    size_t evictions{0};
    size_t cleanedEntries{0};

    void touchLocked(const AssetKey& key);
    size_t totalBytesLocked() const;
    size_t evictLRULocked(size_t neededBytes);
    void eraseLocked(const AssetKey& key);
};
