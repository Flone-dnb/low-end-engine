#include "game/node/MeshNode.h"

// Custom.
#include "render/GpuResourceManager.h"
#include "game/GameInstance.h"
#include "game/geometry/PrimitiveMeshGenerator.h"
#include "render/ShaderProgram.h"

// External.
#include "nameof.hpp"

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
        Error error(
            std::format("changing material of a spawned node is not allowed (node \"{}\")", getNodeName()));
        error.showErrorAndThrowException();
    }

    this->material = material;
}

void MeshNode::setMeshDataBeforeSpawned(const MeshGeometry& meshGeometry) {
    geometry = meshGeometry;
    onAfterMeshGeometryChanged();
}

void MeshNode::setMeshDataBeforeSpawned(MeshGeometry&& meshGeometry) {
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

    // Create shader constants manager.
    shaderConstantManager = ShaderConstantManager();

    // Request shader program.
    material.onNodeSpawning(
        this, getGameInstanceWhileSpawned()->getRenderer(), [this](ShaderProgram* pShaderProgram) {
            shaderConstantManager->addSetterFunction(
                [this,
                 iWorldMatrixLocation =
                     pShaderProgram->getShaderUniformLocation(NAMEOF(mtxShaderConstants.second.worldMatrix)),
                 iNormalMatrixLocation = pShaderProgram->getShaderUniformLocation(
                     NAMEOF(mtxShaderConstants.second.normalMatrix))](ShaderProgram* pShaderProgram) {
                    std::scoped_lock guard(mtxShaderConstants.first);
                    auto& uniforms = mtxShaderConstants.second;

                    pShaderProgram->setMatrix4ToShader(iWorldMatrixLocation, uniforms.worldMatrix);
                    pShaderProgram->setMatrix3ToShader(iNormalMatrixLocation, uniforms.normalMatrix);
                });
        });
}

void MeshNode::onDespawning() {
    SpatialNode::onDespawning();

    // Destroy VAO and constants manager.
    pVao = nullptr;
    shaderConstantManager = {};

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
        Error error(std::format(
            "changing geometry of a spawned node is not allowed, if you need procedural/dynamic geometry "
            "consider passing some additional data to the vertex shader and changing vertices there (node "
            "\"{}\")",
            getNodeName()));
        error.showErrorAndThrowException();
    }
}
