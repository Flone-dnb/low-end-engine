#pragma once

/*
 * Some code is from:
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) ||                                                            \
    (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#endif

#else
#error "unknown OS"
#endif

/** Provides static functions for querying RAM usage. */
class MemoryUsage {
public:
    /**
     * Returns the current resident set size (physical memory use) that this process is using.
     *
     * @return Size in bytes.
     */
    static inline size_t getMemorySizeUsedByProcess() {
#if defined(_WIN32)
        /* Windows -------------------------------------------------- */
        PROCESS_MEMORY_COUNTERS info;
        GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
        return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
        /* OSX ------------------------------------------------------ */
        struct mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) != KERN_SUCCESS)
            return (size_t)0L; /* Can't access? */
        return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
        /* Linux ---------------------------------------------------- */
        long rss = 0L;
        FILE* fp = NULL;
        if ((fp = fopen("/proc/self/statm", "r")) == NULL)
            return (size_t)0L; /* Can't open? */
        if (fscanf(fp, "%*s%ld", &rss) != 1) {
            fclose(fp);
            return (size_t)0L; /* Can't read? */
        }
        fclose(fp);
        return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);

#else
        static_assert(false, "not supported OS");
#endif
    }

    /**
     * Returns the total physical memory (RAM) size.
     *
     * @return Size in bytes.
     */
    static inline size_t getTotalMemorySize() {
#if defined(_WIN32)
        MEMORYSTATUSEX memInfo{};
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return static_cast<size_t>(memInfo.ullTotalPhys);
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
        struct sysinfo memInfo;
        sysinfo(&memInfo);
        long long totalPhysMem = memInfo.totalram;
        totalVirtualMem *= memInfo.mem_unit;
        return static_cast<size_t>(totalPhysMem);
#else
        static_assert(false, "not supported OS");
#endif
    }

    /**
     * Returns the total physical memory (RAM) size that's being used.
     *
     * @return Size in bytes.
     */
    static inline size_t getTotalMemorySizeUsed() {
#if defined(_WIN32)
        MEMORYSTATUSEX memInfo{};
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return static_cast<size_t>(memInfo.ullTotalPhys - memInfo.ullAvailPhys);
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
        struct sysinfo memInfo;
        sysinfo(&memInfo);
        long long physMemUsed = memInfo.totalram - memInfo.freeram;
        physMemUsed *= memInfo.mem_unit;
        return static_cast<size_t>(physMemUsed);
#else
        static_assert(false, "not supported OS");
#endif
    }
};
