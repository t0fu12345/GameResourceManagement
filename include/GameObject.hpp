#pragma once

#include "IAsset.hpp"
#include <memory>
#include <string>

using namespace std;

// Flyweight Context: Chua Extrinsic State nhe (toa do, goc xoay, van toc)
// va con tro shared_ptr toi Intrinsic State nang (IAsset)
class GameObject {
private:
    int id{0};
    string name;
    string monsterType;

    // Extrinsic State: Bien doi lien tuc theo tung doi tuong
    float posX{0.0f};
    float posY{0.0f};
    float posZ{0.0f};
    float speedX{0.0f};
    float speedY{0.0f};
    float rotation{0.0f};
    float scale{1.0f};
    bool active{true};

    // Intrinsic State Reference: Con tro dung chung giua hang ngan doi tuong
    shared_ptr<IAsset> visualAsset;
    shared_ptr<IAsset> soundAsset;

public:
    GameObject(int id, const string& name, const string& type,
               shared_ptr<IAsset> visual,
               shared_ptr<IAsset> sound,
               float x, float y, float vx, float vy, float s = 1.0f);
    ~GameObject() = default;

    void update(float dt, float screenWidth, float screenHeight);
    void draw();
    void playSound();

    // Getters & Setters
    int getId() const { return id; }
    string getName() const { return name; }
    string getMonsterType() const { return monsterType; }
    bool isActive() const { return active; }
    void setActive(bool state) { active = state; }
    float getX() const { return posX; }
    float getY() const { return posY; }
    size_t getExtrinsicSizeBytes() const { return sizeof(*this); }
    shared_ptr<IAsset> getAsset() const { return visualAsset; }
    void setVisualAsset(shared_ptr<IAsset> asset) { visualAsset = asset; }
};
