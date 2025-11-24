#pragma once

#if defined(ENGINE_DEBUG_TOOLS)

#include "glad/glad.h"

/** RAII-style object for creating a GPU time query section. */
class ScopedGpuTimeQuery {
public:
    ScopedGpuTimeQuery() = delete;

    /**
     * Creates a new query.
     *
     * @param iGlQuery OpenGL ID of the query.
     */
    ScopedGpuTimeQuery(unsigned int& iGlQuery) { glBeginQueryEXT(GL_TIME_ELAPSED_EXT, iGlQuery); }

    ~ScopedGpuTimeQuery() { glEndQueryEXT(GL_TIME_ELAPSED_EXT); }
};

//                                              variable name NOT UNIQUE because queries should not intersect
#define MEASURE_GPU_TIME_SCOPED(iGlQuery) ScopedGpuTimeQuery gpuQuery(iGlQuery);
#else
#define MEASURE_GPU_TIME_SCOPED(iGlQuery)
#endif
