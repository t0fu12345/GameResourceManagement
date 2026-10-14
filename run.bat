@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

if not exist "build\GameAssetManager.exe" (
    echo =======================================================
    echo  DANG BIEN DICH GAME ASSET MANAGEMENT FRAMEWORK LAN DAU
    echo =======================================================
    echo  Tu dong kiem tra build tools (Ninja / MinGW / CMake)...

    where ninja >nul 2>nul
    if !errorlevel! equ 0 (
        cmake -B build -G "Ninja"
    ) else (
        where mingw32-make >nul 2>nul
        if !errorlevel! equ 0 (
            cmake -B build -G "MinGW Makefiles"
        ) else (
            cmake -B build
        )
    )

    cmake --build build
)

if exist "build\GameAssetManager.exe" (
    echo =======================================================
    echo  KHOI CHAY NATIVE C++ GAME WINDOW...
    echo =======================================================
    start "" "build\GameAssetManager.exe"
) else (
    echo.
    echo =======================================================
    echo  [LOI BIEN DICH] Khong tao duoc build\GameAssetManager.exe
    echo =======================================================
    echo  Vui long dam bao may ban da cai dat:
    echo   1. Trinh bien dich C++: MinGW-w64 (g++)
    echo   2. Cong cu xay dung: CMake
    echo.
    pause
)
