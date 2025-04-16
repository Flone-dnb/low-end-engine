#include "game/node/MeshNode.h"

// Custom.
#include "render/GpuResourceManager.h"
#include "game/GameInstance.h"
#include "game/geometry/PrimitiveMeshGenerator.h"
#include "render/wrapper/ShaderProgram.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "bea29d45-274d-4a50-91ec-8ca09897ce4d";
}

std::string MeshNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string MeshNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo MeshNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.bools["isVisible"] = ReflectedVariableInfo<bool>{
        .setter = [](Serializable* pThis,
                     const bool& bNewValue) { reinterpret_cast<MeshNode*>(pThis)->setIsVisible(bNewValue); },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<MeshNode*>(pThis)->isVisible();
        }};

    variables.vec3s["materialDiffuseColor"] = ReflectedVariableInfo<glm::vec3>{
        .setter =
            [](Serializable* pThis, const glm::vec3& newValue) {
                reinterpret_cast<MeshNode*>(pThis)->getMaterial().setDiffuseColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec3 {
            return reinterpret_cast<MeshNode*>(pThis)->getMaterial().getDiffuseColor();
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

    variables.meshGeometries[NAMEOF_MEMBER(&MeshNode::geometry).data()] = ReflectedVariableInfo<MeshGeometry>{
        .setter =
            [](Serializable* pThis, const MeshGeometry& newValue) {
                reinterpret_cast<MeshNode*>(pThis)->setMeshGeometryBeforeSpawned(newValue);
            },
        .getter = [](Serializable* pThis) -> MeshGeometry {
            return reinterpret_cast<MeshNode*>(pThis)->copyMeshData();
        }};

    return TypeReflectionInfo(
        "",
        NAMEOF_SHORT_TYPE(MeshNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<MeshNode>(); },
        std::move(variables));
}

MeshNode::MeshNode() : MeshNode("Mesh Node") {}

MeshNode::MeshNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    mtxIsVisible.second = true;

    geometry = PrimitiveMeshGenerator::createCube(1.0F);
}

void MeshNode::setMaterialBeforeSpawned(const Material& material) {
    std::scoped_lock guard(getSpawnDespawnMutex());

    // For simplicity we don't allow changing material while spawned.
    // Moreover, renderable node manager does not expect us to change material.
    if (isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("changing material of a spawned node is not allowed (node \"{}\")", getNodeName()));
    }

    this->material = material;
}

void MeshNode::setMeshGeometryBeforeSpawned(const MeshGeometry& meshGeometry) {
    geometry = meshGeometry;
    onAfterMeshGeometryChanged();
}

void MeshNode::setMeshGeometryBeforeSpawned(MeshGeometry&& meshGeometry) {
    geometry = std::move(meshGeometry);
    onAfterMeshGeometryChanged();
}

void MeshNode::setIsVisible(bool bVisible) {
    std::scoped_lock guard(getSpawnDespawnMutex(), mtxIsVisible.first);

    if (mtxIsVisible.second == bVisible) {
        return;
    }
    mtxIsVisible.second = bVisible;

    if (isSpawned()) {
        material.onNodeChangedVisibilityWhileSpawned(
            bVisible, this, getGameInstanceWhileSpawned()->getRenderer());
    }
}

Material& MeshNode::getMaterial() { return material; }

bool MeshNode::isVisible() {
    std::scoped_lock guard(mtxIsVisible.first);
    return mtxIsVisible.second;
}

void MeshNode::onSpawning() {
    SpatialNode::onSpawning();

    // Create VAO.
    pVao = GpuResourceManager::createVertexArrayObject(geometry);

    // Create shader constants setter.
    shaderConstantsSetter = ShaderConstantsSetter();

    // Request shader program.
    material.onNodeSpawning(
        this, getGameInstanceWhileSpawned()->getRenderer(), [this](ShaderProgram* pShaderProgram) {
            shaderConstantsSetter->addSetterFunction([this](ShaderProgram* pShaderProgram) {
                std::scoped_lock guard(mtxShaderConstants.first);
                auto& uniforms = mtxShaderConstants.second;

                pShaderProgram->setMatrix4ToShader(
                    NAMEOF(mtxShaderConstants.second.worldMatrix).c_str(), uniforms.worldMatrix);
                pShaderProgram->setMatrix3ToShader(
                    NAMEOF(mtxShaderConstants.second.normalMatrix).c_str(), uniforms.normalMatrix);
            });
        });
}

void MeshNode::onDespawning() {
    SpatialNode::onDespawning();

    // Destroy VAO and constants manager.
    pVao = nullptr;
    shaderConstantsSetter = {};

    // Notify material.
    material.onNodeDespawning(this, getGameInstanceWhileSpawned()->getRenderer());
}

void MeshNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    // Update shader constants.
    std::scoped_lock guard(mtxShaderConstants.first);

    mtxShaderConstants.second.worldMatrix = getWorldMatrix();
    mtxShaderConstants.second.normalMatrix =
        glm::transpose(glm::inverse(mtxShaderConstants.second.worldMatrix));
}

void MeshNode::onAfterMeshGeometryChanged() {
    std::scoped_lock guard(getSpawnDespawnMutex());

    // For simplicity we don't allow changing geometry while spawned.
    if (isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format(
                "changing geometry of a spawned node is not allowed, if you need procedural/dynamic geometry "
                "consider passing some additional data to the vertex shader and changing vertices there "
                "(node "
                "\"{}\")",
                getNodeName()));
    }
}
