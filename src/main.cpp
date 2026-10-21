#include "raylib.h"
#include "GameScene.hpp"
#include <iostream>

using namespace std;

int main() {
    // 1. Cau hinh kich thuoc mac dinh 1440x810 va BAT tinh nang co gian / phong to (FLAG_WINDOW_RESIZABLE)
    const int defaultWidth = 1440;
    const int defaultHeight = 810;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(defaultWidth, defaultHeight, "Game Asset Management Framework - OOP Project (NEU)");
    SetWindowMinSize(1080, 640); // Gioi han kich thuoc nho nhat de khong bi vo UI

    InitAudioDevice();
    SetTargetFPS(60);

    // Asset/GameScene phai bi huy truoc khi Audio/Window context dong.
    {
    GameScene scene;
    scene.initialize();

    cout << "=======================================================\n";
    cout << " GAME ASSET MANAGEMENT FRAMEWORK - NATIVE C++ WINDOW   \n";
    cout << "=======================================================\n";
    cout << " [Nut o giua (Maximize)]: Phong to toan man hinh (Hoac F11)\n";
    cout << " [SPACE/ENTER]          : Spawn 50 quai vat\n";
    cout << " [C]                    : Xoa toan bo quai vat (Kiem chung Ref Count -> 0)\n";
    cout << " [1]                    : Bat / Tat Mau Flyweight\n";
    cout << " [2]                    : Bat / Tat Mau Virtual Proxy (Async Loading)\n";
    cout << " [3]                    : Kich hoat don dep Cache LRU\n";
    cout << " [4]                    : Kich hoat Transcoder nen Texture giam VRAM\n";
    cout << " [5]                    : Benchmark 1.000 entity (Flyweight + Proxy)\n";
    cout << " [P]                    : Phat am thanh SFX mau\n";
    cout << " [F11]                  : Toan man hinh (Fullscreen)\n";
    cout << "=======================================================\n";

    // 3. Vong lap Game Loop chinh
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Lay kich thuoc hien tai theo thoi gian thuc (khi phong to / thu nho cua so)
        float currentWidth = static_cast<float>(GetScreenWidth());
        float currentHeight = static_cast<float>(GetScreenHeight());

        // Phim F11 hoac Alt+Enter de bat/tat toan man hinh
        if (IsKeyPressed(KEY_F11) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            ToggleFullscreen();
        }

        // Xu ly phim bam tuong tac
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            scene.spawnHorde(50, currentWidth, currentHeight);
        }
        if (IsKeyPressed(KEY_C)) {
            scene.clearAllEntities();
        }
        if (IsKeyPressed(KEY_ONE)) {
            scene.toggleFlyweight();
        }
        if (IsKeyPressed(KEY_TWO)) {
            scene.toggleProxy();
        }
        if (IsKeyPressed(KEY_THREE)) {
            scene.triggerLRUEvict();
        }
        if (IsKeyPressed(KEY_FOUR)) {
            scene.triggerTranscoderDemo();
        }
        if (IsKeyPressed(KEY_FIVE)) {
            scene.runThousandObjectDemo(currentWidth, currentHeight);
        }
        if (IsKeyPressed(KEY_P)) {
            scene.playAttackSound();
        }

        // Cap nhat logic theo kich thuoc man hinh dong
        scene.update(dt, currentWidth, currentHeight);

        // Render do hoa va HUD Dashboard tu thich ung theo kich thuoc man hinh
        BeginDrawing();
        scene.draw(currentWidth, currentHeight);
        EndDrawing();
    }

    scene.shutdown();
    AssetManager::getInstance().shutdown();
    }

    // 4. Chi dong context sau khi tat ca asset Raylib da duoc unload.
    CloseAudioDevice();
    CloseWindow();

    cout << "[Main] Ung dung da dong an toan.\n";
    return 0;
}
