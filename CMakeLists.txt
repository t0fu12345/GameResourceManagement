cmake_minimum_required(VERSION 3.20)
project(GameAssetManager LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Thu muc Header files
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/raylib-5.5_win64_mingw-w64/include
)

# Thu muc chua thu vien Raylib
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/external/raylib-5.5_win64_mingw-w64/lib
)

# Danh sach cac file ma nguon (.cpp)
set(SOURCES
    src/main.cpp
    src/TextureAsset.cpp
    src/MeshAsset.cpp
    src/AudioAsset.cpp
    src/AssetProxy.cpp
    src/AssetManager.cpp
    src/AssetTranscoder.cpp
    src/GameObject.cpp
    src/MemoryProfiler.cpp
    src/GameScene.cpp
)

add_executable(GameAssetManager ${SOURCES})

# Lien ket thu vien Raylib va cac thu vien he thong Windows can thiet
target_link_libraries(GameAssetManager
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/raylib-5.5_win64_mingw-w64/lib/libraylib.a
    opengl32
    gdi32
    winmm
    psapi
    pthread
)

# Sao chep assets vao thu muc output sau khi build neu can
add_custom_command(TARGET GameAssetManager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:GameAssetManager>/assets
)
