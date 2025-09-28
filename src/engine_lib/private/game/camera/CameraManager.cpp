#include "game/camera/CameraManager.h"

// Standard.
#include <format>

// Custom.
#include "io/Logger.h"
#include "game/node/CameraNode.h"
#include "render/PostProcessManager.h"
#include "game/GameManager.h"
#include "misc/Error.h"
#include "render/wrapper/Framebuffer.h"
#include "render/GpuResourceManager.h"
#include "game/Window.h"

CameraManager::CameraManager(GameManager* pGameManager) : pWindow(pGameManager->getWindow()) {
    onWindowSizeChanged(pGameManager->getWindow());
}

CameraManager::~CameraManager() {}

void CameraManager::onWindowSizeChanged(Window* pWindow) {
    const auto [iWindowWidth, iWindowHeight] = pWindow->getWindowSize();

    // Main framebuffer.
    auto iDepthFormat = GL_DEPTH_COMPONENT24;
#if defined(ENGINE_UI_ONLY)
    iDepthFormat = 0;
#endif
    pMainFramebuffer =
        GpuResourceManager::createFramebuffer(iWindowWidth, iWindowHeight, GL_RGB8, iDepthFormat);

    // Post-process framebuffer.
#if !defined(ENGINE_UI_ONLY)
    if (pPostProcessManager == nullptr) {
        pPostProcessManager =
            std::unique_ptr<PostProcessManager>(new PostProcessManager(pWindow->getGameManager()));
    } else {
        pPostProcessManager->onWindowSizeChanged(pWindow);
    }
#endif
}

PostProcessManager& CameraManager::getPostProcessManager() const {
#if defined(ENGINE_UI_ONLY)
    Error::showErrorAndThrowException("post processing manager is not used in UI only apps");
#endif
    return *pPostProcessManager;
}

void CameraManager::setActiveCamera(CameraNode* pCameraNode, bool bIsSoundListener) {
    if (pCameraNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("`nullptr` is not a valid camera");
    }

#if defined(ENGINE_EDITOR)
    if (pCameraNode->getNodeName().find("editor") == std::string::npos) {
        // In case the editor spawns some custom game node that makes some game camera active
        // we don't want to make it the active camera because the editor's "viewport" camera should be active.
        return;
    }
#endif

    std::scoped_lock guard(mtxActiveCamera.first, pCameraNode->getSpawnDespawnMutex());

    if (!pCameraNode->isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "camera node \"{}\" needs to be spawned in order to make it the active camera",
            pCameraNode->getNodeName()));
    }

    mtxActiveCamera.second.pNode = pCameraNode;
    mtxActiveCamera.second.bIsSoundListener = bIsSoundListener;
}

void CameraManager::clearActiveCamera() {
    std::scoped_lock guard(mtxActiveCamera.first);

    if (mtxActiveCamera.second.pNode == nullptr) {
        return;
    }

    // Clear pointer.
    mtxActiveCamera.second.pNode = nullptr;
}

std::optional<glm::vec2> CameraManager::getCursorPosOnViewport() {
    std::scoped_lock guard(mtxActiveCamera.first);

    if (mtxActiveCamera.second.pNode == nullptr) {
        return {};
    }

    if (!pWindow->isMouseCursorVisible()) {
        return {};
    }

    const auto [iCursorX, iCursorY] = pWindow->getCursorPosition();
    const auto [iWindowWidth, iWindowHeight] = pWindow->getWindowSize();

    auto cursorPos = glm::vec2(
        static_cast<float>(iCursorX) / static_cast<float>(iWindowWidth),
        static_cast<float>(iCursorY) / static_cast<float>(iWindowHeight));
    const auto viewport = mtxActiveCamera.second.pNode->getCameraProperties()->getViewport();

    if (viewport.x > cursorPos.x || viewport.y > cursorPos.y) {
        return {};
    }
    if (cursorPos.x > viewport.x + viewport.z || cursorPos.y > viewport.y + viewport.w) {
        return {};
    }

    cursorPos -= glm::vec2(viewport.x, viewport.y);
    return cursorPos / glm::vec2(viewport.z, viewport.w);
}

std::optional<CameraManager::MouseCursorWorldPosResult> CameraManager::convertCursorPosToWorld() {
    std::scoped_lock guard(mtxActiveCamera.first);

    const auto optCursorPos = getCursorPosOnViewport();
    if (!optCursorPos.has_value()) {
        return {};
    }

    return convertViewportPosToWorld(*optCursorPos);
}

std::optional<CameraManager::MouseCursorWorldPosResult>
CameraManager::convertViewportPosToWorld(const glm::vec2& viewportPos) {
    std::scoped_lock guard(mtxActiveCamera.first);

    if (mtxActiveCamera.second.pNode == nullptr) {
        return {};
    }

    const auto pCameraProperties = mtxActiveCamera.second.pNode->getCameraProperties();

    // Convert [0; 1] screen pos to NDC [-1; 1].
    const auto ndcPos =
        glm::vec4(viewportPos.x * 2.0F - 1.0F, (1.0F - viewportPos.y) * 2.0F - 1.0F, 0.0F, 1.0F);

    // Convert to view space.
    const auto invProjMatrix = pCameraProperties->getInverseProjectionMatrix();
    const glm::vec4 viewSpacePosBeforeDiv = invProjMatrix * ndcPos;
    const glm::vec4 viewSpacePos = viewSpacePosBeforeDiv / viewSpacePosBeforeDiv.w;

    // Convert to world space.
    const auto invViewMatrix = pCameraProperties->getInverseViewMatrix();
    const auto worldSpacePos = glm::vec3(invViewMatrix * viewSpacePos);

    const auto cameraPos = mtxActiveCamera.second.pNode->getWorldLocation();

    return MouseCursorWorldPosResult{
        .worldLocation = cameraPos, .worldDirection = glm::normalize(worldSpacePos - cameraPos)};
}

std::pair<std::recursive_mutex, CameraManager::ActiveCameraInfo>& CameraManager::getActiveCamera() {
    return mtxActiveCamera;
}

void CameraManager::onCameraNodeDespawning(CameraNode* pCameraNode) {
    std::scoped_lock guard(mtxActiveCamera.first);

    // Make sure there's an active camera.
    if (mtxActiveCamera.second.pNode == nullptr) [[unlikely]] {
        Logger::get().error(std::format(
            "the camera node \"{}\" notified the camera manager about it being despawned because "
            "it thinks that it's the active camera but the camera manager has no active camera node",
            pCameraNode->getNodeName()));
        return;
    }

    // See if this camera is indeed used as the active one.
    if (mtxActiveCamera.second.pNode != pCameraNode) [[unlikely]] {
        Logger::get().error(std::format(
            "the camera node \"{}\" notified the camera manager about it being despawned because "
            "it thinks that it's the active camera but it's not the active camera node",
            pCameraNode->getNodeName()));
        return;
    }

    // No active camera now.
    mtxActiveCamera.second.pNode = nullptr;
}
