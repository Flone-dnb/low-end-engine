#pragma once

// Standard.
#include <mutex>

// Custom.
#include "math/GLMath.hpp"
#include "misc/Globals.h"
#include "game/geometry/shapes/Frustum.h"

/** Defines how camera can move and rotate. */
enum class CameraMode {
    FREE,   ///< Camera mode in which the camera can freely move and rotate without any restrictions.
    ORBITAL ///< Camera mode in which the camera is fixed and will always look at the specified point
            ///< (target point) in space. The camera can only move and rotate using spherical coordinate
            ///< system (i.e. rotates and moves around the target point).
};

/** Defines camera settings, base axis, location, modes, etc. */
class CameraProperties {
    // Only camera node can control internal data.
    friend class CameraNode;

    // Renderer sets render target size and also looks if projection matrix was changed or
    // not to recalculate grid of frustums for light culling.
    friend class Renderer;

public:
    CameraProperties();

    /** Stores internal data. */
    struct Data {
        Data();

        /** Stores orbital mode specific data. */
        struct OrbitalModeData {
            OrbitalModeData();

            /** Radial distance or distance from camera to target point (look target). */
            float distanceToTarget = 10.0f;

            /** Polar angle (in degrees). */
            float theta = 0.0f;

            /** Azimuthal angle (in degrees). */
            float phi = 0.0f;
        };

        /** Stores data used for view matrix. */
        struct ViewData {
            ViewData();

            /** Matrix that transforms positions to view (camera) space. */
            glm::mat4x4 viewMatrix = glm::identity<glm::mat4x4>();

            /** Matrix that transforms positions to world space from view space. */
            glm::mat4x4 invViewMatrix = glm::identity<glm::mat4x4>();

            /** Whether @ref viewMatrix needs to be recalculated or not. */
            bool bViewMatrixNeedsUpdate = true;

            /** Location of the camera in world space. */
            glm::vec3 worldLocation = glm::vec3(0.0f, 0.0f, 0.0f);

            /** Unit vector that points in camera's current up direction in world space. */
            glm::vec3 worldUpDirection = Globals::WorldDirection::up;

            /** Location of the point in world space that the camera should look at. */
            glm::vec3 targetPointWorldLocation = glm::vec3(1.0f, 0.0f, 0.0f);
        };

        /** Stores data used for projection matrix. */
        struct ProjectionData {
            ProjectionData();

            /**
             * Transforms positions from view (camera) space to 2D projection window
             * (homogeneous clip space).
             */
            glm::mat4x4 projectionMatrix = glm::identity<glm::mat4x4>();

            /** Inverse of @ref projectionMatrix. */
            glm::mat4x4 invProjectionMatrix = glm::identity<glm::mat4x4>();

            /** Whether @ref projectionMatrix needs to be recalculated or not. */
            bool bProjectionMatrixNeedsUpdate = true;

            /**
             * Used by the renderer to track if @ref projectionMatrix was changed or not
             * to recalculate grid of frustums for light culling.
             *
             * @remark Camera only sets this value to `true` and only renderer is allowed
             * to set this value to `false`.
             */
            bool bLightGridFrustumsNeedUpdate = true;

            /** Distance from camera (view) space origin to camera's near clip plane. */
            float nearClipPlaneDistance = 0.2f;

            /** Distance to camera's far clip plane. */
            float farClipPlaneDistance = 250.0f;

            /** Vertical field of view. */
            unsigned int iVerticalFov = 90;

            /** Width of the buffer we are rendering the image to. */
            unsigned int iRenderTargetWidth = 800; // default value, will be changed at runtime

            /** Height of the buffer we are rendering the image to. */
            unsigned int iRenderTargetHeight = 600; // default value, will be changed at runtime

            /** Height of camera's near clip plane. */
            float nearClipPlaneHeight = 0.0f;

            /** Height of camera's far clip plane. */
            float farClipPlaneHeight = 0.0f;
        };

        /**
         * Contains a flag the indicates whether view matrix needs to be recalculated or not
         * and a matrix that transforms positions to view (camera) space.
         *
         * @remark The bool variable is used to minimize the amount of times we recalculate view matrix.
         */
        ViewData viewData;

        /**
         * Contains a flag the indicates whether projection matrix needs to be recalculated or not
         * and a matrix that transforms positions from view (camera) space to 2D projection window
         * (homogeneous clip space).
         *
         * @remark The bool variable is used to minimize the amount of times we recalculate projection
         * matrix.
         */
        ProjectionData projectionData;

        /** Camera's frustum. */
        Frustum frustum;

        /** Defines how camera can move and rotate. */
        CameraMode currentCameraMode = CameraMode::FREE;

        /** Parameters used by orbital camera mode. */
        OrbitalModeData orbitalModeData;

        /** Minimum allowed value for near clip plane distance and far clip plane distance. */
        static inline const float minimumClipPlaneDistance = 0.00001f;
    };

    /**
     * Sets camera's vertical field of view.
     *
     * @param iVerticalFov Vertical field of view.
     */
    void setFov(unsigned int iVerticalFov);

    /**
     * Sets distance from camera (view) space origin to camera's near clip plane.
     *
     * @param nearClipPlaneDistance Near Z distance. Should be a positive value.
     */
    void setNearClipPlaneDistance(float nearClipPlaneDistance);

    /**
     * Sets distance from camera (view) space origin to camera's far clip plane.
     *
     * @param farClipPlaneDistance Far Z distance. Should be a positive value.
     */
    void setFarClipPlaneDistance(float farClipPlaneDistance);

    /**
     * Sets position of the top-left corner of the viewport rectangle in XY and size in ZW (in range [0; 1]).
     *
     * @param viewport Viewport rectangle.
     */
    void setViewport(const glm::vec4& viewport);

    /**
     * Returns vertical field of view of the camera.
     *
     * @return Vertical field of view.
     */
    unsigned int getVerticalFov();

    /**
     * Returns distance from camera (view) space origin to camera's near clip plane.
     *
     * @return Distance to near clip plane.
     */
    float getNearClipPlaneDistance();

    /**
     * Returns distance from camera (view) space origin to camera's far clip plane.
     *
     * @return Distance to far clip plane.
     */
    float getFarClipPlaneDistance();

    /**
     * Returns the current camera mode.
     *
     * @return Camera mode.
     */
    CameraMode getCurrentCameraMode();

    /**
     * Returns orbital camera properties.
     *
     * @return Orbital camera properties.
     */
    Data::OrbitalModeData getOrbitalModeProperties();

    /**
     * Returns camera's world location.
     *
     * @return Location in world space.
     */
    glm::vec3 getWorldLocation();

    /**
     * Returns camera's look direction.
     *
     * @return Uniform vector in world space.
     */
    glm::vec3 getForwardDirection();

    /**
     * Returns a matrix that transforms positions to view (camera) space.
     *
     * @return View matrix.
     */
    glm::mat4x4 getViewMatrix();

    /**
     * Returns a matrix that transforms positions from view (camera) space to 2D projection window
     * (homogeneous clip space).
     *
     * @return Projection matrix.
     */
    glm::mat4x4 getProjectionMatrix();

    /**
     * Returns a matrix that transforms positions to view (camera) space from 2D projection window
     * (homogeneous clip space).
     *
     * @return Inverse of the projection matrix.
     */
    glm::mat4x4 getInverseProjectionMatrix();

    /**
     * Returns a matrix that transforms positions to world space from view space.
     *
     * @return Inverse of the view matrix.
     */
    glm::mat4x4 getInverseViewMatrix();

    /**
     * Returns camera's frustum for fast read-only access.
     *
     * @warning Returned frustum may be outdated (does not include changes made during the last frame),
     * generally you would use this function when you know that frustum is updated (contains latest
     * changes), if not sure call @ref getViewMatrix or @ref getProjectionMatrix that will make sure
     * frustum is updated but since you would call a different function this might have a small
     * performance penalty.
     *
     * @return Camera's frustum.
     */
    inline const Frustum& getCameraFrustumMaybeOutdated() { return mtxData.second.frustum; }

    /**
     * Returns position of the top-left corner of the viewport rectangle in XY and size in ZW
     * (in range [0; 1]).
     *
     * @return Viewport rectangle.
     */
    glm::vec4 getViewport() const { return viewport; }

private:
    /**
     * Sets size of the render target for projection matrix calculations.
     *
     * @remark Does nothing if the specified size is the same as the previous one.
     *
     * @remark Called by the renderer to set render target size.
     *
     * @param iRenderTargetWidth  Width of the buffer we are rendering the image to.
     * @param iRenderTargetHeight Height of the buffer we are rendering the image to.
     */
    void setRenderTargetProportions(unsigned int iRenderTargetWidth, unsigned int iRenderTargetHeight);

    /**
     * Recalculates camera's view matrix if it needs to be updated.
     *
     * @remark This function can ignore the call if there's no need to recalculate view matrix.
     */
    void makeSureViewMatrixIsUpToDate();

    /**
     * Recalculates camera's projection matrix and clip plane heights if they need to be updated.
     *
     * @remark This function can ignore the call if there's no need to recalculate this data.
     */
    void makeSureProjectionMatrixAndClipPlanesAreUpToDate();

    /**
     * Recalculates camera's frustum.
     *
     * @remark Called after view or projection data is updated.
     */
    void recalculateFrustum();

    /** Internal properties. */
    std::pair<std::recursive_mutex, Data> mtxData{};

    /** Position of the top-left corner of the viewport rectangle in XY and size in ZW (in range [0; 1]). */
    glm::vec4 viewport = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    /** Delta to compare input to zero. */
    static inline constexpr float floatDelta = 0.00001f;
};
