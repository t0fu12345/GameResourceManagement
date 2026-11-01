#pragma once

#include "IAsset.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std;

enum class AssetLoadState {
    UNLOADED,
    QUEUED,
    LOADING,
    CPU_READY,
    READY,
    FAILED,
    CANCELLED
};

struct PreparedAssetData {
    AssetType type{AssetType::TEXTURE_2D};
    string path;
    bool success{false};
    bool usedFallback{false};
    string error;

    int width{0};
    int height{0};
    vector<uint8_t> pixels;

    unsigned int sampleRate{0};
    unsigned int channels{0};
    vector<int16_t> samples;

    vector<float> vertices;
};

struct AssetLoadTicket {
    atomic<AssetLoadState> state{AssetLoadState::UNLOADED};
    mutable mutex mutex_;
    PreparedAssetData prepared;
    shared_ptr<IAsset> asset;
    string error;
};

class AssetLoadService {
public:
    explicit AssetLoadService(size_t workerCount = 2);
    ~AssetLoadService();

    shared_ptr<AssetLoadTicket> submit(const string& path, AssetType type);
    size_t pumpMainThreadUploads(size_t maxUploads = 2, double budgetMs = 2.0);
    size_t getLoadingCount() const;
    void shutdown();

private:
    struct WorkItem {
        string path;
        AssetType type;
        shared_ptr<AssetLoadTicket> ticket;
    };

    mutable mutex mutex_;
    condition_variable condition;
    deque<WorkItem> pending;
    deque<shared_ptr<AssetLoadTicket>> completed;
    vector<thread> workers;
    bool accepting{true};
    bool stopping{false};
    atomic<size_t> activeJobs{0};

    void workerLoop();
    static PreparedAssetData prepare(const string& path, AssetType type);
    static shared_ptr<IAsset> finalize(const PreparedAssetData& data);
};
