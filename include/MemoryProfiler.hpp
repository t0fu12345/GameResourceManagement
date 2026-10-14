#pragma once

#include <string>
#include <cstddef>

using namespace std;

struct ProcessMemoryInfo {
    size_t workingSetBytes{0};       // RAM vat ly thuc te dang dung
    size_t peakWorkingSetBytes{0};   // RAM dinh diem cao nhat
    size_t privateUsageBytes{0};     // Tong dung luong commit
    int currentFPS{0};
    float frameTimeMs{0.0f};
};

// Memory Profiler: Do dac va theo doi tai nguyen RAM/VRAM/FPS thoi gian thuc tu he dieu hanh
class MemoryProfiler {
public:
    MemoryProfiler() = default;

    // Lay thong tin RAM he thong hien tai tu Windows OS API
    static ProcessMemoryInfo queryProcessMemory();

    // Dinh dang dung luong byte thanh chuoi KB, MB, GB de doc
    static string formatBytes(size_t bytes);
};
