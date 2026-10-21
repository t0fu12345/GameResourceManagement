#include "MemoryProfiler.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/resource.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

using namespace std;

ProcessMemoryInfo MemoryProfiler::queryProcessMemory() {
    ProcessMemoryInfo info{};
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        info.workingSetBytes = pmc.WorkingSetSize;
        info.peakWorkingSetBytes = pmc.PeakWorkingSetSize;
        info.privateUsageBytes = pmc.PrivateUsage;
    }
#elif defined(__APPLE__)
    mach_task_basic_info_data_t taskInfo{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&taskInfo), &count) == KERN_SUCCESS) {
        info.workingSetBytes = taskInfo.resident_size;
        info.privateUsageBytes = taskInfo.virtual_size;
    }
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) info.peakWorkingSetBytes = static_cast<size_t>(usage.ru_maxrss);
#else
    ifstream status("/proc/self/status");
    string line;
    while (getline(status, line)) {
        istringstream row(line);
        string key;
        size_t valueKb = 0;
        string unit;
        if (!(row >> key >> valueKb >> unit)) continue;
        if (key == "VmRSS:") info.workingSetBytes = valueKb * 1024ULL;
        else if (key == "VmHWM:") info.peakWorkingSetBytes = valueKb * 1024ULL;
        else if (key == "VmSize:") info.privateUsageBytes = valueKb * 1024ULL;
    }
#endif
    return info;
}

string MemoryProfiler::formatBytes(size_t bytes) {
    stringstream ss;
    ss << fixed << setprecision(2);
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) ss << static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0) << " GB";
    else if (bytes >= 1024ULL * 1024ULL) ss << static_cast<double>(bytes) / (1024.0 * 1024.0) << " MB";
    else if (bytes >= 1024ULL) ss << static_cast<double>(bytes) / 1024.0 << " KB";
    else ss << bytes << " Bytes";
    return ss.str();
}
