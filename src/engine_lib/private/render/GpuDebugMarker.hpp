#pragma once

#if defined(DEBUG)

#include <string_view>
#include "glad/glad.h"

/** RAII-style object for creating a GPU debug markers (groups GPU commands in RenderDoc). */
class ScopedGpuDebugSection {
public:
    ScopedGpuDebugSection() = delete;

    /**
     * Creates a new query.
     *
     * @param sSectionName Name of the section.
     */
    ScopedGpuDebugSection(std::string_view sSectionName) {
        glPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION, 0, -1, sSectionName.data());
    }

    ~ScopedGpuDebugSection() { glPopDebugGroupKHR(); }
};

#define GPU_MARKER_SCOPED(sSectionName) ScopedGpuDebugSection gpuSection(sSectionName);
#else
#define GPU_MARKER_SCOPED(sSectionName)
#endif
