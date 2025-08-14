#include "node/GizmoNode.h"

// Custom.
#include "game/node/MeshNode.h"
#include "math/MathHelpers.hpp"
#include "EditorResourcePaths.hpp"
#include "game/World.h"
#include "game/camera/CameraManager.h"
#include "EditorConstants.hpp"
#include "EditorGameInstance.h"
#include "node/property_inspector/PropertyInspector.h"
#include "game/node/ui/TextUiNode.h"

GizmoNode::GizmoNode(GizmoMode mode, SpatialNode* pControlledNode)
    : SpatialNode(std::string(EditorConstants::getHiddenNodeNamePrefix()) + " Gizmo Node"), mode(mode),
      pControlledNode(pControlledNode) {
    setSerialize(false);
    setIsReceivingInput(true);

    // Deserialize gizmo geometry.
    std::unique_ptr<Node> pXAxisGizmoU;
    switch (mode) {
    case (GizmoMode::MOVE): {
        auto result = Node::deserializeNodeTree(
            EditorResourcePaths::getPathToModelsDirectory() / "gizmo_move" / "gizmo_move.toml");
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            error.showErrorAndThrowException();
        }
        pXAxisGizmoU = std::get<std::unique_ptr<Node>>(std::move(result));
        break;
    }
    case (GizmoMode::ROTATE): {
        Error::showErrorAndThrowException("not implemented");
        break;
    }
    case (GizmoMode::SCALE): {
        Error::showErrorAndThrowException("not implemented");
        break;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
    }
    }

    const auto pXAxisGizmo = dynamic_cast<MeshNode*>(pXAxisGizmoU.get());
    if (pXAxisGizmo == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected a mesh node");
    }
    if (!pXAxisGizmo->getChildNodes().second.empty()) [[unlikely]] {
        Error::showErrorAndThrowException("expected a single mesh node");
    }

    MeshGeometry gizmoGeometry = pXAxisGizmo->copyMeshData();

    auto pYAxisGizmoU = std::make_unique<MeshNode>("Gizmo Y");
    pYAxisGizmoU->setMeshGeometryBeforeSpawned(gizmoGeometry);
    pYAxisGizmoU->setRelativeRotation(glm::vec3(0.0F, 0.0F, 90.0F));

    auto pZAxisGizmoU = std::make_unique<MeshNode>("Gizmo Z");
    pZAxisGizmoU->setMeshGeometryBeforeSpawned(std::move(gizmoGeometry));
    pZAxisGizmoU->setRelativeRotation(glm::vec3(0.0F, -90.0F, 0.0F));

    // Set color.
    pXAxisGizmo->getMaterial().setDiffuseColor(glm::vec3(1.0F, 0.0F, 0.0F));
    pYAxisGizmoU->getMaterial().setDiffuseColor(glm::vec3(0.0F, 1.0F, 0.0F));
    pZAxisGizmoU->getMaterial().setDiffuseColor(glm::vec3(0.0F, 0.0F, 1.0F));

    // Save.
    pXAxisGizmoNode = pXAxisGizmo;
    pYAxisGizmoNode = pYAxisGizmoU.get();
    pZAxisGizmoNode = pZAxisGizmoU.get();

    pXAxisGizmoNode->setSerialize(false);
    pYAxisGizmoNode->setSerialize(false);
    pZAxisGizmoNode->setSerialize(false);

    addChildNode(std::move(pXAxisGizmoU));
    addChildNode(std::move(pYAxisGizmoU));
    addChildNode(std::move(pZAxisGizmoU));

    // Add usage hint.
    {
        const auto pUsageHintText = addChildNode(std::make_unique<TextUiNode>(
            std::format("{}Gizmo Hint", EditorConstants::getHiddenNodeNamePrefix())));
        pUsageHintText->setSerialize(false);

        pUsageHintText->setPosition(glm::vec2(0.6F, 0.01F));
        pUsageHintText->setTextHeight(0.025F);
        pUsageHintText->setSize(
            glm::vec2(1.0F - pUsageHintText->getPosition().x, pUsageHintText->getTextHeight() * 1.25F));
        pUsageHintText->setText(u"gizmo usage (keyboard): 1 - move, 2 - rotate, 3 - scale");
    }
}

size_t GizmoNode::getAxisNodeId(GizmoAxis axis) {
    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException("this function can only be used while spawned");
    }

    switch (axis) {
    case (GizmoAxis::X): {
        return pXAxisGizmoNode->getNodeId().value();
        break;
    }
    case (GizmoAxis::Y): {
        return pYAxisGizmoNode->getNodeId().value();
        break;
    }
    case (GizmoAxis::Z): {
        return pZAxisGizmoNode->getNodeId().value();
        break;
    }
    }
}

std::optional<float>
GizmoNode::calculateOffsetFromGizmoToCursorRay(const glm::vec3& gizmoOriginalLocation, GizmoAxis axis) {
    // Get cursor pos in world.
    auto& cameraManager = getWorldWhileSpawned()->getCameraManager();
    const auto optCursorWorldInfo = cameraManager.convertCursorPosToWorld();
    if (!optCursorWorldInfo.has_value()) {
        return {};
    }

    glm::vec3 normalForPlaneAlongAxis = glm::vec3(0.0F);

    switch (axis) {
    case (GizmoAxis::X): {
        normalForPlaneAlongAxis = glm::vec3(0.0F, 1.0F, 0.0F);
        break;
    }
    case (GizmoAxis::Y): {
        normalForPlaneAlongAxis = glm::vec3(-1.0F, 0.0F, 0.0F);
        break;
    }
    case (GizmoAxis::Z): {
        normalForPlaneAlongAxis = glm::vec3(1.0F, 0.0F, 0.0F);
        break;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
        break;
    }
    }

    Plane planeAlongAxis(normalForPlaneAlongAxis, gizmoOriginalLocation);
    const float rayLength = MathHelpers::calculateRayPlaneIntersection(
        optCursorWorldInfo->worldLocation, optCursorWorldInfo->worldDirection, planeAlongAxis);
    if (rayLength < 0.0F) {
        return {};
    }
    const glm::vec3 toHitOnGizmo =
        optCursorWorldInfo->worldLocation + optCursorWorldInfo->worldDirection * rayLength;
    const glm::vec3 offsetVector = toHitOnGizmo - gizmoOriginalLocation;

    switch (axis) {
    case (GizmoAxis::X): {
        return offsetVector.x;
        break;
    }
    case (GizmoAxis::Y): {
        return offsetVector.y;
        break;
    }
    case (GizmoAxis::Z): {
        return offsetVector.z;
        break;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
        break;
    }
    }

    return {};
}

void GizmoNode::trackMouseMovement(GizmoAxis axis) {
    const auto optOffset = calculateOffsetFromGizmoToCursorRay(getWorldLocation(), axis);
    if (!optOffset.has_value()) {
        return;
    }

    optTrackingInfo = TrackingInfo{
        .axis = axis,
        .originalRelativePos = pControlledNode->getRelativeLocation(),
        .originalWorldPos = pControlledNode->getWorldLocation(),
        .offsetToGizmoPivot = *optOffset};
}

void GizmoNode::stopTrackingMouseMovement() {
    if (!optTrackingInfo.has_value()) {
        return;
    }

    const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
    if (pGameInstance == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected editor game instance");
    }

    if (pGameInstance->getPropertyInspector()->getInspectedNode() == pControlledNode) {
        pGameInstance->getPropertyInspector()->refreshInspectedProperties();
    }

    optTrackingInfo = {};
}

void GizmoNode::onMouseMove(double xOffset, double yOffset) {
    SpatialNode::onMouseMove(xOffset, yOffset);

    if (!optTrackingInfo.has_value()) {
        // Not tracking.
        return;
    }

    const auto optNewOffsetToGizmoPivot =
        calculateOffsetFromGizmoToCursorRay(optTrackingInfo->originalWorldPos, optTrackingInfo->axis);
    if (!optNewOffsetToGizmoPivot.has_value()) {
        return;
    }

    switch (mode) {
    case (GizmoMode::MOVE): {
        glm::vec3 offsetDiff = glm::vec3(0.0F);
        switch (optTrackingInfo->axis) {
        case (GizmoAxis::X): {
            offsetDiff.x = *optNewOffsetToGizmoPivot - optTrackingInfo->offsetToGizmoPivot;
            break;
        }
        case (GizmoAxis::Y): {
            offsetDiff.y = *optNewOffsetToGizmoPivot - optTrackingInfo->offsetToGizmoPivot;
            break;
        }
        case (GizmoAxis::Z): {
            offsetDiff.z = *optNewOffsetToGizmoPivot - optTrackingInfo->offsetToGizmoPivot;
            break;
        }
        default: {
            Error::showErrorAndThrowException("unhandled case");
        }
        }
        pControlledNode->setRelativeLocation(optTrackingInfo->originalRelativePos + offsetDiff);
        break;
    }
    case (GizmoMode::ROTATE): {
        Error::showErrorAndThrowException("not implemented");
        break;
    }
    case (GizmoMode::SCALE): {
        Error::showErrorAndThrowException("not implemented");
        break;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
    }
    }
}
