#include "AssetCache.hpp"
#include <algorithm>
#include <filesystem>

using namespace std;

size_t AssetKeyHash::operator()(const AssetKey& key) const {
    size_t seed = hash<string>{}(key.canonicalPath);
    seed ^= static_cast<size_t>(key.type) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    seed ^= static_cast<size_t>(key.mode) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    return seed;
}

AssetCache::AssetCache(size_t initialBudgetBytes) : budgetBytes(initialBudgetBytes) {}

shared_ptr<IAsset> AssetCache::find(const AssetKey& key) {
    lock_guard<std::mutex> lock(mutex);
    auto it = registry.find(key);
    if (it == registry.end()) {
        ++misses;
        return nullptr;
    }
    auto asset = it->second.lock();
    if (!asset) {
        eraseLocked(key);
        ++misses;
        return nullptr;
    }
    ++hits;
    touchLocked(key);
    return asset;
}

void AssetCache::insert(const AssetKey& key, const shared_ptr<IAsset>& asset) {
    if (!asset) return;
    lock_guard<std::mutex> lock(mutex);
    evictLRULocked(asset->getSizeBytes());
    registry[key] = asset;
    residentAssets[key] = asset;
    touchLocked(key);
}

size_t AssetCache::evictLRU(size_t neededBytes) {
    lock_guard<std::mutex> lock(mutex);
    return evictLRULocked(neededBytes);
}

size_t AssetCache::evictLRULocked(size_t neededBytes) {
    size_t removed = 0;
    while (!lru.empty() && totalBytesLocked() + neededBytes > budgetBytes) {
        bool found = false;
        for (auto it = lru.end(); it != lru.begin();) {
            --it;
            auto residentIt = residentAssets.find(*it);
            if (residentIt != residentAssets.end() && residentIt->second.use_count() == 1) {
                AssetKey key = *it;
                eraseLocked(key);
                ++evictions;
                ++removed;
                found = true;
                break;
            }
        }
        if (!found) break;
    }
    return removed;
}

size_t AssetCache::collectUnused() {
    lock_guard<std::mutex> lock(mutex);
    vector<AssetKey> unused;
    unused.reserve(residentAssets.size());
    for (const auto& pair : residentAssets) {
        if (pair.second.use_count() == 1) unused.push_back(pair.first);
    }
    for (const auto& key : unused) {
        eraseLocked(key);
        ++cleanedEntries;
    }
    return unused.size();
}

void AssetCache::clear() {
    lock_guard<std::mutex> lock(mutex);
    registry.clear();
    residentAssets.clear();
    lru.clear();
    lruPositions.clear();
}

void AssetCache::setBudget(size_t newBudgetBytes) {
    lock_guard<std::mutex> lock(mutex);
    budgetBytes = newBudgetBytes;
    evictLRULocked(0);
}

AssetCacheStats AssetCache::getStats() const {
    lock_guard<std::mutex> lock(mutex);
    AssetCacheStats stats;
    stats.budgetBytes = budgetBytes;
    stats.residentEntries = residentAssets.size();
    stats.hits = hits;
    stats.misses = misses;
    stats.evictions = evictions;
    stats.cleanedEntries = cleanedEntries;
    for (const auto& pair : registry) {
        auto asset = pair.second.lock();
        if (asset) {
            ++stats.liveEntries;
            stats.ramBytes += asset->getRamSizeBytes();
            stats.vramBytes += asset->getVramSizeBytes();
        }
    }
    return stats;
}

void AssetCache::touchLocked(const AssetKey& key) {
    auto pos = lruPositions.find(key);
    if (pos != lruPositions.end()) lru.erase(pos->second);
    lru.push_front(key);
    lruPositions[key] = lru.begin();
}

size_t AssetCache::totalBytesLocked() const {
    size_t total = 0;
    for (const auto& pair : residentAssets) total += pair.second->getSizeBytes();
    return total;
}

void AssetCache::eraseLocked(const AssetKey& key) {
    auto pos = lruPositions.find(key);
    if (pos != lruPositions.end()) {
        lru.erase(pos->second);
        lruPositions.erase(pos);
    }
    residentAssets.erase(key);
    registry.erase(key);
}
