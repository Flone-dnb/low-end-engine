#pragma once

// Standard.
#include <chrono>
#include <optional>

/** Stores various statistics about rendering (FPS for example). */
class RenderStatistics {
    // Renderer will update statistics.
    friend class Renderer;

public:
    RenderStatistics() = default;

    RenderStatistics(const RenderStatistics&) = delete;
    RenderStatistics& operator=(const RenderStatistics&) = delete;

    /**
     * Returns the total number of frames that the renderer produced in the last second.
     *
     * @return Zero if not calculated yet (wait at least 1 second), otherwise FPS count.
     */
    size_t getFramesPerSecond() const;

private:
    /** Groups info related to measuring frame count per second. */
    struct FramesPerSecondInfo {
        /**
         * Time when the renderer has finished initializing or when
         * @ref iFramesPerSecond was updated.
         */
        std::chrono::steady_clock::time_point timeAtLastFpsUpdate;

        /**
         * The number of times the renderer presented a new image since the last time we updated
         * @ref iFramesPerSecond.
         */
        size_t iPresentCountSinceFpsUpdate = 0;

        /** The number of frames that the renderer produced in the last second. */
        size_t iFramesPerSecond = 0;
    };

    /** Groups info related to FPS limiting. */
    struct FpsLimitInfo {
        /** Time when last frame was started to be processed. */
        std::chrono::steady_clock::time_point frameStartTime;

        /** 0 if not set. */
        unsigned int iFpsLimit = 0;

        /** Not empty if FPS limit is set, defines time in nanoseconds that one frame should take. */
        std::optional<double> optionalTargetTimeToRenderFrameInNs;
    };

    /** Info related to measuring frame count per second. */
    FramesPerSecondInfo fpsInfo;

    /** Info related to FPS limiting. */
    FpsLimitInfo fpsLimitInfo;
};
