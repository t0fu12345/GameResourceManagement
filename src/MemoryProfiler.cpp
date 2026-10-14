#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#include <psapi.h>

#include "MemoryProfiler.hpp"
#include <sstream>
#include <iomanip>

using namespace std;

ProcessMemoryInfo MemoryProfiler::queryProcessMemory() {
    ProcessMemoryInfo info{};

    PROCESS_MEMORY_COUNTERS_EX pmc;
    HANDLE hProcess = GetCurrentProcess();

    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        info.workingSetBytes = pmc.WorkingSetSize;
        info.peakWorkingSetBytes = pmc.PeakWorkingSetSize;
        info.privateUsageBytes = pmc.PrivateUsage;
    }

    return info;
}

string MemoryProfiler::formatBytes(size_t bytes) {
    stringstream ss;
    ss << fixed << setprecision(2);

    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        ss << (double)bytes / (1024.0 * 1024.0 * 1024.0) << " GB";
    } else if (bytes >= 1024ULL * 1024ULL) {
        ss << (double)bytes / (1024.0 * 1024.0) << " MB";
    } else if (bytes >= 1024ULL) {
        ss << (double)bytes / 1024.0 << " KB";
    } else {
        ss << bytes << " Bytes";
    }

    return ss.str();
}
