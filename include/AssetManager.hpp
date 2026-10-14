#pragma once

#include "IAsset.hpp"
#include "AssetTranscoder.hpp"
#include <string>
#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>

using namespace std;

struct MemoryStats {
    size_t totalAssetMemoryBytes{0};
    size_t totalCachedEntries{0};
    size_t activeLivingAssets{0};
    size_t deadReferencesCleaned{0};
    size_t maxBudgetBytes{0};
    bool lruTriggered{false};
};

// Singleton Pattern: Bo quan ly trung tam dieu phoi moi tai nguyen game
class AssetManager {
private:
    // Cache Registry su dung weak_ptr de ho tro giai phong bo nho tu dong (Reference Counting)
    unordered_map<string, weak_ptr<IAsset>> cacheRegistry;

    // Danh sach LRU theo doi thu tu su dung tai nguyen
    list<string> lruAccessList;
    unordered_map<string, list<string>::iterator> lruIterators;

    // Danh sach giu tham chieu chu dong cho LRU Cache de khong bi giai phong ngay lap tuc neu chua vuot budget
    unordered_map<string, shared_ptr<IAsset>> lruHoldCache;

    size_t maxMemoryBudget{128 * 1024 * 1024}; // Mac dinh 128 MB
    size_t currentCachedMemoryBytes{0};
    size_t totalCleanedDeadCount{0};

    mutable mutex managerMutex;
    AssetTranscoder transcoder;

    // Private Constructor (Singleton Pattern)
    AssetManager();
    ~AssetManager();

    // Khong cho phep Copy hoac Assign
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

public:
    static AssetManager& getInstance();

    // Lay tai nguyen (Neu useFlyweight=true: dung chung; neu false: cap phat ban sao rieng)
    // Neu useProxy=true: tra ve AssetProxy de load ngam (anti-stuttering)
    shared_ptr<IAsset> getAsset(const string& path, AssetType type, bool useProxy = true, bool useFlyweight = true, int delayMs = 1500);

    // Kich hoat giai phong bo nho theo thuat toan LRU neu vuot nguong budget
    void evictLRU(size_t neededBytes = 0);

    // Don dep cac weak_ptr da het han (ref count == 0)
    void cleanDeadReferences();

    // Xoa sach toan bo Cache
    void clearAllCache();

    // Cap nhat thu tu truy cap trong LRU list
    void touchLRU(const string& path);

    // Cau hinh nguong Memory Budget
    void setMemoryBudget(size_t budgetBytes);
    size_t getMemoryBudget() const { return maxMemoryBudget; }

    // Lay thong ke bo nho hien tai
    MemoryStats getMemoryStats();

    // Truy cap bo chuyen ma
    AssetTranscoder& getTranscoder() { return transcoder; }
};
