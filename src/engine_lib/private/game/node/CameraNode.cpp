#include "game/node/CameraNode.h"

// Custom.
#include "game/GameInstance.h"
#include "game/camera/CameraManager.h"
#include "math/MathHelpers.hpp"
#include "game/World.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "e472b11f-7914-49f8-a86e-a500e6bb749f";
}

std::string CameraNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string CameraNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo CameraNode::getReflectionInfo() {
    ReflectedVariables variables;

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(CameraNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<CameraNode>(); },
        std::move(variables));
}

CameraNode::CameraNode() : CameraNode("Camera Node") {}

CameraNode::CameraNode(const std::string& sNodeName) : SpatialNode(sNodeName) {}

void CameraNode::onWorldLocationRotationScaleChanged() {
    PROFILE_FUNC

    SpatialNode::onWorldLocationRotationScaleChanged();

    {
        // Update origin location in world space.
        auto& mtxSpatialParent = getClosestSpatialParent();
        std::scoped_lock spatialParentGuard(mtxSpatialParent.first);

        glm::mat4x4 parentWorldMatrix = glm::identity<glm::mat4x4>();
        if (mtxSpatialParent.second != nullptr) {
            parentWorldMatrix = mtxSpatialParent.second->getWorldMatrix();
        }

        localSpaceOriginInWorldSpace = parentWorldMatrix * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
    }

    updateCameraProperties();
}

CameraProperties* CameraNode::getCameraProperties() { return &cameraProperties; }

void CameraNode::onDespawning() {
    SpatialNode::onDespawning();

    {
        // Notify camera manager if this node is the active camera.
        auto& mtxActiveCamera = getWorldWhileSpawned()->getCameraManager().getActiveCamera();
        std::scoped_lock guardActive(mtxActiveCamera.first);
        if (mtxActiveCamera.second.pNode == this) {
            getWorldWhileSpawned()->getCameraManager().onCameraNodeDespawning(this);
        }
    }
}

void CameraNode::updateCameraProperties() {
    PROFILE_FUNC

    // Update camera properties.
    std::scoped_lock cameraPropertiesGuard(cameraProperties.mtxData.first);

    CameraProperties::Data::ViewData& viewData = cameraProperties.mtxData.second.viewData;

    viewData.worldLocation = getWorldLocation();

    if (cameraProperties.mtxData.second.currentCameraMode == CameraMode::FREE) {
        // Get target direction to point along Node's forward (to be used in view matrix).
        viewData.targetPointWorldLocation = viewData.worldLocation + getWorldForwardDirection();

        // Get world up from Node's up (to be used in view matrix).
        viewData.worldUpDirection = getWorldUpDirection();
    } else {
        // Update target for view matrix.
        if (orbitalCameraTargetInWorldSpace.has_value()) {
            viewData.targetPointWorldLocation = orbitalCameraTargetInWorldSpace.value();
        } else {
            viewData.targetPointWorldLocation = localSpaceOriginInWorldSpace;
        }

        // Update rotation.
        MathHelpers::convertCartesianCoordinatesToSpherical(
            viewData.worldLocation - viewData.targetPointWorldLocation,
            cameraProperties.mtxData.second.orbitalModeData.distanceToTarget,
            cameraProperties.mtxData.second.orbitalModeData.theta,
            cameraProperties.mtxData.second.orbitalModeData.phi);

        // Make node look at target.
        const auto toTarget = viewData.targetPointWorldLocation - viewData.worldLocation;
        glm::vec3 targetRotation =
            MathHelpers::convertNormalizedDirectionToRollPitchYaw(MathHelpers::normalizeSafely(toTarget));

        // Set rotation if different.
        if (!glm::all(glm::epsilonEqual(targetRotation, getWorldRotation(), rotationDelta))) {
            setWorldRotation(targetRotation);
        }

        // Get world up from Node's up (to be used in view matrix).
        viewData.worldUpDirection = getWorldUpDirection();
    }

    // Mark view matrix as "needs update".
    cameraProperties.mtxData.second.viewData.bViewMatrixNeedsUpdate = true;
}

void CameraNode::setCameraMode(CameraMode mode) {
    // Update camera properties.
    std::scoped_lock cameraPropertiesGuard(cameraProperties.mtxData.first);

    cameraProperties.mtxData.second.currentCameraMode = mode;

    updateCameraProperties();
}

void CameraNode::clearOrbitalTargetLocation() {
    // Update camera properties.
    std::scoped_lock cameraPropertiesGuard(cameraProperties.mtxData.first);

    // Make sure we are in the orbital camera mode.
    if (cameraProperties.mtxData.second.currentCameraMode == CameraMode::FREE) [[unlikely]] {
        Logger::get().warn(
            "an attempt to clear orbital camera's target location was ignored because the camera is not "
            "in the orbital mode");
        return;
    }

    orbitalCameraTargetInWorldSpace = {};

    updateCameraProperties();
}

void CameraNode::setOrbitalTargetLocation(const glm::vec3& targetPointLocation) {
    // Update camera properties.
    std::scoped_lock cameraPropertiesGuard(cameraProperties.mtxData.first);

    // Make sure we are in the orbital camera mode.
    if (cameraProperties.mtxData.second.currentCameraMode == CameraMode::FREE) [[unlikely]] {
        Logger::get().warn(
            "an attempt to set orbital camera's target location was ignored because the camera is not in "
            "the orbital mode");
        return;
    }

    orbitalCameraTargetInWorldSpace = targetPointLocation;

    updateCameraProperties();
}

void CameraNode::setOrbitalRotation(float phi, float theta) {
    // Update camera properties.
    std::scoped_lock cameraPropertiesGuard(cameraProperties.mtxData.first);

    // Make sure we are in the orbital camera mode.
    if (cameraProperties.mtxData.second.currentCameraMode == CameraMode::FREE) [[unlikely]] {
        Logger::get().warn(
            "an attempt to set orbital camera's rotation was ignored because the camera is not in "
            "the orbital mode");
        return;
    }

    // Update properties.
    cameraProperties.mtxData.second.orbitalModeData.phi = phi;
    cameraProperties.mtxData.second.orbitalModeData.theta = theta;

    // Change node's location according to new spherical rotation.
    const auto newWorldLocation = MathHelpers::convertSphericalToCartesianCoordinates(
                                      cameraProperties.mtxData.second.orbitalModeData.distanceToTarget,
                                      cameraProperties.mtxData.second.orbitalModeData.theta,
                                      cameraProperties.mtxData.second.orbitalModeData.phi) +
                                  cameraProperties.mtxData.second.viewData.targetPointWorldLocation;

    setWorldLocation(newWorldLocation); // causes `updateCameraProperties` to be called
}

void CameraNode::setOrbitalDistanceToTarget(float distanceToTarget) {
    // Update camera properties.
    std::scoped_lock cameraPropertiesGuard(cameraProperties.mtxData.first);

    // Make sure we are in the orbital camera mode.
    if (cameraProperties.mtxData.second.currentCameraMode == CameraMode::FREE) [[unlikely]] {
        Logger::get().warn(
            "an attempt to set orbital camera's rotation was ignored because the camera is not in "
            "the orbital mode");
        return;
    }

    // Update properties.
    cameraProperties.mtxData.second.orbitalModeData.distanceToTarget = distanceToTarget;

    // Change node's location according to new spherical rotation.
    const auto newWorldLocation = MathHelpers::convertSphericalToCartesianCoordinates(
                                      cameraProperties.mtxData.second.orbitalModeData.distanceToTarget,
                                      cameraProperties.mtxData.second.orbitalModeData.theta,
                                      cameraProperties.mtxData.second.orbitalModeData.phi) +
                                  cameraProperties.mtxData.second.viewData.targetPointWorldLocation;

    setWorldLocation(newWorldLocation); // causes `updateCameraProperties` to be called
}

glm::vec3 CameraNode::getOrbitalTargetLocation() {
    std::scoped_lock cameraPropertiesGuard(cameraProperties.mtxData.first);

    // Make sure we are in the orbital camera mode.
    if (cameraProperties.mtxData.second.currentCameraMode == CameraMode::FREE) [[unlikely]] {
        Logger::get().warn(
            "an attempt to get orbital camera's target location was ignored because the camera is not in "
            "the orbital mode");
        return glm::vec3(0.0F, 0.0F, 0.0F);
    }

    if (orbitalCameraTargetInWorldSpace.has_value()) {
        return orbitalCameraTargetInWorldSpace.value();
    }

    return localSpaceOriginInWorldSpace;
}

void CameraNode::makeActive(bool bIsSoundListener) {
    getWorldWhileSpawned()->getCameraManager().setActiveCamera(this, bIsSoundListener);
}
