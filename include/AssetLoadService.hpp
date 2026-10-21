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
    std::string path;
    bool success{false};
    bool usedFallback{false};
    std::string error;

    int width{0};
    int height{0};
    std::vector<uint8_t> pixels;

    unsigned int sampleRate{0};
    unsigned int channels{0};
    std::vector<int16_t> samples;

    std::vector<float> vertices;
};

struct AssetLoadTicket {
    std::atomic<AssetLoadState> state{AssetLoadState::UNLOADED};
    mutable std::mutex mutex;
    PreparedAssetData prepared;
    std::shared_ptr<IAsset> asset;
    std::string error;
};

class AssetLoadService {
public:
    explicit AssetLoadService(size_t workerCount = 2);
    ~AssetLoadService();

    std::shared_ptr<AssetLoadTicket> submit(const std::string& path, AssetType type);
    size_t pumpMainThreadUploads(size_t maxUploads = 2, double budgetMs = 2.0);
    size_t getLoadingCount() const;
    void shutdown();

private:
    struct WorkItem {
        std::string path;
        AssetType type;
        std::shared_ptr<AssetLoadTicket> ticket;
    };

    mutable std::mutex mutex;
    std::condition_variable condition;
    std::deque<WorkItem> pending;
    std::deque<std::shared_ptr<AssetLoadTicket>> completed;
    std::vector<std::thread> workers;
    bool accepting{true};
    bool stopping{false};
    std::atomic<size_t> activeJobs{0};

    void workerLoop();
    static PreparedAssetData prepare(const std::string& path, AssetType type);
    static std::shared_ptr<IAsset> finalize(const PreparedAssetData& data);
};
