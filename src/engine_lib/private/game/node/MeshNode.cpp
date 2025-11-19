#include "game/node/MeshNode.h"

// Custom.
#include "render/GpuResourceManager.h"
#include "game/GameInstance.h"
#include "game/geometry/PrimitiveMeshGenerator.h"
#include "render/wrapper/ShaderProgram.h"
#include "game/World.h"
#include "render/MeshRenderer.h"
#include "render/RenderingHandle.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "bea29d45-274d-4a50-91ec-8ca09897ce4d";
}

std::string MeshNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string MeshNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo MeshNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.bools[NAMEOF_MEMBER(&MeshNode::bIsVisible).data()] = ReflectedVariableInfo<bool>{
        .setter = [](Serializable* pThis,
                     const bool& bNewValue) { reinterpret_cast<MeshNode*>(pThis)->setIsVisible(bNewValue); },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<MeshNode*>(pThis)->isVisible();
        }};

    variables.vec4s["materialDiffuseColor"] = ReflectedVariableInfo<glm::vec4>{
        .setter =
            [](Serializable* pThis, const glm::vec4& newValue) {
                auto& material = reinterpret_cast<MeshNode*>(pThis)->getMaterial();
                material.setDiffuseColor(newValue);
                material.setOpacity(newValue.w);
            },
        .getter = [](Serializable* pThis) -> glm::vec4 {
            auto& material = reinterpret_cast<MeshNode*>(pThis)->getMaterial();
            return glm::vec4(material.getDiffuseColor(), material.getOpacity());
        }};

    variables.strings["materialDiffuseTexture"] = ReflectedVariableInfo<std::string>{
        .setter =
            [](Serializable* pThis, const std::string& sNewValue) {
                reinterpret_cast<MeshNode*>(pThis)->getMaterial().setPathToDiffuseTexture(sNewValue);
            },
        .getter = [](Serializable* pThis) -> std::string {
            return reinterpret_cast<MeshNode*>(pThis)->getMaterial().getPathToDiffuseTexture();
        }};

    variables.vec2s["materialTextureTilingMultiplier"] = ReflectedVariableInfo<glm::vec2>{
        .setter =
            [](Serializable* pThis, const glm::vec2& newValue) {
                auto& material = reinterpret_cast<MeshNode*>(pThis)->getMaterial();
                material.setTextureTilingMultiplier(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec2 {
            auto& material = reinterpret_cast<MeshNode*>(pThis)->getMaterial();
            return material.getTextureTilingMultiplier();
        }};

    variables.strings["materialCustomVertexShader"] = ReflectedVariableInfo<std::string>{
        .setter =
            [](Serializable* pThis, const std::string& sNewValue) {
                reinterpret_cast<MeshNode*>(pThis)->getMaterial().setPathToCustomVertexShader(sNewValue);
            },
        .getter = [](Serializable* pThis) -> std::string {
            return reinterpret_cast<MeshNode*>(pThis)->getMaterial().getPathToCustomVertexShader();
        }};

    variables.strings["materialCustomFragmentShader"] = ReflectedVariableInfo<std::string>{
        .setter =
            [](Serializable* pThis, const std::string& sNewValue) {
                reinterpret_cast<MeshNode*>(pThis)->getMaterial().setPathToCustomFragmentShader(sNewValue);
            },
        .getter = [](Serializable* pThis) -> std::string {
            return reinterpret_cast<MeshNode*>(pThis)->getMaterial().getPathToCustomFragmentShader();
        }};

    variables.floats["materialOpacity"] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<MeshNode*>(pThis)->getMaterial().setOpacity(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<MeshNode*>(pThis)->getMaterial().getOpacity();
        }};

    variables.bools["materialTransparencyEnabled"] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<MeshNode*>(pThis)->getMaterial().setEnableTransparency(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<MeshNode*>(pThis)->getMaterial().isTransparencyEnabled();
        }};

    variables.meshNodeGeometries[NAMEOF_MEMBER(&MeshNode::meshGeometry).data()] =
        ReflectedVariableInfo<MeshNodeGeometry>{
            .setter =
                [](Serializable* pThis, const MeshNodeGeometry& newValue) {
                    reinterpret_cast<MeshNode*>(pThis)->setMeshGeometryBeforeSpawned(newValue);
                },
            .getter = [](Serializable* pThis) -> MeshNodeGeometry {
                return reinterpret_cast<MeshNode*>(pThis)->copyMeshData();
            }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(MeshNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<MeshNode>(); },
        std::move(variables));
}

MeshNode::MeshNode() : MeshNode("Mesh Node") {}

MeshNode::MeshNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    meshGeometry = PrimitiveMeshGenerator::createCube(1.0F);
}

MeshNode::~MeshNode() {}

void MeshNode::setMaterialBeforeSpawned(Material&& material) {
    std::scoped_lock guard(getSpawnDespawnMutex());

    // For simplicity we don't allow changing material while spawned.
    // Moreover, renderable node manager does not expect us to change material.
    if (isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("changing material of a spawned node is not allowed (node \"{}\")", getNodeName()));
    }

    this->material = std::move(material);
}

void MeshNode::setMeshGeometryBeforeSpawned(const MeshNodeGeometry& meshGeometry) {
    if (isUsingSkeletalMeshGeometry()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "use other function to set geometry because skeletal mesh node uses skeletal "
            "geometry not the usual mesh node geometry, node: {}",
            getNodeName()));
    }

    std::scoped_lock guard(getSpawnDespawnMutex());

    // For simplicity we don't allow changing geometry while spawned.
    if (isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "changing geometry of a spawned node is not allowed, if you need procedural/dynamic geometry "
            "consider passing some additional data to the vertex shader and changing vertices there "
            "(node \"{}\")",
            getNodeName()));
    }

    this->meshGeometry = meshGeometry;
}

void MeshNode::setMeshGeometryBeforeSpawned(MeshNodeGeometry&& meshGeometry) {
    if (isUsingSkeletalMeshGeometry()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "use other function to set geometry because skeletal mesh node uses skeletal "
            "geometry not the usual mesh node geometry, node: {}",
            getNodeName()));
    }

    std::scoped_lock guard(getSpawnDespawnMutex());

    // For simplicity we don't allow changing geometry while spawned.
    if (isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "changing geometry of a spawned node is not allowed, if you need procedural/dynamic geometry "
            "consider passing some additional data to the vertex shader and changing vertices there "
            "(node \"{}\")",
            getNodeName()));
    }

    this->meshGeometry = std::move(meshGeometry);
}

void MeshNode::setIsVisible(bool bNewVisible) {
    std::scoped_lock guard(getSpawnDespawnMutex());

    if (bIsVisible == bNewVisible) {
        return;
    }
    bIsVisible = bNewVisible;

    if (isSpawned()) {
        if (bIsVisible) {
            registerToRendering();
        } else {
            unregisterFromRendering();
        }
    }
}

Material& MeshNode::getMaterial() { return material; }

std::unique_ptr<VertexArrayObject> MeshNode::createVertexArrayObject() {
    // SpatialNode::createVertexArrayObject(); // <- commended out to silence our node super call checker

    if (meshGeometry.getVertices().empty() || meshGeometry.getIndices().empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected node \"{}\" geometry to be not empty", getNodeName()));
    }

    return GpuResourceManager::createVertexArrayObject(meshGeometry);
}

void MeshNode::clearMeshNodeGeometry() { meshGeometry = {}; }

void MeshNode::registerToRendering() {
    PROFILE_FUNC

    if (pRenderingHandle != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("mesh node \"{}\" already created GPU resources", getNodeName()));
    }

    if (!bIsVisible) {
        return;
    }

    // Init render resources.
    material.initShaderProgramAndResources(this, getGameInstanceWhileSpawned()->getRenderer());
    pVao = createVertexArrayObject();

    // After we initialized render resources, add to rendering.
    pRenderingHandle = getWorldWhileSpawned()->getMeshRenderer().addMeshForRendering(
        material.getShaderProgram(), material.isTransparencyEnabled());
    onRenderingHandleInitialized(pRenderingHandle.get());

    // Initialize shader data.
    updateRenderData(true);
}

void MeshNode::updateRenderData(bool bJustRegistered) {
    if (pRenderingHandle == nullptr) {
        return;
    }

    auto renderDataGuard = getWorldWhileSpawned()->getMeshRenderer().getMeshRenderData(*pRenderingHandle);
    auto& data = renderDataGuard.getData();

    if (bJustRegistered) {
        aabbLocal = calculateBoundingBoxFromGeometry();
        data.aabbWorld = aabbLocal.convertToWorldSpace(getWorldMatrix());
    }

    data.worldMatrix = getWorldMatrix();
    data.normalMatrix = glm::transpose(glm::inverse(data.worldMatrix));
    data.diffuseColor = glm::vec4(material.getDiffuseColor(), material.getOpacity());
    data.textureTilingMultiplier = material.getTextureTilingMultiplier();
    data.iDiffuseTextureId = material.getDiffuseTextureId();
    data.iVertexArrayObject = pVao->getVertexArrayObjectId();
    data.iIndexCount = pVao->getIndexCount();
#if defined(ENGINE_EDITOR)
    auto iNodeId = *getNodeId();
    if (iNodeId > std::numeric_limits<unsigned int>::max()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "unable to set node ID to shaders because it reached shader type limit, id: {}, node: {}",
            iNodeId,
            getNodeName()));
    }
    data.iNodeId = static_cast<unsigned int>(iNodeId);
#endif
}

AABB MeshNode::calculateBoundingBoxFromGeometry() {
    auto min = glm::vec3(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max());

    auto max = glm::vec3(
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max());

    for (const auto& vertex : meshGeometry.getVertices()) {
        auto& position = vertex.position;

        min.x = std::min(min.x, position.x);
        min.y = std::min(min.y, position.y);
        min.z = std::min(min.z, position.z);

        max.x = std::max(max.x, position.x);
        max.y = std::max(max.y, position.y);
        max.z = std::max(max.z, position.z);
    }

    AABB aabb{};
    aabb.center = glm::vec3((min.x + max.x) * 0.5F, (min.y + max.y) * 0.5F, (min.z + max.z) * 0.5F);
    aabb.extents = glm::vec3(max.x - aabb.center.x, max.y - aabb.center.y, max.z - aabb.center.z);

    return aabb;
}

void MeshNode::unregisterFromRendering() {
    PROFILE_FUNC

    if (pRenderingHandle == nullptr) {
        return;
    }

    // Remove from rendering.
    pRenderingHandle = nullptr;

    // Deinit render resources.
    pVao = nullptr;
    material.deinitShaderProgramAndResources(this, getGameInstanceWhileSpawned()->getRenderer());
}

void MeshNode::onSpawning() {
    PROFILE_FUNC

    SpatialNode::onSpawning();

    registerToRendering();
}

void MeshNode::onDespawning() {
    SpatialNode::onDespawning();

    unregisterFromRendering();
}

void MeshNode::onWorldLocationRotationScaleChanged() {
    PROFILE_FUNC

    SpatialNode::onWorldLocationRotationScaleChanged();

    if (pRenderingHandle == nullptr) {
        return;
    }

    // Update shader data.
    auto renderDataGuard = getWorldWhileSpawned()->getMeshRenderer().getMeshRenderData(*pRenderingHandle);
    auto& data = renderDataGuard.getData();

    data.worldMatrix = getWorldMatrix();
    data.normalMatrix = glm::transpose(glm::inverse(data.worldMatrix));
    data.aabbWorld = aabbLocal.convertToWorldSpace(getWorldMatrix());
}
