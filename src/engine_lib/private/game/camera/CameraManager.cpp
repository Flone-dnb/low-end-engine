#include "game/camera/CameraManager.h"

// Standard.
#include <format>

// Custom.
#include "io/Logger.h"
#include "game/node/CameraNode.h"
#include "misc/Error.h"

CameraManager::CameraManager(Renderer* pRenderer) : pRenderer(pRenderer) {}

void CameraManager::setActiveCamera(CameraNode* pCameraNode) {
    if (pCameraNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("`nullptr` is not a valid camera");
    }

    std::scoped_lock guard(mtxActiveCamera.first);

    // Make sure this node is spawned.
    std::scoped_lock nodeSpawnGuard(pCameraNode->getSpawnDespawnMutex());
    if (!pCameraNode->isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "camera node \"{}\" needs to be spawned in order to make it the active camera",
            pCameraNode->getNodeName()));
    }

    // don't unlock the node mutex yet

    mtxActiveCamera.second = pCameraNode;
}

void CameraManager::clearActiveCamera() {
    std::scoped_lock guard(mtxActiveCamera.first);

    if (mtxActiveCamera.second == nullptr) {
        return;
    }

    // Clear pointer.
    mtxActiveCamera.second = nullptr;
}

std::pair<std::recursive_mutex, CameraNode*>& CameraManager::getActiveCamera() { return mtxActiveCamera; }

void CameraManager::onCameraNodeDespawning(CameraNode* pCameraNode) {
    std::scoped_lock guard(mtxActiveCamera.first);

    // Make sure there's an active camera.
    if (mtxActiveCamera.second == nullptr) [[unlikely]] {
        Logger::get().error(std::format(
            "the camera node \"{}\" notified the camera manager about it being despawned because "
            "it thinks that it's the active camera but the camera manager has no active camera node",
            pCameraNode->getNodeName()));
        return;
    }

    // See if this camera is indeed used as the active one.
    if (mtxActiveCamera.second != pCameraNode) [[unlikely]] {
        Logger::get().error(std::format(
            "the camera node \"{}\" notified the camera manager about it being despawned because "
            "it thinks that it's the active camera but it's not the active camera node",
            pCameraNode->getNodeName()));
        return;
    }

    // No active camera now.
    mtxActiveCamera.second = nullptr;
}
