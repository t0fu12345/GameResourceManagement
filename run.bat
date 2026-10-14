@echo off
cd /d "%~dp0"

if not exist "build\GameAssetManager.exe" (
    echo Compiling GameAssetManager...
    cmake -B build -G "Ninja"
    cmake --build build
)

echo =======================================================
echo  GAME ASSET MANAGEMENT FRAMEWORK (NEU OOP PROJECT)
echo =======================================================
echo  Dang mo cua so game Native C++...
start "" "build\GameAssetManager.exe"
