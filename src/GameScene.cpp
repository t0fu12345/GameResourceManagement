#include "GameScene.hpp"
#include "raylib.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

using namespace std;

GameScene::GameScene() = default;

GameScene::~GameScene() {
    clearAllEntities();
}

void GameScene::initialize() {
    // Spawn khoi dau 50 quai vat de nguoi dung quan sat ngay lap tuc
    spawnHorde(50);
    statusMessage = "He thong san sang! Nhan [SPACE] hoac [ENTER] de spawn 50 quai vat.";
    statusTimer = 5.0f;
}

void GameScene::spawnHorde(int count, float screenWidth, float screenHeight) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> disX(50.0f, max(100.0f, screenWidth - 550.0f));
    uniform_real_distribution<float> disY(60.0f, max(80.0f, screenHeight - 110.0f));
    uniform_real_distribution<float> disSpeed(-90.0f, 90.0f);
    uniform_real_distribution<float> disScale(0.85f, 1.15f);
    uniform_int_distribution<size_t> disType(0, monsterTypes.size() - 1);

    auto& manager = AssetManager::getInstance();

    for (int i = 0; i < count; ++i) {
        string type = monsterTypes[disType(gen)];
        string texturePath = "assets/textures/" + type + ".png";
        string audioPath = "assets/audio/" + type + "_hit.wav";

        // Yeu cau tai nguyen tu AssetManager theo trang thai hien tai cua Flyweight & Proxy
        auto visualAsset = manager.getAsset(texturePath, AssetType::TEXTURE_2D, useProxy, useFlyweight, 1200);
        auto soundAsset = manager.getAsset(audioPath, AssetType::AUDIO_CLIP, false, useFlyweight, 0);

        float vx = disSpeed(gen);
        float vy = disSpeed(gen);
        if (abs(vx) < 25.0f) vx = 50.0f;
        if (abs(vy) < 25.0f) vy = 50.0f;

        auto obj = make_unique<GameObject>(
            nextEntityId++,
            type + "_" + to_string(nextEntityId),
            type,
            visualAsset,
            soundAsset,
            disX(gen),
            disY(gen),
            vx,
            vy,
            disScale(gen)
        );

        entities.push_back(move(obj));
    }

    stringstream ss;
    ss << "Da spawn them " << count << " quai vat! (Tong so entity: " << entities.size() << ")";
    statusMessage = ss.str();
    statusTimer = 4.0f;
}

void GameScene::clearAllEntities() {
    size_t count = entities.size();
    entities.clear();

    // Kich hoat AssetManager kiem tra don dep ref count
    AssetManager::getInstance().cleanDeadReferences();

    stringstream ss;
    ss << "Da xoa toan bo " << count << " quai vat! Reference Counting da giam ve 0.";
    statusMessage = ss.str();
    statusTimer = 4.0f;
}

void GameScene::toggleFlyweight() {
    useFlyweight = !useFlyweight;

    // Cap nhat lai tai nguyen cho toan bo quai vat hien co de nguoi dung thay su thay doi RAM tuc thi
    auto& manager = AssetManager::getInstance();
    for (auto& entity : entities) {
        string texturePath = "assets/textures/" + entity->getMonsterType() + ".png";
        auto newVisual = manager.getAsset(texturePath, AssetType::TEXTURE_2D, false, useFlyweight, 0);
        entity->setVisualAsset(newVisual);
    }
    if (useFlyweight) {
        manager.cleanDeadReferences();
    }

    stringstream ss;
    if (useFlyweight) {
        ss << "Flyweight: [BAT] - Dung chung bo nho (RAM giam xuong con thap nhat)!";
    } else {
        ss << "Flyweight: [TAT] - Nhan ban " << entities.size() << " ban sao VRAM (RAM tang vot)!";
    }
    statusMessage = ss.str();
    statusTimer = 5.0f;
}

void GameScene::toggleProxy() {
    useProxy = !useProxy;
    stringstream ss;
    if (useProxy) {
        ss << "Proxy: [BAT] - Virtual Proxy / Async Loading (Khong giat lag khung hinh)";
    } else {
        ss << "Proxy: [TAT] - Blocking Sync Load (Chan luong, gay khung man hinh khi spawn)";
    }
    statusMessage = ss.str();
    statusTimer = 5.0f;
}

void GameScene::triggerLRUEvict() {
    AssetManager::getInstance().evictLRU(2 * 1024 * 1024);
    statusMessage = "Da kich hoat thuat toan LRU Eviction de don dep Cache!";
    statusTimer = 4.0f;
}

void GameScene::triggerTranscoderDemo() {
    auto result = AssetManager::getInstance().getTranscoder().transcodeTexture("assets/textures/zombie.png", "DDS_DXT5");
    stringstream ss;
    ss << "Transcoder: " << result.sourceFormat << " -> " << result.targetFormat 
       << " | Tiet kiem " << fixed << setprecision(1) << result.compressionRatio << "% VRAM!";
    statusMessage = ss.str();
    statusTimer = 5.0f;
}

void GameScene::playAttackSound() {
    if (!entities.empty()) {
        entities.front()->playSound();
        statusMessage = "Da phat am thanh hieu ung (AudioAsset::play)!";
        statusTimer = 2.0f;
    }
}

void GameScene::update(float dt, float screenWidth, float screenHeight) {
    if (statusTimer > 0.0f) {
        statusTimer -= dt;
    }

    // Vung chien truong (ben trai thanh Dashboard da mo rong)
    float battlefieldWidth = screenWidth - 515.0f;

    for (auto& entity : entities) {
        entity->update(dt, battlefieldWidth, screenHeight);
    }
}

void GameScene::draw(float screenWidth, float screenHeight) {
    // 1. Ve nen chien truong Game
    ClearBackground(Color{16, 20, 28, 255}); // Dark sci-fi slate

    // Bang Dashboard mo rong 490px de chua co chu to hon 50%
    float panelW = 490.0f;
    float panelX = screenWidth - panelW - 14.0f;
    float panelY = 10.0f;
    float panelH = screenHeight - 20.0f;
    float battlefieldWidth = panelX - 10.0f;

    // Luoi grid trang tri phong cach game engine
    for (int x = 0; x < battlefieldWidth; x += 40) {
        DrawLine(x, 0, x, static_cast<int>(screenHeight), Color{26, 32, 44, 255});
    }
    for (int y = 0; y < screenHeight; y += 40) {
        DrawLine(0, y, static_cast<int>(battlefieldWidth), y, Color{26, 32, 44, 255});
    }

    // 2. Ve cac quai vat (GameObjects)
    for (auto& entity : entities) {
        entity->draw();
    }

    // 3. VE BANG DIEU KHIEN (MEMORY PROFILER DASHBOARD) - SIZE CHU TO HON 50%
    DrawRectangleRec(Rectangle{panelX, panelY, panelW, panelH}, Color{10, 14, 22, 245});
    DrawRectangleLinesEx(Rectangle{panelX, panelY, panelW, panelH}, 2.0f, Color{46, 130, 220, 255});

    // Tieu de Dashboard (Size 32 - rat to, ro rang)
    DrawText("ASSET PROFILER", static_cast<int>(panelX + 22), static_cast<int>(panelY + 16), 32, Color{100, 200, 255, 255});
    DrawText("Game Engine Memory Monitor", static_cast<int>(panelX + 22), static_cast<int>(panelY + 54), 16, LIGHTGRAY);
    DrawLine(static_cast<int>(panelX + 22), static_cast<int>(panelY + 76), static_cast<int>(panelX + panelW - 22), static_cast<int>(panelY + 76), Color{60, 90, 130, 255});

    // Lay thong so tu MemoryProfiler va AssetManager
    auto memInfo = MemoryProfiler::queryProcessMemory();
    memInfo.currentFPS = GetFPS();
    memInfo.frameTimeMs = GetFrameTime() * 1000.0f;
    auto memStats = AssetManager::getInstance().getMemoryStats();

    int startY = static_cast<int>(panelY + 90);
    int stepY = 32;
    int labelSize = 21;  // To hon 50% (truoc 13-15)
    int valSize = 22;    // To hon 50% (truoc 14-16)
    int colValX = static_cast<int>(panelX + 235);

    // Chi so FPS
    Color fpsColor = (memInfo.currentFPS >= 50) ? GREEN : (memInfo.currentFPS >= 30 ? YELLOW : RED);
    DrawText("Performance:", static_cast<int>(panelX + 22), startY, labelSize, GRAY);
    DrawText(TextFormat("%d FPS (%.1f ms)", memInfo.currentFPS, memInfo.frameTimeMs), colValX, startY, valSize, fpsColor);

    // RAM vat ly thuc te (Working Set OS)
    startY += stepY;
    DrawText("Process RAM (OS):", static_cast<int>(panelX + 22), startY, labelSize, GRAY);
    Color ramColor = (memInfo.workingSetBytes > 150ULL * 1024ULL * 1024ULL) ? RED : Color{255, 185, 60, 255};
    DrawText(MemoryProfiler::formatBytes(memInfo.workingSetBytes).c_str(), colValX, startY, valSize, ramColor);

    // RAM dinh diem (Peak)
    startY += stepY;
    DrawText("Peak RAM:", static_cast<int>(panelX + 22), startY, labelSize, GRAY);
    DrawText(MemoryProfiler::formatBytes(memInfo.peakWorkingSetBytes).c_str(), colValX, startY, valSize, WHITE);

    // So luong Entity tren man hinh
    startY += stepY;
    DrawText("Entities (Quai):", static_cast<int>(panelX + 22), startY, labelSize, GRAY);
    DrawText(TextFormat("%d objects", static_cast<int>(entities.size())), colValX, startY, valSize, YELLOW);

    // Asset Cache Registry
    startY += stepY;
    DrawText("Cache Registry:", static_cast<int>(panelX + 22), startY, labelSize, GRAY);
    DrawText(TextFormat("%d (Live: %d)", static_cast<int>(memStats.totalCachedEntries), static_cast<int>(memStats.activeLivingAssets)), 
             colValX, startY, valSize, GREEN);

    DrawLine(static_cast<int>(panelX + 22), startY + 28, static_cast<int>(panelX + panelW - 22), startY + 28, Color{60, 90, 130, 255});
    startY += 40;

    // TRANG THAI CAC DESIGN PATTERNS (Size 24)
    DrawText("DESIGN PATTERNS", static_cast<int>(panelX + 22), startY, 24, Color{100, 200, 255, 255});
    startY += stepY;

    // Flyweight status
    DrawText("1. Flyweight:", static_cast<int>(panelX + 22), startY, labelSize, GRAY);
    if (useFlyweight) {
        DrawText("[ON] Shared RAM", colValX, startY, valSize, GREEN);
    } else {
        DrawText("[OFF] Duplicate!", colValX, startY, valSize, RED);
    }

    // Proxy status
    startY += stepY;
    DrawText("2. Proxy:", static_cast<int>(panelX + 22), startY, labelSize, GRAY);
    if (useProxy) {
        DrawText("[ON] Async/Lazy", colValX, startY, valSize, GREEN);
    } else {
        DrawText("[OFF] Blocking", colValX, startY, valSize, RED);
    }

    // Singleton status
    startY += stepY;
    DrawText("3. Singleton:", static_cast<int>(panelX + 22), startY, labelSize, GRAY);
    DrawText("[ACTIVE] Registry", colValX, startY, valSize, GREEN);

    DrawLine(static_cast<int>(panelX + 22), startY + 28, static_cast<int>(panelX + panelW - 22), startY + 28, Color{60, 90, 130, 255});
    startY += 40;

    // HUONG DAN PHIM BAM (Size 24)
    DrawText("KEYBOARD CONTROLS", static_cast<int>(panelX + 22), startY, 24, Color{100, 200, 255, 255});
    stepY = 28;
    int keySize = 19; // To hon 50% (truoc 12-14)
    Color keyColor = Color{235, 235, 235, 255};

    startY += stepY;
    DrawText("[SPACE/ENTER] : +50 quai vat", static_cast<int>(panelX + 22), startY, keySize, keyColor);
    startY += stepY;
    DrawText("[C]           : Xoa het (Ref Count->0)", static_cast<int>(panelX + 22), startY, keySize, keyColor);
    startY += stepY;
    DrawText("[1]           : Bat / Tat Flyweight", static_cast<int>(panelX + 22), startY, keySize, keyColor);
    startY += stepY;
    DrawText("[2]           : Bat / Tat Proxy Async", static_cast<int>(panelX + 22), startY, keySize, keyColor);
    startY += stepY;
    DrawText("[3]           : Kich hoat LRU Evict", static_cast<int>(panelX + 22), startY, keySize, keyColor);
    startY += stepY;
    DrawText("[4]           : Transcoder Nen VRAM", static_cast<int>(panelX + 22), startY, keySize, keyColor);
    startY += stepY;
    DrawText("[P]           : Phat Am Thanh SFX", static_cast<int>(panelX + 22), startY, keySize, keyColor);
    startY += stepY;
    DrawText("[F11] / Nut [ ]: Toan man hinh", static_cast<int>(panelX + 22), startY, keySize, Color{120, 220, 255, 255});

    // 4. THANH THONG BAO TRANG THAI DUOI DAY MAN HINH (STATUS BAR)
    float statusH = 46.0f;
    DrawRectangle(0, static_cast<int>(screenHeight - statusH), static_cast<int>(battlefieldWidth), static_cast<int>(statusH), Color{12, 16, 24, 240});
    DrawRectangleLines(0, static_cast<int>(screenHeight - statusH), static_cast<int>(battlefieldWidth), static_cast<int>(statusH), Color{40, 60, 90, 255});
    DrawText(statusMessage.c_str(), 18, static_cast<int>(screenHeight - statusH + 13), 20, Color{100, 230, 160, 255});
}
