#include "AssetManager.hpp"
#include "TextureAsset.hpp"
#include "MeshAsset.hpp"
#include "AudioAsset.hpp"
#include "AssetProxy.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

AssetManager::AssetManager() {
    cout << "[AssetManager] Singleton Instance Khoi Tao Thanh Cong.\n";
}

AssetManager::~AssetManager() {
    clearAllCache();
    cout << "[AssetManager] Singleton Instance Da Huy.\n";
}

AssetManager& AssetManager::getInstance() {
    // Thread-safe Meyers' Singleton trong Modern C++
    static AssetManager instance;
    return instance;
}

shared_ptr<IAsset> AssetManager::getAsset(const string& path, AssetType type, bool useProxy, bool useFlyweight, int delayMs) {
    lock_guard<mutex> lock(managerMutex);

    // Don dep cac weak_ptr da bi huy truoc do
    cleanDeadReferences();

    // 1. NEU DUNG FLYWEIGHT: Kiem tra Cache Registry truoc
    if (useFlyweight) {
        auto it = cacheRegistry.find(path);
        if (it != cacheRegistry.end()) {
            shared_ptr<IAsset> existing = it->second.lock();
            if (existing) {
                // Asset da ton tai trong Cache: Chia se con tro dung chung
                touchLRU(path);
                return existing;
            }
        }
    }

    // 2. TAO TAI NGUYEN MOI
    shared_ptr<IAsset> newAsset;

    if (useProxy) {
        // Su dung Virtual Proxy de load ngam (tranh giat lag)
        newAsset = make_shared<AssetProxy>(path, type, nullptr, delayMs);
        newAsset->load(path);
    } else {
        // Load dong bo (Blocking Load) tren main thread -> Gay lag khung hinh de quan sat ro ret
        this_thread::sleep_for(chrono::milliseconds(25));
        if (type == AssetType::TEXTURE_2D) {
            newAsset = make_shared<TextureAsset>();
        } else if (type == AssetType::MESH_3D) {
            newAsset = make_shared<MeshAsset>();
        } else {
            newAsset = make_shared<AudioAsset>();
        }
        newAsset->load(path);
    }

    // 3. NEU DUNG FLYWEIGHT: Luu vao Cache Registry duoi dang weak_ptr
    if (useFlyweight && newAsset) {
        cacheRegistry[path] = newAsset;
        touchLRU(path);
        currentCachedMemoryBytes += newAsset->getSizeBytes();

        // Kiem tra nguong Memory Budget va thuc hien LRU neu can
        if (currentCachedMemoryBytes > maxMemoryBudget) {
            evictLRU();
        }
    }

    return newAsset;
}

void AssetManager::touchLRU(const string& path) {
    auto it = lruIterators.find(path);
    if (it != lruIterators.end()) {
        lruAccessList.erase(it->second);
    }
    lruAccessList.push_front(path);
    lruIterators[path] = lruAccessList.begin();
}

void AssetManager::evictLRU(size_t neededBytes) {
    cout << "[AssetManager] [LRU Eviction] Kich hoat don dep Cache (Dung luong hien tai: " 
         << currentCachedMemoryBytes / (1024 * 1024) << " MB, Nguong: " 
         << maxMemoryBudget / (1024 * 1024) << " MB)...\n";

    // Duyet tu cuoi danh sach LRU (cac tai nguyen it duoc dung nhat)
    auto it = lruAccessList.rbegin();
    while (it != lruAccessList.rend() && (currentCachedMemoryBytes > maxMemoryBudget || neededBytes > 0)) {
        string path = *it;
        auto regIt = cacheRegistry.find(path);

        if (regIt != cacheRegistry.end()) {
            // Neu khong con doi tuong nao su dung (use_count == 0 vi la weak_ptr)
            if (regIt->second.expired()) {
                cacheRegistry.erase(regIt);
                lruIterators.erase(path);
                it = list<string>::reverse_iterator(lruAccessList.erase((++it).base()));
                cout << "[AssetManager] [LRU] Evicted expired asset: " << path << "\n";
                continue;
            }
        }
        ++it;
    }
}

void AssetManager::cleanDeadReferences() {
    for (auto it = cacheRegistry.begin(); it != cacheRegistry.end();) {
        if (it->second.expired()) {
            string deadPath = it->first;
            auto lruIt = lruIterators.find(deadPath);
            if (lruIt != lruIterators.end()) {
                lruAccessList.erase(lruIt->second);
                lruIterators.erase(lruIt);
            }
            it = cacheRegistry.erase(it);
            totalCleanedDeadCount++;
            cout << "[AssetManager] [Ref Counting] Cleaned dead weak_ptr: " << deadPath << "\n";
        } else {
            ++it;
        }
    }
}

void AssetManager::clearAllCache() {
    lock_guard<mutex> lock(managerMutex);
    cacheRegistry.clear();
    lruAccessList.clear();
    lruIterators.clear();
    lruHoldCache.clear();
    currentCachedMemoryBytes = 0;
}

void AssetManager::setMemoryBudget(size_t budgetBytes) {
    lock_guard<mutex> lock(managerMutex);
    maxMemoryBudget = budgetBytes;
    if (currentCachedMemoryBytes > maxMemoryBudget) {
        evictLRU();
    }
}

MemoryStats AssetManager::getMemoryStats() {
    lock_guard<mutex> lock(managerMutex);

    MemoryStats stats;
    stats.maxBudgetBytes = maxMemoryBudget;
    stats.deadReferencesCleaned = totalCleanedDeadCount;
    stats.totalCachedEntries = cacheRegistry.size();

    size_t activeCount = 0;
    size_t totalBytes = 0;

    for (const auto& pair : cacheRegistry) {
        auto sp = pair.second.lock();
        if (sp) {
            activeCount++;
            totalBytes += sp->getSizeBytes();
        }
    }

    stats.activeLivingAssets = activeCount;
    stats.totalAssetMemoryBytes = totalBytes;
    return stats;
}
