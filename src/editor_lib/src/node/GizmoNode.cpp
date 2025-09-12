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

    // Prepare path to gizmo model.
    std::string_view sGizmoModelName;
    switch (mode) {
    case (GizmoMode::MOVE): {
        sGizmoModelName = "gizmo_move";
        break;
    }
    case (GizmoMode::ROTATE): {
        sGizmoModelName = "gizmo_rotate";
        break;
    }
    case (GizmoMode::SCALE): {
        sGizmoModelName = "gizmo_scale";
        break;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
    }
    }

    // Deserialize model.
    auto result = Node::deserializeNodeTree(
        EditorResourcePaths::getPathToModelsDirectory() / sGizmoModelName / sGizmoModelName);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        error.showErrorAndThrowException();
    }
    auto pXAxisGizmoU = std::get<std::unique_ptr<Node>>(std::move(result));

    const auto pXAxisMesh = dynamic_cast<MeshNode*>(pXAxisGizmoU.get());
    if (pXAxisMesh == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected a mesh node");
    }
    if (!pXAxisMesh->getChildNodes().second.empty()) [[unlikely]] {
        Error::showErrorAndThrowException("expected a single mesh node");
    }

    pXAxisMesh->setNodeName("Gizmo X");

    MeshNodeGeometry gizmoGeometry = pXAxisMesh->copyMeshData();

    auto pYAxisMeshU = std::make_unique<MeshNode>("Gizmo Y");
    pYAxisMeshU->setMeshGeometryBeforeSpawned(gizmoGeometry);
    pYAxisMeshU->setRelativeRotation(glm::vec3(0.0F, 0.0F, 90.0F));

    auto pZAxisMeshU = std::make_unique<MeshNode>("Gizmo Z");
    pZAxisMeshU->setMeshGeometryBeforeSpawned(std::move(gizmoGeometry));
    pZAxisMeshU->setRelativeRotation(glm::vec3(0.0F, -90.0F, 0.0F));

    // Set color.
    pXAxisMesh->getMaterial().setDiffuseColor(glm::vec3(1.0F, 0.0F, 0.0F));
    pYAxisMeshU->getMaterial().setDiffuseColor(glm::vec3(0.0F, 1.0F, 0.0F));
    pZAxisMeshU->getMaterial().setDiffuseColor(glm::vec3(0.0F, 0.0F, 1.0F));

    // Save pointers.
    pXAxisGizmoNode = pXAxisMesh;
    pYAxisGizmoNode = pYAxisMeshU.get();
    pZAxisGizmoNode = pZAxisMeshU.get();

    // Configure meshes.
    pXAxisGizmoNode->setSerialize(false);
    pYAxisGizmoNode->setSerialize(false);
    pZAxisGizmoNode->setSerialize(false);

    pXAxisGizmoNode->setDrawLayer(MeshDrawLayer::LAYER2);
    pYAxisGizmoNode->setDrawLayer(MeshDrawLayer::LAYER2);
    pZAxisGizmoNode->setDrawLayer(MeshDrawLayer::LAYER2);

    pXAxisGizmoNode->setIsAffectedByLightSources(false);
    pYAxisGizmoNode->setIsAffectedByLightSources(false);
    pZAxisGizmoNode->setIsAffectedByLightSources(false);

    addChildNode(std::move(pXAxisGizmoU));
    addChildNode(std::move(pYAxisMeshU));
    addChildNode(std::move(pZAxisMeshU));

    // Add usage hint.
    {
        const auto pUsageHintText = addChildNode(std::make_unique<TextUiNode>());
        pUsageHintText->setSerialize(false);

        pUsageHintText->setPosition(glm::vec2(0.6F, 0.01F));
        pUsageHintText->setTextHeight(0.025F);
        pUsageHintText->setSize(
            glm::vec2(1.0F - pUsageHintText->getPosition().x, pUsageHintText->getTextHeight() * 1.25F));
        pUsageHintText->setText(u"gizmo usage (keyboard): W - move, E - rotate, R - scale");
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
    default: {
        Error::showErrorAndThrowException("unhandled case");
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

    if (mode == GizmoMode::ROTATE) {
        switch (axis) {
        case (GizmoAxis::X): {
            normalForPlaneAlongAxis = glm::vec3(1.0F, 0.0F, 0.0F);
            break;
        }
        case (GizmoAxis::Y): {
            normalForPlaneAlongAxis = glm::vec3(0.0F, 1.0F, 0.0F);
            break;
        }
        case (GizmoAxis::Z): {
            normalForPlaneAlongAxis = glm::vec3(0.0F, 0.0F, 1.0F);
            break;
        }
        default: {
            Error::showErrorAndThrowException("unhandled case");
            break;
        }
        }
    } else {
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

    if (mode == GizmoMode::ROTATE) {
        switch (axis) {
        case (GizmoAxis::X): {
            return -offsetVector.y;
            break;
        }
        case (GizmoAxis::Y): {
            return offsetVector.x;
            break;
        }
        case (GizmoAxis::Z): {
            return -offsetVector.y;
            break;
        }
        default: {
            Error::showErrorAndThrowException("unhandled case");
            break;
        }
        }
    } else {
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
    }

    return {};
}

void GizmoNode::onSpawning() {
    SpatialNode::onSpawning();

    setWorldLocation(pControlledNode->getWorldLocation());

    if (mode == GizmoMode::ROTATE || mode == GizmoMode::SCALE) {
        setWorldRotation(pControlledNode->getWorldRotation());
    }
}

void GizmoNode::trackMouseMovement(GizmoAxis axis) {
    const auto optOffset = calculateOffsetFromGizmoToCursorRay(getWorldLocation(), axis);
    if (!optOffset.has_value()) {
        return;
    }

    glm::vec3 originalRelativeTransform;
    switch (mode) {
    case (GizmoMode::MOVE): {
        originalRelativeTransform = pControlledNode->getRelativeLocation();
        break;
    }
    case (GizmoMode::ROTATE): {
        originalRelativeTransform = pControlledNode->getRelativeRotation();
        break;
    }
    case (GizmoMode::SCALE): {
        originalRelativeTransform = pControlledNode->getRelativeScale();
        break;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
    }
    }

    optTrackingInfo = TrackingInfo{
        .axis = axis,
        .originalRelativeTransform = originalRelativeTransform,
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

    // Calculate mouse movement diff.
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

    // Apply change.
    switch (mode) {
    case (GizmoMode::MOVE): {
        pControlledNode->setRelativeLocation(optTrackingInfo->originalRelativeTransform + offsetDiff);
        setWorldLocation(pControlledNode->getWorldLocation());
        break;
    }
    case (GizmoMode::ROTATE): {
        pControlledNode->setRelativeRotation(
            optTrackingInfo->originalRelativeTransform + offsetDiff * 10.0F); // make rotation more sensitive
        setWorldRotation(pControlledNode->getWorldRotation());
        break;
    }
    case (GizmoMode::SCALE): {
        auto newScale = optTrackingInfo->originalRelativeTransform + offsetDiff;
        newScale = glm::max(newScale, glm::vec3(0.01F, 0.01F, 0.01F)); // avoid negative scale
        pControlledNode->setRelativeScale(newScale);
        break;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
    }
    }
}
