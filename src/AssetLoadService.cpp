#include "AssetLoadService.hpp"
#include "AudioAsset.hpp"
#include "MeshAsset.hpp"
#include "TextureAsset.hpp"
#include "raylib.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace std;

namespace {
Color colorForPath(const string& path) {
    if (path.find("zombie") != string::npos) return DARKGREEN;
    if (path.find("demon") != string::npos) return RED;
    if (path.find("dragon") != string::npos) return GOLD;
    if (path.find("robot") != string::npos) return SKYBLUE;
    return PURPLE;
}

vector<float> parseObjTriangles(const string& path) {
    ifstream input(path);
    if (!input) return {};
    vector<float> positions;
    vector<float> triangles;
    string line;
    while (getline(input, line)) {
        istringstream stream(line);
        string tag;
        stream >> tag;
        if (tag == "v") {
            float x, y, z;
            if (stream >> x >> y >> z) {
                positions.push_back(x);
                positions.push_back(y);
                positions.push_back(z);
            }
        } else if (tag == "f") {
            vector<int> face;
            string token;
            while (stream >> token) {
                size_t slash = token.find('/');
                int index = 0;
                try {
                    index = stoi(token.substr(0, slash));
                } catch (const exception&) {
                    continue;
                }
                if (index < 0) index = static_cast<int>(positions.size() / 3) + index + 1;
                face.push_back(index - 1);
            }
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                for (int index : {face[0], face[i], face[i + 1]}) {
                    if (index < 0 || static_cast<size_t>(index * 3 + 2) >= positions.size()) continue;
                    triangles.insert(triangles.end(), positions.begin() + index * 3,
                                     positions.begin() + index * 3 + 3);
                }
            }
        }
    }
    return triangles;
}
}

AssetLoadService::AssetLoadService(size_t workerCount) {
    workerCount = max<size_t>(1, workerCount);
    for (size_t i = 0; i < workerCount; ++i) workers.emplace_back(&AssetLoadService::workerLoop, this);
}

AssetLoadService::~AssetLoadService() { shutdown(); }

shared_ptr<AssetLoadTicket> AssetLoadService::submit(const string& path, AssetType type) {
    auto ticket = make_shared<AssetLoadTicket>();
    ticket->state.store(AssetLoadState::QUEUED);
    {
        lock_guard<std::mutex> lock(mutex);
        if (!accepting) {
            ticket->state.store(AssetLoadState::CANCELLED);
            ticket->error = "Asset loader is shutting down";
            return ticket;
        }
        pending.push_back(WorkItem{path, type, ticket});
    }
    condition.notify_one();
    return ticket;
}

void AssetLoadService::workerLoop() {
    while (true) {
        WorkItem item;
        {
            unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [this] { return stopping || !pending.empty(); });
            if (stopping && pending.empty()) return;
            item = move(pending.front());
            pending.pop_front();
        }
        ++activeJobs;
        item.ticket->state.store(AssetLoadState::LOADING);
        PreparedAssetData prepared = prepare(item.path, item.type);
        {
            lock_guard<std::mutex> ticketLock(item.ticket->mutex);
            item.ticket->prepared = move(prepared);
            item.ticket->error = item.ticket->prepared.error;
        }
        item.ticket->state.store(item.ticket->prepared.success ? AssetLoadState::CPU_READY
                                                                : AssetLoadState::FAILED);
        {
            lock_guard<std::mutex> lock(mutex);
            if (item.ticket->state.load() == AssetLoadState::CPU_READY) completed.push_back(item.ticket);
        }
        --activeJobs;
    }
}

PreparedAssetData AssetLoadService::prepare(const string& path, AssetType type) {
    PreparedAssetData data;
    data.type = type;
    data.path = path;

    if (type == AssetType::TEXTURE_2D) {
        Image image{};
        if (filesystem::exists(path)) image = LoadImage(path.c_str());
        if (image.data) {
            ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            data.width = image.width;
            data.height = image.height;
            size_t size = static_cast<size_t>(image.width) * image.height * 4U;
            auto* bytes = static_cast<uint8_t*>(image.data);
            data.pixels.assign(bytes, bytes + size);
            UnloadImage(image);
        } else {
            data.usedFallback = true;
            data.width = 48;
            data.height = 48;
            Color color = colorForPath(path);
            data.pixels.resize(static_cast<size_t>(data.width * data.height * 4), 0);
            for (int y = 4; y < 44; ++y) {
                for (int x = 4; x < 44; ++x) {
                    size_t p = static_cast<size_t>((y * data.width + x) * 4);
                    data.pixels[p] = color.r;
                    data.pixels[p + 1] = color.g;
                    data.pixels[p + 2] = color.b;
                    data.pixels[p + 3] = 255;
                }
            }
        }
        data.success = !data.pixels.empty();
    } else if (type == AssetType::AUDIO_CLIP) {
        Wave wave{};
        if (filesystem::exists(path)) wave = LoadWave(path.c_str());
        if (wave.data) {
            WaveFormat(&wave, 22050, 16, wave.channels == 0 ? 1 : wave.channels);
            data.sampleRate = wave.sampleRate;
            data.channels = wave.channels;
            size_t count = static_cast<size_t>(wave.frameCount) * wave.channels;
            auto* samples = static_cast<int16_t*>(wave.data);
            data.samples.assign(samples, samples + count);
            UnloadWave(wave);
        } else {
            data.usedFallback = true;
            data.sampleRate = 22050;
            data.channels = 1;
            size_t count = data.sampleRate / 10;
            data.samples.resize(count);
            for (size_t i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / data.sampleRate;
                data.samples[i] = static_cast<int16_t>(9000.0f * sinf(2.0f * 3.1415926f * 520.0f * t));
            }
        }
        data.success = !data.samples.empty();
    } else {
        if (filesystem::exists(path) && filesystem::path(path).extension() == ".obj") {
            data.vertices = parseObjTriangles(path);
        }
        data.usedFallback = data.vertices.empty();
        data.success = true; // Main thread creates a cube when OBJ data is unavailable.
    }

    if (!data.success) data.error = "Unable to prepare asset: " + path;
    return data;
}

shared_ptr<IAsset> AssetLoadService::finalize(const PreparedAssetData& data) {
    if (!data.success) return nullptr;
    if (data.type == AssetType::TEXTURE_2D) {
        auto asset = make_shared<TextureAsset>();
        return asset->loadPixels(data.path, data.width, data.height, data.pixels) ? asset : nullptr;
    }
    if (data.type == AssetType::AUDIO_CLIP) {
        auto asset = make_shared<AudioAsset>();
        return asset->loadSamples(data.path, data.sampleRate, data.channels, data.samples) ? asset : nullptr;
    }
    auto asset = make_shared<MeshAsset>();
    bool loaded = data.vertices.empty() ? asset->load(data.path) : asset->loadTriangles(data.path, data.vertices);
    return loaded ? asset : nullptr;
}

size_t AssetLoadService::pumpMainThreadUploads(size_t maxUploads, double budgetMs) {
    size_t uploaded = 0;
    auto start = chrono::steady_clock::now();
    while (uploaded < maxUploads) {
        shared_ptr<AssetLoadTicket> ticket;
        {
            lock_guard<std::mutex> lock(mutex);
            if (completed.empty()) break;
            ticket = completed.front();
            completed.pop_front();
        }
        PreparedAssetData prepared;
        {
            lock_guard<std::mutex> ticketLock(ticket->mutex);
            prepared = move(ticket->prepared);
        }
        auto asset = finalize(prepared);
        {
            lock_guard<std::mutex> ticketLock(ticket->mutex);
            ticket->asset = move(asset);
            if (!ticket->asset) ticket->error = "Main-thread upload failed: " + prepared.path;
        }
        ticket->state.store(ticket->asset ? AssetLoadState::READY : AssetLoadState::FAILED);
        ++uploaded;
        double elapsed = chrono::duration<double, milli>(chrono::steady_clock::now() - start).count();
        if (elapsed >= budgetMs) break;
    }
    return uploaded;
}

size_t AssetLoadService::getLoadingCount() const {
    lock_guard<std::mutex> lock(mutex);
    return pending.size() + completed.size() + activeJobs.load();
}

void AssetLoadService::shutdown() {
    {
        lock_guard<std::mutex> lock(mutex);
        if (!accepting && stopping) return;
        accepting = false;
        stopping = true;
        for (auto& item : pending) {
            item.ticket->state.store(AssetLoadState::CANCELLED);
            item.ticket->error = "Cancelled during shutdown";
        }
        pending.clear();
    }
    condition.notify_all();
    for (auto& worker : workers) if (worker.joinable()) worker.join();
    workers.clear();
    lock_guard<std::mutex> lock(mutex);
    completed.clear();
}
