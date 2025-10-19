#include "game/node/SkeletonBoneAttachmentNode.h"

// Custom.
#include "misc/Profiler.hpp"
#include "game/node/SkeletonNode.h"
#include "io/Log.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "903689d9-7fdc-4ce8-a21e-95d11e1b6abf";
}

std::string SkeletonBoneAttachmentNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string SkeletonBoneAttachmentNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo SkeletonBoneAttachmentNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.unsignedInts[NAMEOF_MEMBER(&SkeletonBoneAttachmentNode::iBoneIndex).data()] =
        ReflectedVariableInfo<unsigned int>{
            .setter =
                [](Serializable* pThis, const unsigned int& iNewValue) {
                    reinterpret_cast<SkeletonBoneAttachmentNode*>(pThis)->setBoneIndex(iNewValue);
                },
            .getter = [](Serializable* pThis) -> unsigned int {
                return reinterpret_cast<SkeletonBoneAttachmentNode*>(pThis)->getBoneIndex();
            }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SkeletonBoneAttachmentNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SkeletonBoneAttachmentNode>(); },
        std::move(variables));
}

SkeletonBoneAttachmentNode::SkeletonBoneAttachmentNode()
    : SkeletonBoneAttachmentNode("Skeleton Bone Attachment Node") {}

SkeletonBoneAttachmentNode::SkeletonBoneAttachmentNode(const std::string& sNodeName)
    : SpatialNode(sNodeName) {
    setIsCalledEveryFrame(true);
    setTickGroup(TickGroup::SECOND); // skeleton node first, then this node second
}

void SkeletonBoneAttachmentNode::setBoneIndex(unsigned int iNewBoneIndex) { iBoneIndex = iNewBoneIndex; }

void SkeletonBoneAttachmentNode::onSpawning() {
    SpatialNode::onSpawning();

    const auto mtxParent = getParentNode();
    std::scoped_lock guard(*mtxParent.first);

    const auto pSkeletonNode = dynamic_cast<SkeletonNode*>(mtxParent.second);
    if (pSkeletonNode == nullptr) {
        Log::warn(std::format(
            "skeleton bone attachment node \"{}\" must be a child node of the skeleton node, otherwise the "
            "node will do nothing",
            getNodeName()));
        return;
    }
}

void SkeletonBoneAttachmentNode::onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {
    SpatialNode::onAfterAttachedToNewParent(bThisNodeBeingAttached);

    if (!bThisNodeBeingAttached) {
        return;
    }

    const auto mtxParent = getParentNode();
    std::scoped_lock guard(*mtxParent.first, mtxCachedSkeletonParent.first);

    const auto pSkeletonNode = dynamic_cast<SkeletonNode*>(mtxParent.second);
    if (pSkeletonNode == nullptr) {
        Log::warn(std::format(
            "skeleton bone attachment node \"{}\" must be a child node of the skeleton node, otherwise the "
            "node will do nothing",
            getNodeName()));
        return;
    }

    mtxCachedSkeletonParent.second = pSkeletonNode;
}

void SkeletonBoneAttachmentNode::onBeforeNewFrame(float timeSincePrevFrameInSec) {
    PROFILE_FUNC

    SpatialNode::onBeforeNewFrame(timeSincePrevFrameInSec);

    std::scoped_lock guard(mtxCachedSkeletonParent.first);
    if (mtxCachedSkeletonParent.second == nullptr) {
        return;
    }

    const auto& boneMatrices = mtxCachedSkeletonParent.second->getModelBoneMatrices();
    if (boneMatrices.empty()) {
        return;
    }

    const auto& boneOzzMatrix =
        boneMatrices[std::min(static_cast<size_t>(iBoneIndex), boneMatrices.size() - 1)];
    glm::mat4 boneMatrix = glm::identity<glm::mat4>();
    for (int k = 0; k < 4; k++) {
        ozz::math::StorePtr(boneOzzMatrix.cols[k], &boneMatrix[k].x);
    }

    glm::vec3 scale{};
    glm::quat rotation{};
    glm::vec3 translation{};
    glm::vec3 skew{};
    glm::vec4 perspective{};
    glm::decompose(boneMatrix, scale, rotation, translation, skew, perspective);

    setRelativeLocation(translation);
    setRelativeRotation(glm::degrees(glm::eulerAngles(rotation)));
}
