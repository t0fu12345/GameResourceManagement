#include "AssetCache.hpp"
#include "AssetLoadService.hpp"
#include "AssetManager.hpp"
#include "AssetProxy.hpp"
#include "AssetTranscoder.hpp"
#include "raylib.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <thread>

using namespace std;

namespace {
int failures = 0;

#define CHECK(condition) do { if (!(condition)) { \
    cerr << "FAIL " << __FILE__ << ':' << __LINE__ << " - " #condition "\n"; ++failures; \
} } while (false)

class FakeAsset : public IAsset {
public:
    explicit FakeAsset(size_t bytes, bool* destroyedSafely = nullptr, bool* contextAlive = nullptr)
        : bytes(bytes), destroyedSafely(destroyedSafely), contextAlive(contextAlive) {}
    ~FakeAsset() override {
        if (destroyedSafely && contextAlive) *destroyedSafely = *contextAlive;
    }
    bool load(const string&) override { loaded = true; return true; }
    void unload() override { loaded = false; }
    void render(float, float, float) override {}
    void play() override {}
    bool isLoaded() const override { return loaded; }
    size_t getSizeBytes() const override { return bytes; }
    size_t getRamSizeBytes() const override { return bytes; }
    size_t getVramSizeBytes() const override { return 0; }
    AssetType getType() const override { return AssetType::TEXTURE_2D; }
    string getPath() const override { return "fake"; }
private:
    size_t bytes;
    bool loaded{true};
    bool* destroyedSafely;
    bool* contextAlive;
};

AssetKey key(const string& path) {
    return AssetKey{path, AssetType::TEXTURE_2D, LoadMode::SYNCHRONOUS};
}

void testSingletonAndFlyweightLookup() {
    CHECK(&AssetManager::getInstance() == &AssetManager::getInstance());
    AssetCache cache(1024);
    auto asset = make_shared<FakeAsset>(64);
    cache.insert(key("shared"), asset);
    CHECK(cache.find(key("shared")) == asset);
    auto stats = cache.getStats();
    CHECK(stats.hits == 1);
    CHECK(stats.misses == 0);
}

void testLruAndPinnedAssets() {
    AssetCache cache(100);
    auto oldAsset = make_shared<FakeAsset>(60);
    cache.insert(key("old"), oldAsset);
    oldAsset.reset();
    auto newAsset = make_shared<FakeAsset>(60);
    cache.insert(key("new"), newAsset);
    CHECK(cache.find(key("old")) == nullptr);
    CHECK(cache.getStats().evictions == 1);

    AssetCache pinnedCache(100);
    auto pinned = make_shared<FakeAsset>(60);
    pinnedCache.insert(key("pinned"), pinned);
    auto second = make_shared<FakeAsset>(60);
    pinnedCache.insert(key("second"), second);
    CHECK(pinnedCache.getStats().residentEntries == 2);
    pinned.reset();
    second.reset();
    CHECK(pinnedCache.collectUnused() == 2);
    CHECK(pinnedCache.getStats().residentEntries == 0);
}

void testLifecycleOrdering() {
    bool contextAlive = true;
    bool destroyedSafely = false;
    AssetCache cache(1024);
    auto asset = make_shared<FakeAsset>(16, &destroyedSafely, &contextAlive);
    cache.insert(key("lifecycle"), asset);
    asset.reset();
    cache.clear();
    contextAlive = false;
    CHECK(destroyedSafely);
}

void testTranscoder() {
    AssetTranscoder transcoder;
    auto dds = transcoder.transcodeTexture("sample.png", "DDS_DXT5");
    auto webp = transcoder.transcodeTexture("sample.png", "WEBP");
    CHECK(dds.success && fabs(dds.compressionRatio - 75.0f) < 0.01f);
    CHECK(webp.success && fabs(webp.compressionRatio - 82.0f) < 0.01f);
    CHECK(AssetTranscoder::calculateCompressedSize(0, 0.25f) == 0);
    CHECK(AssetTranscoder::calculateCompressedSize(400, 0.25f) == 100);
    CHECK(AssetTranscoder::calculateCompressedSize(400, -1.0f) == 0);
    CHECK(AssetTranscoder::calculateCompressedSize(400, 2.0f) == 400);
}

void testLazyProxyAndWorkerPreparation() {
    AssetLoadService loader(1);
    filesystem::path tempRoot = filesystem::temp_directory_path() / "game_asset_manager_tests";
    filesystem::create_directories(tempRoot);
    filesystem::path pngPath = tempRoot / "disk_texture.png";
    Image image = GenImageColor(4, 4, RED);
    CHECK(ExportImage(image, pngPath.string().c_str()));
    UnloadImage(image);
    auto textureTicket = loader.submit(pngPath.string(), AssetType::TEXTURE_2D);
    auto textureDeadline = chrono::steady_clock::now() + chrono::seconds(2);
    while ((textureTicket->state.load() == AssetLoadState::QUEUED ||
            textureTicket->state.load() == AssetLoadState::LOADING) &&
           chrono::steady_clock::now() < textureDeadline) {
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    CHECK(textureTicket->state.load() == AssetLoadState::CPU_READY);
    {
        lock_guard<mutex> lock(textureTicket->mutex);
        CHECK(!textureTicket->prepared.usedFallback);
        CHECK(textureTicket->prepared.width == 4 && textureTicket->prepared.height == 4);
        CHECK(textureTicket->prepared.pixels.size() == 64);
    }

    filesystem::path wavPath = tempRoot / "disk_audio.wav";
    vector<int16_t> diskSamples(2205, 1000);
    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(diskSamples.size());
    wave.sampleRate = 22050;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = diskSamples.data();
    CHECK(ExportWave(wave, wavPath.string().c_str()));
    auto waveTicket = loader.submit(wavPath.string(), AssetType::AUDIO_CLIP);
    auto waveDeadline = chrono::steady_clock::now() + chrono::seconds(2);
    while ((waveTicket->state.load() == AssetLoadState::QUEUED ||
            waveTicket->state.load() == AssetLoadState::LOADING) &&
           chrono::steady_clock::now() < waveDeadline) {
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    CHECK(waveTicket->state.load() == AssetLoadState::CPU_READY);
    {
        lock_guard<mutex> lock(waveTicket->mutex);
        CHECK(!waveTicket->prepared.usedFallback);
        CHECK(!waveTicket->prepared.samples.empty());
    }

    string objPath = (filesystem::path("assets") / "models/sample_cube.obj").string();
    auto objTicket = loader.submit(objPath, AssetType::MESH_3D);
    auto objDeadline = chrono::steady_clock::now() + chrono::seconds(2);
    while ((objTicket->state.load() == AssetLoadState::QUEUED ||
            objTicket->state.load() == AssetLoadState::LOADING) &&
           chrono::steady_clock::now() < objDeadline) {
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    CHECK(objTicket->state.load() == AssetLoadState::CPU_READY);
    {
        lock_guard<mutex> lock(objTicket->mutex);
        CHECK(!objTicket->prepared.usedFallback);
        CHECK(objTicket->prepared.vertices.size() == 108);
    }

    auto ticket = loader.submit("missing-audio.wav", AssetType::AUDIO_CLIP);
    auto prepareDeadline = chrono::steady_clock::now() + chrono::seconds(2);
    while ((ticket->state.load() == AssetLoadState::QUEUED ||
            ticket->state.load() == AssetLoadState::LOADING) &&
           chrono::steady_clock::now() < prepareDeadline) {
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    CHECK(ticket->state.load() == AssetLoadState::CPU_READY);
    {
        lock_guard<mutex> lock(ticket->mutex);
        CHECK(ticket->prepared.success);
        CHECK(ticket->prepared.usedFallback);
        CHECK(!ticket->prepared.samples.empty());
    }

    auto placeholder = make_shared<FakeAsset>(1);
    AssetProxy proxy("missing-zombie.png", AssetType::TEXTURE_2D, &loader, placeholder, 0);
    CHECK(proxy.load("missing-zombie.png"));
    CHECK(proxy.getLoadState() == AssetLoadState::UNLOADED);
    proxy.play(); // A public use starts the lazy job without needing a graphics context.
    CHECK(proxy.getLoadState() != AssetLoadState::UNLOADED);

    CHECK(proxy.getLoadState() != AssetLoadState::FAILED);
    loader.shutdown();
    filesystem::remove(pngPath);
    filesystem::remove(wavPath);
    filesystem::remove(tempRoot);
}
}

int main() {
    testSingletonAndFlyweightLookup();
    testLruAndPinnedAssets();
    testLifecycleOrdering();
    testTranscoder();
    testLazyProxyAndWorkerPreparation();
    if (failures == 0) cout << "All core tests passed.\n";
    return failures == 0 ? 0 : 1;
}
