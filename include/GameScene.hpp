#pragma once

#include "GameObject.hpp"
#include "AssetManager.hpp"
#include "MemoryProfiler.hpp"
#include <vector>
#include <memory>
#include <string>

// Quan ly man choi, dieu phoi quai vat va hien thi Dashboard do dac
class GameScene {
private:
    std::vector<std::unique_ptr<GameObject>> entities;
    int nextEntityId{1};

    // Cac che do toi uu hoa
    bool useFlyweight{true};
    bool useProxy{true};

    // Thong tin thong bao tren man hinh
    std::string statusMessage{"He thong san sang! Nhan [SPACE] de spawn 50 quai vat."};
    float statusTimer{5.0f};

    // Danh sach cac asset mau
    std::vector<std::string> monsterTypes{"zombie", "demon", "dragon", "robot"};

public:
    GameScene();
    ~GameScene();

    void initialize();
    void update(float dt, float screenWidth, float screenHeight);
    void draw(float screenWidth, float screenHeight);

    // Cac hanh dong tuong tac
    void spawnHorde(int count = 50, float screenWidth = 1280.0f, float screenHeight = 720.0f);
    void clearAllEntities();
    void toggleFlyweight();
    void toggleProxy();
    void triggerLRUEvict();
    void triggerTranscoderDemo();
    void playAttackSound();

    // Getters
    size_t getEntityCount() const { return entities.size(); }
    bool isFlyweightEnabled() const { return useFlyweight; }
    bool isProxyEnabled() const { return useProxy; }
};
