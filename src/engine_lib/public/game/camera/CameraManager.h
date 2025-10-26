#pragma once

// Standard.
#include <mutex>
#include <memory>
#include <optional>

// Custom.
#include "math/GLMath.hpp"

class CameraNode;
class Renderer;
class CameraProperties;
class PostProcessManager;
class GameManager;
class Window;
class Framebuffer;

/** Determines which camera is used to draw a world on the screen. */
class CameraManager {
    // Active camera node will notify the manager when it's being despawned.
    friend class CameraNode;

    // Renderer notifies to change framebuffer size.
    friend class Renderer;

public:
    /** Groups information about the active camera. */
    struct ActiveCameraInfo {
        ActiveCameraInfo() = default;

        /**
         * It's safe to store a raw pointer here because when the camera node will be despawning
         * it will notify this manager to clear this pointer.
         */
        CameraNode* pNode = nullptr;

        /** `true` to consider position and direction of the camera as player's ears. */
        bool bIsSoundListener = true;
    };

    /** Result of converting mouse cursor position to world space. */
    struct MouseCursorWorldPosResult {
        /** Location of the active camera. */
        glm::vec3 worldLocation;

        /** Normalized direction from @ref worldLocation along the mouse cursor pos. */
        glm::vec3 worldDirection;
    };

    CameraManager() = delete;
    ~CameraManager();

    /**
     * Creates a new manager.
     *
     * @param pGameManager Game manager.
     */
    CameraManager(GameManager* pGameManager);

    /**
     * Makes a camera node to be the primary camera.
     *
     * @remark Only spawned camera nodes can be used here, otherwise an error will be shown.
     *
     * @remark Previously active camera (if there was one) will become inactive.
     *
     * @param pCameraNode      Spawned camera node to make active.
     * @param bIsSoundListener `true` to consider position and direction of the camera as player's ears.
     */
    void setActiveCamera(CameraNode* pCameraNode, bool bIsSoundListener = true);

    /** Removes the currently active camera so that there will be no active camera. */
    void clearActiveCamera();

    /**
     * If the mouse cursor is visible and is inside of the active camera's viewport returns a non-empty value
     * in range [0.0; 1.0] where 0 is viewport's top-left corner and 1 is viewport's right-bottom corner.
     *
     * @return Empty if the cursor is not visible or is outside of the viewport.
     */
    std::optional<glm::vec2> getCursorPosOnViewport();

    /**
     * Converts mouse cursor pos to world space.
     *
     * @return Empty if the cursor is not visible or is outside of the viewport.
     */
    std::optional<MouseCursorWorldPosResult> convertCursorPosToWorld();

    /**
     * Converts the specified position within the active camera's viewport (a rectangle on the screen)
     * to world space.
     *
     * @param viewportPos Position in range [0; 1].
     *
     * @return Empty if no active camera.
     */
    std::optional<MouseCursorWorldPosResult> convertViewportPosToWorld(const glm::vec2& viewportPos);

    /**
     * Returns settings for post processing of the rendered image.
     *
     * @return Settings.
     */
    PostProcessManager& getPostProcessManager() const;

    /**
     * Returns the currently active camera.
     *
     * @warning Don't change pointers to cameras in returned object, only copy pointers or modify
     * camera/node properties.
     *
     * @warning Avoid saving returned reference as it points to the node's field and does not guarantee
     * that the node will always live while you hold this reference.
     *
     * @return `nullptr` as node if there is no active camera, otherwise valid camera.
     */
    std::pair<std::recursive_mutex, ActiveCameraInfo>& getActiveCamera();

    /**
     * Returns framebuffer to draw meshes.
     *
     * @return Framebuffer.
     */
    Framebuffer& getMainFramebuffer() const;

private:
    /**
     * Should be called after window's size changed.
     *
     * @param pWindow Window.
     */
    void onWindowSizeChanged(Window* pWindow);

    /**
     * Called by an active camera node when it's being despawned.
     *
     * @param pCameraNode Camera node that's being despawned.
     */
    void onCameraNodeDespawning(CameraNode* pCameraNode);

    /** Framebuffer for rendering. */
    std::unique_ptr<Framebuffer> pMainFramebuffer;

    /** Settings for post processing of the rendered image. */
    std::unique_ptr<PostProcessManager> pPostProcessManager;

    /** Active camera. */
    std::pair<std::recursive_mutex, ActiveCameraInfo> mtxActiveCamera;

    /** Game's window. */
    Window* const pWindow = nullptr;
};
