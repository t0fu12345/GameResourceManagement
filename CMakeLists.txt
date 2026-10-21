cmake_minimum_required(VERSION 3.20)
project(GameAssetManager LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
find_package(Threads REQUIRED)

set(CORE_SOURCES
    src/TextureAsset.cpp
    src/MeshAsset.cpp
    src/AudioAsset.cpp
    src/AssetProxy.cpp
    src/AssetManager.cpp
    src/AssetCache.cpp
    src/AssetLoadService.cpp
    src/AssetTranscoder.cpp
    src/GameObject.cpp
    src/MemoryProfiler.cpp
    src/GameScene.cpp
)

add_library(GameAssetCore STATIC ${CORE_SOURCES})
target_include_directories(GameAssetCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/raylib-5.5_win64_mingw-w64/lib/libraylib.a")
    add_library(raylib_vendor STATIC IMPORTED GLOBAL)
    set_target_properties(raylib_vendor PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/external/raylib-5.5_win64_mingw-w64/lib/libraylib.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/external/raylib-5.5_win64_mingw-w64/include"
    )
    target_link_libraries(GameAssetCore PUBLIC raylib_vendor Threads::Threads opengl32 gdi32 winmm psapi)
else()
    find_package(raylib 5.5 REQUIRED)
    target_link_libraries(GameAssetCore PUBLIC raylib Threads::Threads)
endif()

add_executable(GameAssetManager src/main.cpp)
target_link_libraries(GameAssetManager PRIVATE GameAssetCore)

add_custom_command(TARGET GameAssetManager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:GameAssetManager>/assets
)

if(BUILD_TESTING)
    add_executable(GameAssetCoreTests tests/CoreTests.cpp)
    target_link_libraries(GameAssetCoreTests PRIVATE GameAssetCore)
    add_test(NAME GameAssetCoreTests COMMAND GameAssetCoreTests)
    set_tests_properties(GameAssetCoreTests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
