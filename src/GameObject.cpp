#include "GameObject.hpp"
#include "raylib.h"
#include <iostream>

using namespace std;

GameObject::GameObject(int id, const string& name, const string& type,
                       shared_ptr<IAsset> visual,
                       shared_ptr<IAsset> sound,
                       float x, float y, float vx, float vy, float s)
    : id(id), name(name), monsterType(type), visualAsset(visual), soundAsset(sound),
      posX(x), posY(y), speedX(vx), speedY(vy), scale(s) {
}

void GameObject::update(float dt, float screenWidth, float screenHeight) {
    if (!active) return;

    posX += speedX * dt;
    posY += speedY * dt;

    // Va cham voi bien man hinh thi bat lai
    float margin = 48.0f * scale;
    if (posX < 0) {
        posX = 0;
        speedX = -speedX;
    } else if (posX > screenWidth - margin) {
        posX = screenWidth - margin;
        speedX = -speedX;
    }

    if (posY < 50) {
        posY = 50;
        speedY = -speedY;
    } else if (posY > screenHeight - margin - 40) {
        posY = screenHeight - margin - 40;
        speedY = -speedY;
    }

    rotation += 30.0f * dt;
    if (rotation > 360.0f) rotation -= 360.0f;
}

void GameObject::draw() {
    if (!active) return;

    if (visualAsset) {
        visualAsset->render(posX, posY, scale);
    }

    // Nhãn tên quái vật hiển thị rõ ràng
    DrawText(monsterType.c_str(), static_cast<int>(posX + 2), static_cast<int>(posY + 48 * scale + 4), 14, RAYWHITE);
}

void GameObject::playSound() {
    if (soundAsset) {
        soundAsset->play();
    }
}
