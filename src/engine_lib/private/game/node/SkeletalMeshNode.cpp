#include "game/node/SkeletalMeshNode.h"

// Custom.
#include "render/GpuResourceManager.h"
#include "game/GameInstance.h"
#include "game/geometry/PrimitiveMeshGenerator.h"
#include "render/wrapper/ShaderProgram.h"
#include "game/World.h"
#include "render/MeshNodeManager.h"
#include "game/node/SkeletonNode.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "548ce3b1-a484-40e2-8f2f-fdc70ea8d26f";
}

std::string SkeletalMeshNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string SkeletalMeshNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo SkeletalMeshNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.skeletalMeshNodeGeometries[NAMEOF_MEMBER(&SkeletalMeshNode::skeletalMeshGeometry).data()] =
        ReflectedVariableInfo<SkeletalMeshNodeGeometry>{
            .setter =
                [](Serializable* pThis, const SkeletalMeshNodeGeometry& newValue) {
                    reinterpret_cast<SkeletalMeshNode*>(pThis)->setSkeletalMeshGeometryBeforeSpawned(
                        newValue);
                },
            .getter = [](Serializable* pThis) -> SkeletalMeshNodeGeometry {
                return reinterpret_cast<SkeletalMeshNode*>(pThis)->copySkeletalMeshData();
            }};

    return TypeReflectionInfo(
        MeshNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SkeletalMeshNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SkeletalMeshNode>(); },
        std::move(variables));
}

SkeletalMeshNode::SkeletalMeshNode() : SkeletalMeshNode("Skeletal Mesh Node") {}

SkeletalMeshNode::SkeletalMeshNode(const std::string& sNodeName) : MeshNode(sNodeName) {
    clearMeshNodeGeometry();
}

std::string_view SkeletalMeshNode::getPathToDefaultVertexShader() {
    // MeshNode::getPathToDefaultVertexShader(); // <- commended out to silence our node super call checker

    return "engine/shaders/node/SkeletalMeshNode.vert.glsl";
}

bool SkeletalMeshNode::isUsingSkeletalMeshGeometry() {
    // MeshNode::isUsingSkeletalMeshGeometry(); // <- commended out to silence our node super call checker

    return true;
}

std::unique_ptr<VertexArrayObject> SkeletalMeshNode::createVertexArrayObject() {
    // MeshNode::createVertexArrayObject(); // <- commended out to silence our node super call checker

    if (skeletalMeshGeometry.getVertices().empty() || skeletalMeshGeometry.getIndices().empty())
        [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected node \"{}\" geometry to be not empty", getNodeName()));
    }

    return GpuResourceManager::createVertexArrayObject(skeletalMeshGeometry);
}

void SkeletalMeshNode::setSkeletalMeshGeometryBeforeSpawned(const SkeletalMeshNodeGeometry& meshGeometry) {
    std::scoped_lock guard(getSpawnDespawnMutex());

    // For simplicity we don't allow changing geometry while spawned.
    if (isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "changing geometry of a spawned node is not allowed, if you need procedural/dynamic geometry "
            "consider passing some additional data to the vertex shader and changing vertices there "
            "(node \"{}\")",
            getNodeName()));
    }

    this->skeletalMeshGeometry = meshGeometry;
}

void SkeletalMeshNode::setSkeletalMeshGeometryBeforeSpawned(SkeletalMeshNodeGeometry&& meshGeometry) {
    std::scoped_lock guard(getSpawnDespawnMutex());

    // For simplicity we don't allow changing geometry while spawned.
    if (isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "changing geometry of a spawned node is not allowed, if you need procedural/dynamic geometry "
            "consider passing some additional data to the vertex shader and changing vertices there "
            "(node \"{}\")",
            getNodeName()));
    }

    this->skeletalMeshGeometry = std::move(meshGeometry);
}

void SkeletalMeshNode::onAfterDeserialized() {
    MeshNode::onAfterDeserialized();

    clearMeshNodeGeometry();
}

void SkeletalMeshNode::onSpawning() {
    MeshNode::onSpawning();

    {
        const auto mtxParentNode = getParentNode();
        std::scoped_lock parentGuard(*mtxParentNode.first);

        pSpawnedSkeleton = dynamic_cast<SkeletonNode*>(mtxParentNode.second);
        if (pSpawnedSkeleton == nullptr) {
            Log::warn(std::format(
                "node \"{}\" expects a SkeletonNode to be a direct parent node in order for animations to "
                "work",
                getNodeName()));
            return;
        }

#if defined(DEBUG)
        // Make sure our per-vertex bone indices won't reference bones out of bounds for the skeleton.
        const auto iBoneCount = pSpawnedSkeleton->getSkinningMatrices().size();
        for (const auto& vertex : skeletalMeshGeometry.getVertices()) {
            for (SkeletalMeshNodeVertex::BoneIndexType iBoneIndex : vertex.vBoneIndices) {
                if (iBoneIndex >= iBoneCount) [[unlikely]] {
                    Log::error(std::format(
                        "skeletal mesh node \"{}\" has vertices that reference bone with index {} but parent "
                        "skeleton node only has {} bones (index out of bounds - incompatible skeleton)",
                        getNodeName(),
                        iBoneIndex,
                        iBoneCount));
                    pSpawnedSkeleton = nullptr;
                    return;
                }
            }
        }
#endif
    }

    getShaderConstantsSetterWhileSpawned().addSetterFunction([this](ShaderProgram* pShaderProgram) {
        const auto& matrices = pSpawnedSkeleton->getSkinningMatrices();
        pShaderProgram->setMatrix4ArrayToShader(
            "vSkinningMatrices[0]", static_cast<int>(matrices.size()), glm::value_ptr(matrices[0]));
    });
}

void SkeletalMeshNode::onDespawning() {
    MeshNode::onDespawning();

    pSpawnedSkeleton = nullptr;
}

void SkeletalMeshNode::onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {
    MeshNode::onAfterAttachedToNewParent(bThisNodeBeingAttached);

    if (!isSpawned()) {
        return;
    }

    if (!bThisNodeBeingAttached) {
        return;
    }

    const auto mtxParentNode = getParentNode();
    std::scoped_lock parentGuard(*mtxParentNode.first);

    pSpawnedSkeleton = dynamic_cast<SkeletonNode*>(mtxParentNode.second);
    if (pSpawnedSkeleton == nullptr) {
        Log::warn(std::format(
            "node \"{}\" expects a SkeletonNode to be a direct parent node in order for animations to work",
            getNodeName()));
    }
}
