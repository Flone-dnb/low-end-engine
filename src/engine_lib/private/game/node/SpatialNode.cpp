#include "game/node/SpatialNode.h"

// Custom.
#include "math/MathHelpers.hpp"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "ac1356e14-2d2f-4c64-9e05-d6b632d9f6b7";
}

std::string SpatialNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string SpatialNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo SpatialNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec3s[NAMEOF_MEMBER(&SpatialNode::relativeLocation).data()] = ReflectedVariableInfo<glm::vec3>{
        .setter =
            [](Serializable* pThis, const glm::vec3& newValue) {
                reinterpret_cast<SpatialNode*>(pThis)->setRelativeLocation(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec3 {
            return reinterpret_cast<SpatialNode*>(pThis)->getRelativeLocation();
        }};

    variables.vec3s[NAMEOF_MEMBER(&SpatialNode::relativeRotation).data()] = ReflectedVariableInfo<glm::vec3>{
        .setter =
            [](Serializable* pThis, const glm::vec3& newValue) {
                reinterpret_cast<SpatialNode*>(pThis)->setRelativeRotation(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec3 {
            return reinterpret_cast<SpatialNode*>(pThis)->getRelativeRotation();
        }};

    variables.vec3s[NAMEOF_MEMBER(&SpatialNode::relativeScale).data()] = ReflectedVariableInfo<glm::vec3>{
        .setter =
            [](Serializable* pThis, const glm::vec3& newValue) {
                reinterpret_cast<SpatialNode*>(pThis)->setRelativeScale(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec3 {
            return reinterpret_cast<SpatialNode*>(pThis)->getRelativeScale();
        }};

    return TypeReflectionInfo(
        Node::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SpatialNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SpatialNode>(); },
        std::move(variables));
}

SpatialNode::SpatialNode() : SpatialNode("Spatial Node") {}

SpatialNode::SpatialNode(const std::string& sNodeName) : Node(sNodeName) {
    mtxSpatialParent.second = nullptr;
}

void SpatialNode::setRelativeLocation(const glm::vec3& location) {
    std::scoped_lock guard(mtxWorldMatrix.first);

    relativeLocation = location;

    recalculateLocalMatrix();
    recalculateWorldMatrix();
}

void SpatialNode::setRelativeRotation(const glm::vec3& rotation) {
    std::scoped_lock guard(mtxWorldMatrix.first);

    relativeRotation.x = MathHelpers::normalizeToRange(rotation.x, -360.0F, 360.0F); // NOLINT
    relativeRotation.y = MathHelpers::normalizeToRange(rotation.y, -360.0F, 360.0F); // NOLINT
    relativeRotation.z = MathHelpers::normalizeToRange(rotation.z, -360.0F, 360.0F); // NOLINT

    recalculateLocalMatrix();
    recalculateWorldMatrix();
}

void SpatialNode::setRelativeScale(const glm::vec3& scale) {
#if defined(DEBUG)
    // Make sure we don't have negative scale specified.
    if (scale.x < 0.0F || scale.y < 0.0F || scale.z < 0.0F) [[unlikely]] {
        Logger::get().warn("avoid using negative scale as it may cause issues");
    }
#endif

    std::scoped_lock guard(mtxWorldMatrix.first);

    relativeScale = scale;

    recalculateLocalMatrix();
    recalculateWorldMatrix();
}

glm::vec3 SpatialNode::getWorldLocation() {
    std::scoped_lock guard(mtxWorldMatrix.first);
    return mtxWorldMatrix.second.worldLocation;
}

glm::vec3 SpatialNode::getWorldRotation() {
    std::scoped_lock guard(mtxWorldMatrix.first);
    return mtxWorldMatrix.second.worldRotation;
}

glm::quat SpatialNode::getWorldRotationQuaternion() {
    std::scoped_lock guard(mtxWorldMatrix.first);
    return mtxWorldMatrix.second.worldRotationQuaternion;
}

glm::vec3 SpatialNode::getWorldScale() {
    std::scoped_lock guard(mtxWorldMatrix.first);
    return mtxWorldMatrix.second.worldScale;
}

glm::vec3 SpatialNode::getWorldForwardDirection() {
    std::scoped_lock guard(mtxWorldMatrix.first);
    return mtxWorldMatrix.second.worldForward;
}

glm::vec3 SpatialNode::getWorldRightDirection() {
    std::scoped_lock guard(mtxWorldMatrix.first);
    return mtxWorldMatrix.second.worldRight;
}

glm::vec3 SpatialNode::getWorldUpDirection() {
    std::scoped_lock guard(mtxWorldMatrix.first);
    return mtxWorldMatrix.second.worldUp;
}

void SpatialNode::setWorldLocation(const glm::vec3& location) {
    std::scoped_lock guard(mtxWorldMatrix.first, mtxSpatialParent.first);

    // See if we have a parent.
    if (mtxSpatialParent.second != nullptr) {
        // Get parent location/rotation/scale.
        const auto parentLocation = mtxSpatialParent.second->getWorldLocation();
        const auto parentRotationQuat = mtxSpatialParent.second->getWorldRotationQuaternion();
        const auto parentScale = mtxSpatialParent.second->getWorldScale();

        // Calculate inverted transformation.
        const auto invertedTranslation = location - parentLocation;
        const auto invertedRotatedTranslation = glm::inverse(parentRotationQuat) * invertedTranslation;
        const auto invertedScale = MathHelpers::calculateReciprocalVector(parentScale);

        // Calculate relative location.
        relativeLocation = invertedRotatedTranslation * invertedScale;
    } else {
        relativeLocation = location;
    }

    recalculateLocalMatrix();
    recalculateWorldMatrix();
}

void SpatialNode::setWorldRotation(const glm::vec3& rotation) {
    glm::vec3 targetWorldRotation;
    targetWorldRotation.x = MathHelpers::normalizeToRange(rotation.x, -360.0F, 360.0F); // NOLINT
    targetWorldRotation.y = MathHelpers::normalizeToRange(rotation.y, -360.0F, 360.0F); // NOLINT
    targetWorldRotation.z = MathHelpers::normalizeToRange(rotation.z, -360.0F, 360.0F); // NOLINT

    std::scoped_lock guard(mtxWorldMatrix.first, mtxSpatialParent.first);

    // See if we have a parent.
    if (mtxSpatialParent.second != nullptr) {
        // Don't care for negative scale (mirrors rotations) because it's rarely used
        // and we warn about it.
        const auto inverseParentQuat = glm::inverse(mtxSpatialParent.second->getWorldRotationQuaternion());
        const auto rotationQuat = glm::toQuat(MathHelpers::buildRotationMatrix(targetWorldRotation));

        relativeRotation = glm::degrees(glm::eulerAngles(inverseParentQuat * rotationQuat));
    } else {
        relativeRotation = targetWorldRotation;
    }

    recalculateLocalMatrix();
    recalculateWorldMatrix();
}

void SpatialNode::setWorldScale(const glm::vec3& scale) {
#if defined(DEBUG)
    // Make sure we don't have negative scale specified.
    if (scale.x < 0.0F || scale.y < 0.0F || scale.z < 0.0F) [[unlikely]] {
        Logger::get().warn("avoid using negative scale as it's not supported and may cause issues");
    }
#endif

    std::scoped_lock guard(mtxWorldMatrix.first, mtxSpatialParent.first);

    // See if we have a parent.
    if (mtxSpatialParent.second != nullptr) {
        // Get parent scale.
        const auto parentScale = mtxSpatialParent.second->getWorldScale();

        relativeScale = scale * MathHelpers::calculateReciprocalVector(parentScale);
    } else {
        relativeScale = scale;
    }

    recalculateLocalMatrix();
    recalculateWorldMatrix();
}

void SpatialNode::onSpawning() {
    PROFILE_FUNC

    Node::onSpawning();

    // No need to notify child nodes since this function is called before any of
    // the child nodes are spawned.
    recalculateWorldMatrix(false);
}

glm::mat4x4 SpatialNode::getWorldMatrix() {
    std::scoped_lock guard(mtxWorldMatrix.first);
    return mtxWorldMatrix.second.worldMatrix;
}

void SpatialNode::recalculateWorldMatrix(bool bNotifyChildren) {
    PROFILE_FUNC

    // Prepare some variables.
    glm::mat4x4 parentWorldMatrix = glm::identity<glm::mat4x4>();
    glm::quat parentWorldRotationQuat = glm::identity<glm::quat>();
    glm::vec3 parentWorldScale = glm::vec3(1.0F, 1.0F, 1.0F);

    std::scoped_lock guard(mtxWorldMatrix.first, mtxLocalSpace.first);

    {
        // See if there is a spatial node in the parent chain.
        std::scoped_lock spatialParentGuard(mtxSpatialParent.first);
        if (mtxSpatialParent.second != nullptr) {
            // Save parent's world information.
            parentWorldMatrix = mtxSpatialParent.second->getWorldMatrix();
            parentWorldRotationQuat = mtxSpatialParent.second->getWorldRotationQuaternion();
            parentWorldScale = mtxSpatialParent.second->getWorldScale();
        }
    }

    // Calculate world matrix without counting the parent.
    const auto myWorldMatrix = glm::translate(relativeLocation) *
                               mtxLocalSpace.second.relativeRotationMatrix * glm::scale(relativeScale);

    // Recalculate world matrix.
    mtxWorldMatrix.second.worldMatrix = parentWorldMatrix * myWorldMatrix;

    // Save world location.
    mtxWorldMatrix.second.worldLocation =
        parentWorldMatrix *
        glm::vec4(relativeLocation, 1.0F); // don't apply relative rotation/scale to world location

    // Save world rotation.
    mtxWorldMatrix.second.worldRotationQuaternion =
        parentWorldRotationQuat * mtxLocalSpace.second.relativeRotationQuaternion;
    mtxWorldMatrix.second.worldRotation =
        glm::degrees(glm::eulerAngles(mtxWorldMatrix.second.worldRotationQuaternion));

    // Save world scale.
    mtxWorldMatrix.second.worldScale = parentWorldScale * relativeScale;

    // Calculate world forward direction.
    mtxWorldMatrix.second.worldForward =
        glm::normalize(mtxWorldMatrix.second.worldMatrix * glm::vec4(Globals::WorldDirection::forward, 0.0F));

    // Calculate world right direction.
    mtxWorldMatrix.second.worldRight =
        glm::normalize(mtxWorldMatrix.second.worldMatrix * glm::vec4(Globals::WorldDirection::right, 0.0F));

    // Calculate world up direction.
    mtxWorldMatrix.second.worldUp =
        glm::cross(mtxWorldMatrix.second.worldRight, mtxWorldMatrix.second.worldForward);

    if (mtxWorldMatrix.second.bInOnWorldLocationRotationScaleChanged) {
        // We came here from a `onWorldLocationRotationScaleChanged` call, stop recursion and
        // don't notify children as it will be done when this `onWorldLocationRotationScaleChanged` call
        // will be finished.
        return;
    }

    mtxWorldMatrix.second.bInOnWorldLocationRotationScaleChanged = true;
    onWorldLocationRotationScaleChanged();
    mtxWorldMatrix.second.bInOnWorldLocationRotationScaleChanged = false;

    if (bNotifyChildren) {
        // Notify spatial child nodes.
        // (don't unlock our world matrix yet)
        const auto mtxChildNodes = getChildNodes();
        std::scoped_lock childNodesGuard(*mtxChildNodes.first);
        for (const auto& pNode : mtxChildNodes.second) {
            recalculateWorldMatrixForNodeAndNotifyChildren(pNode);
        }
    }
}

void SpatialNode::recalculateWorldMatrixForNodeAndNotifyChildren(Node* pNode) {
    PROFILE_FUNC

    const auto pSpatialNode = dynamic_cast<SpatialNode*>(pNode);
    if (pSpatialNode != nullptr) {
        pSpatialNode->recalculateWorldMatrix(); // recalculates for its children
        return;
    }

    assert(pNode != nullptr); // silence clang-tidy's false positive warning

    // This is not a spatial node, notify children maybe there's a spatial node somewhere.
    const auto mtxChildNodes = pNode->getChildNodes();
    std::scoped_lock childNodesGuard(*mtxChildNodes.first);
    for (const auto& pNode : mtxChildNodes.second) {
        recalculateWorldMatrixForNodeAndNotifyChildren(pNode);
    }
}

void SpatialNode::onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {
    Node::onAfterAttachedToNewParent(bThisNodeBeingAttached);

    // Find a spatial node in the parent chain and save it.
    std::scoped_lock spatialParentGuard(mtxSpatialParent.first);

    mtxSpatialParent.second = getParentNodeOfType<SpatialNode>();

    // No need to notify child nodes since this function (on after attached)
    // will be also called on all child nodes.
    recalculateWorldMatrix(false);
}

void SpatialNode::recalculateLocalMatrix() {
    PROFILE_FUNC

    std::scoped_lock guard(mtxLocalSpace.first);

    mtxLocalSpace.second.relativeRotationMatrix = MathHelpers::buildRotationMatrix(relativeRotation);
    mtxLocalSpace.second.relativeRotationQuaternion =
        glm::toQuat(mtxLocalSpace.second.relativeRotationMatrix);
}

glm::mat4x4 SpatialNode::getRelativeRotationMatrix() {
    std::scoped_lock guard(mtxLocalSpace.first);
    return mtxLocalSpace.second.relativeRotationMatrix;
}

std::pair<std::recursive_mutex, SpatialNode*>& SpatialNode::getClosestSpatialParent() {
    return mtxSpatialParent;
}

void SpatialNode::onAfterDeserialized() {
    Node::onAfterDeserialized();

    recalculateLocalMatrix();

    // No need to notify children here because:
    // 1. If this is a node tree that is being deserialized, child nodes will be added
    // after this function is finished, once a child node is added it will recalculate its matrix.
    // 2. If this is a single node that is being deserialized, there are no children.
    recalculateWorldMatrix(false);
}

void SpatialNode::applyAttachmentRule(
    Node::AttachmentRule locationRule,
    const glm::vec3& worldLocationBeforeAttachment,
    Node::AttachmentRule rotationRule,
    const glm::vec3& worldRotationBeforeAttachment,
    Node::AttachmentRule scaleRule,
    const glm::vec3& worldScaleBeforeAttachment) {
    // Apply location rule.
    switch (locationRule) {
    case (AttachmentRule::KEEP_RELATIVE): {
        // Do nothing.
        break;
    }
    case (AttachmentRule::KEEP_WORLD): {
        setWorldLocation(worldLocationBeforeAttachment);
        break;
    }
    }

    // Apply rotation rule.
    switch (rotationRule) {
    case (AttachmentRule::KEEP_RELATIVE): {
        // Do nothing.
        break;
    }
    case (AttachmentRule::KEEP_WORLD): {
        setWorldRotation(worldRotationBeforeAttachment);
        break;
    }
    }

    // Apply scale rule.
    switch (scaleRule) {
    case (AttachmentRule::KEEP_RELATIVE): {
        // Do nothing.
        break;
    }
    case (AttachmentRule::KEEP_WORLD): {
        setWorldScale(worldScaleBeforeAttachment);
        break;
    }
    }
}
