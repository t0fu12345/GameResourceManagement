@echo off
setlocal
rem Bat buoc cmd/CMake/Ninja dung UTF-8 de giu dung duong dan co ky tu "i" dau huyen.
chcp 65001 >nul
cd /d "%~dp0"

cmake --version >nul 2>nul
if errorlevel 1 goto :tool_error

rem CMakeCache.txt records absolute source/build paths and cannot be reused
rem after the project directory is moved or copied.
set "SOURCE_DIR=%CD:\=/%"
set "BINARY_DIR=%SOURCE_DIR%/build"
if exist "build\CMakeCache.txt" (
    findstr /L /I /X /C:"CMAKE_HOME_DIRECTORY:INTERNAL=%SOURCE_DIR%" "build\CMakeCache.txt" >nul
    if errorlevel 1 goto :refresh_cache
    findstr /L /I /X /C:"CMAKE_CACHEFILE_DIR:INTERNAL=%BINARY_DIR%" "build\CMakeCache.txt" >nul
    if errorlevel 1 goto :refresh_cache
)
goto :configure

:refresh_cache
echo  Phat hien CMake cache tu vi tri cu. Tao lai cau hinh build...
cmake -E rm -f "build\CMakeCache.txt"
if errorlevel 1 goto :build_error
cmake -E rm -rf "build\CMakeFiles"
if errorlevel 1 goto :build_error

:configure
if not exist "build\CMakeCache.txt" (
    echo =======================================================
    echo  DANG BIEN DICH GAME ASSET MANAGEMENT FRAMEWORK LAN DAU
    echo =======================================================
    echo  Tu dong kiem tra build tools [Ninja / MinGW / CMake]...

    ninja --version >nul 2>nul
    if not errorlevel 1 (
        cmake -B build -G "Ninja"
    ) else (
        mingw32-make --version >nul 2>nul
        if not errorlevel 1 (
            cmake -B build -G "MinGW Makefiles"
        ) else (
            cmake -B build
        )
    )

    if errorlevel 1 goto :build_error
)

cmake -S . -B build -DBUILD_TESTING=ON
if errorlevel 1 goto :build_error
cmake --build build --config Release
if errorlevel 1 goto :build_error
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto :build_error

if /I "%~1"=="--build-only" exit /b 0

set "GAME_EXE=build\GameAssetManager.exe"
if not exist "%GAME_EXE%" set "GAME_EXE=build\Release\GameAssetManager.exe"
if not exist "%GAME_EXE%" goto :build_error

echo =======================================================
echo  KHOI CHAY NATIVE C++ GAME WINDOW...
echo =======================================================

rem Chay truc tiep de cua so game thuoc phien hien tai va de hien loi runtime.
"%GAME_EXE%"
if errorlevel 1 goto :run_error
exit /b 0

:tool_error
echo.
echo =======================================================
echo  [LOI CONG CU] Khong tim thay CMake trong PATH
echo =======================================================
echo  Hay cai CMake, mo terminal moi, sau do chay lai run.bat.
pause
exit /b 1

:build_error
echo.
echo =======================================================
echo  [LOI BIEN DICH] Khong tao duoc build\GameAssetManager.exe
echo =======================================================
echo  Vui long dam bao may ban da cai dat:
echo   1. Trinh bien dich C++: MinGW-w64 (g++)
echo   2. Cong cu xay dung: CMake
echo.
pause
exit /b 1

:run_error
set "GAME_EXIT_CODE=%errorlevel%"
echo.
echo =======================================================
echo  [LOI KHOI CHAY] Game da thoat voi ma loi %GAME_EXIT_CODE%
echo =======================================================
echo  Xem cac dong log phia tren de biet nguyen nhan.
pause
exit /b %GAME_EXIT_CODE%
