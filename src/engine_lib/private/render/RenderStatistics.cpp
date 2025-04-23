#include "render/RenderStatistics.h"

#if defined(WIN32)
#include <Windows.h>
#endif

RenderStatistics::RenderStatistics() {
#if defined(WIN32)
    LARGE_INTEGER perfFreq;
    QueryPerformanceFrequency(&perfFreq);
    fpsLimitInfo.iTimeStampsPerSecond = perfFreq.QuadPart;

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    fpsLimitInfo.iPerfCounterLastFrameEnd = counter.QuadPart;
#endif
}

size_t RenderStatistics::getFramesPerSecond() const { return fpsInfo.iFramesPerSecond; }
