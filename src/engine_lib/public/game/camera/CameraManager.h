#pragma once

// Standard.
#include <mutex>

class CameraNode;
class Renderer;
class CameraProperties;

/** Determines what camera is used to draw on the screen. */
class CameraManager {
    // Active camera node will notify the manager when it's being despawned.
    friend class CameraNode;

public:
    CameraManager() = delete;

    /**
     * Creates a new manager.
     *
     * @param pRenderer Used renderer.
     */
    CameraManager(Renderer* pRenderer);

    /**
     * Makes a camera node to be the primary camera.
     *
     * @remark Only spawned camera nodes can be used here, otherwise an error will be shown.
     *
     * @remark Previously active camera (if there was one) will become inactive.
     *
     * @param pCameraNode Spawned camera node to make active.
     */
    void setActiveCamera(CameraNode* pCameraNode);

    /** Removes the currently active camera so that there will be no active camera. */
    void clearActiveCamera();

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
    std::pair<std::recursive_mutex, CameraNode*>& getActiveCamera();

private:
    /**
     * Called by an active camera node when it's being despawned.
     *
     * @param pCameraNode Camera node that's being despawned.
     */
    void onCameraNodeDespawning(CameraNode* pCameraNode);

    /**
     * Stores active camera.
     *
     * @remark It's safe to store a raw pointer here because when the camera node will be despawning
     * it will notify this manager to clear this pointer.
     */
    std::pair<std::recursive_mutex, CameraNode*> mtxActiveCamera;

    /** Used renderer. */
    Renderer* const pRenderer = nullptr;
};
