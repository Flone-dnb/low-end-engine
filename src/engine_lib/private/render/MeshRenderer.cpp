#include "MeshRenderer.h"

// Custom.
#include "game/node/MeshNode.h"
#include "game/camera/CameraProperties.h"
#include "render/LightSourceManager.h"
#include "render/shader/LightSourceShaderArray.h"
#include "render/wrapper/ShaderProgram.h"
#include "game/DebugConsole.h"
#include "render/Renderer.h"
#include "misc/Profiler.hpp"
#include "render/RenderingHandle.h"

// External.
#include "glad/glad.h"

MeshRenderDataGuard::~MeshRenderDataGuard() { pMeshRenderer->mtxRenderData.first.unlock(); }

MeshRenderer::RenderData::ShaderInfo
MeshRenderer::RenderData::ShaderInfo::create(ShaderProgram* pShaderProgram) {
    if (pShaderProgram == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected a valid shader program");
    }

    ShaderInfo info{};
    info.pShaderProgram = pShaderProgram;

    info.iWorldMatrixUniform = pShaderProgram->getShaderUniformLocation("worldMatrix");
    info.iNormalMatrixUniform = pShaderProgram->getShaderUniformLocation("normalMatrix");
    info.iIsUsingDiffuseTextureUniform = pShaderProgram->getShaderUniformLocation("bIsUsingDiffuseTexture");
    info.iDiffuseColorUniform = pShaderProgram->getShaderUniformLocation("diffuseColor");
    info.iTextureTilingMultiplierUniform =
        pShaderProgram->getShaderUniformLocation("textureTilingMultiplier");

    info.iSkinningMatricesUniform = pShaderProgram->tryGetShaderUniformLocation("vSkinningMatrices[0]");

#if defined(ENGINE_EDITOR)
    info.iNodeIdUniform = pShaderProgram->getShaderUniformLocation("iNodeId");
#endif

    return info;
}

MeshRenderer::~MeshRenderer() {
    std::scoped_lock guard(mtxRenderData.first);

    if (mtxRenderData.second.iRegisteredMeshCount != 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            "mesh node manager is being destroyed but there are still some meshes registered");
    }
}

std::unique_ptr<MeshRenderingHandle>
MeshRenderer::addMeshForRendering(ShaderProgram* pShaderProgram, bool bEnableTransparency) {
    PROFILE_FUNC

    std::scoped_lock guard(mtxRenderData.first);
    auto& data = mtxRenderData.second;

    // Check limit.
    if (data.iRegisteredMeshCount == data.vMeshRenderData.size()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "unable to add mesh for rendering because reached limit of renderable meshes: {}",
            data.vMeshRenderData.size()));
    }

    // Warn if close to limit.
    static bool bWarnedAboutCloseToLimit = false;
    if (!bWarnedAboutCloseToLimit &&
        data.iRegisteredMeshCount ==
            static_cast<unsigned short>(static_cast<float>(data.vMeshRenderData.size()) * 0.9F)) {
        bWarnedAboutCloseToLimit = true;
        Log::warn(std::format(
            "adding another mesh for rendering, note the limit, current mesh count for rendering: {}, max: "
            "{}, this message will not be shown again",
            data.iRegisteredMeshCount,
            data.vMeshRenderData.size()));
    }

    unsigned short iNewMeshIndex = 0;

    // Check if we already have meshes that use this shader program.
    RenderData::ShaderInfo* pUpdatedShader = nullptr;
    if (!bEnableTransparency) {
        // Search in opaque shaders.
        for (size_t i = 0; i < data.vOpaqueShaders.size(); i++) {
            auto& shaderInfo = data.vOpaqueShaders[i];

            if (shaderInfo.pShaderProgram != pShaderProgram) {
                continue;
            }
            pUpdatedShader = &shaderInfo;

            iNewMeshIndex = shaderInfo.iFirstMeshIndex + shaderInfo.iMeshCount;
            shaderInfo.iMeshCount += 1;

            if (i + 1 == data.vOpaqueShaders.size() && data.vTransparentShaders.empty()) {
                // Nothing to shift to the right.
                break;
            }

            // Shift all next shaders to the right.
            size_t iShiftItemCount = 0;
            for (size_t j = i + 1; j < data.vOpaqueShaders.size(); j++) {
                auto& shader = data.vOpaqueShaders[j];

                // Also update handles.
                for (unsigned short iHandleIndex = shader.iFirstMeshIndex;
                     iHandleIndex < shader.iFirstMeshIndex + shader.iMeshCount;
                     iHandleIndex++) {
                    data.vIndexToHandle[iHandleIndex]->iMeshRenderDataIndex += 1;
                }

                shader.iFirstMeshIndex += 1;
                iShiftItemCount += shader.iMeshCount;
            }
            for (size_t j = 0; j < data.vTransparentShaders.size(); j++) {
                auto& shader = data.vTransparentShaders[j];

                // Also update handles.
                for (unsigned short iHandleIndex = shader.iFirstMeshIndex;
                     iHandleIndex < shader.iFirstMeshIndex + shader.iMeshCount;
                     iHandleIndex++) {
                    data.vIndexToHandle[iHandleIndex]->iMeshRenderDataIndex += 1;
                }

                shader.iFirstMeshIndex += 1;
                iShiftItemCount += shader.iMeshCount;
            }

            // Shift all data.
            size_t iDstIndex = shaderInfo.iFirstMeshIndex + shaderInfo.iMeshCount;
            size_t iSrcIndex = iDstIndex - 1;
            std::memmove(
                &data.vMeshRenderData[iDstIndex],
                &data.vMeshRenderData[iSrcIndex],
                sizeof(data.vMeshRenderData[0]) * iShiftItemCount);
            std::memmove(
                &data.vIndexToHandle[iDstIndex],
                &data.vIndexToHandle[iSrcIndex],
                sizeof(data.vIndexToHandle[0]) * iShiftItemCount); // NOLINT: need sizeof pointer, not value

            break;
        }
    } else {
        // Search in transparent shaders.
        for (size_t i = 0; i < data.vTransparentShaders.size(); i++) {
            auto& shaderInfo = data.vTransparentShaders[i];

            if (shaderInfo.pShaderProgram != pShaderProgram) {
                continue;
            }
            pUpdatedShader = &shaderInfo;

            iNewMeshIndex = shaderInfo.iFirstMeshIndex + shaderInfo.iMeshCount;
            shaderInfo.iMeshCount += 1;

            if (i + 1 == data.vTransparentShaders.size()) {
                // Nothing to shift to the right.
                break;
            }

            // Shift all next shaders to the right.
            size_t iShiftItemCount = 0;
            for (size_t j = i + 1; j < data.vTransparentShaders.size(); j++) {
                auto& shader = data.vTransparentShaders[j];

                // Also update handles.
                for (unsigned short iHandleIndex = shader.iFirstMeshIndex;
                     iHandleIndex < shader.iFirstMeshIndex + shader.iMeshCount;
                     iHandleIndex++) {
                    data.vIndexToHandle[iHandleIndex]->iMeshRenderDataIndex += 1;
                }

                shader.iFirstMeshIndex += 1;
                iShiftItemCount += shader.iMeshCount;
            }

            // Shift all data.
            size_t iDstIndex = shaderInfo.iFirstMeshIndex + shaderInfo.iMeshCount;
            size_t iSrcIndex = iDstIndex - 1;
            std::memmove(
                &data.vMeshRenderData[iDstIndex],
                &data.vMeshRenderData[iSrcIndex],
                sizeof(data.vMeshRenderData[0]) * iShiftItemCount);
            std::memmove(
                &data.vIndexToHandle[iDstIndex],
                &data.vIndexToHandle[iSrcIndex],
                sizeof(data.vIndexToHandle[0]) * iShiftItemCount); // NOLINT: need sizeof pointer, not value

            break;
        }
    }

    if (pUpdatedShader == nullptr) {
        // Add new shader.
        auto newShaderInfo = RenderData::ShaderInfo::create(pShaderProgram);
        newShaderInfo.iMeshCount = 1;

        if (!bEnableTransparency) {
            // Add mesh to new opaque shader.
            iNewMeshIndex = 0;

            if (!data.vOpaqueShaders.empty()) {
                const auto& last = data.vOpaqueShaders.back();
                iNewMeshIndex = static_cast<unsigned short>(last.iFirstMeshIndex + last.iMeshCount);
            }

            newShaderInfo.iFirstMeshIndex = iNewMeshIndex;
            data.vOpaqueShaders.push_back(newShaderInfo);

            if (!data.vTransparentShaders.empty()) {
                // Shift all next shaders.
                size_t iShiftItemCount = 0;
                for (size_t i = 0; i < data.vTransparentShaders.size(); i++) {
                    auto& shader = data.vTransparentShaders[i];

                    // Also update handles.
                    for (unsigned short iHandleIndex = shader.iFirstMeshIndex;
                         iHandleIndex < shader.iFirstMeshIndex + shader.iMeshCount;
                         iHandleIndex++) {
                        data.vIndexToHandle[iHandleIndex]->iMeshRenderDataIndex += 1;
                    }

                    shader.iFirstMeshIndex += 1;
                    iShiftItemCount += shader.iMeshCount;
                }

                // Shift data to the right.
                size_t iDstIndex =
                    data.vTransparentShaders[0].iFirstMeshIndex + data.vTransparentShaders[0].iMeshCount;
                size_t iSrcIndex = iDstIndex - 1;
                std::memmove(
                    &data.vMeshRenderData[iDstIndex],
                    &data.vMeshRenderData[iSrcIndex],
                    sizeof(data.vMeshRenderData[0]) * iShiftItemCount);
                std::memmove(
                    &data.vIndexToHandle[iDstIndex],
                    &data.vIndexToHandle[iSrcIndex],
                    sizeof(data.vIndexToHandle[0]) * // NOLINT: need sizeof pointer, not value
                        iShiftItemCount);
            }
        } else {
            // Add mesh to new transparent shader.
            iNewMeshIndex = 0;

            if (data.vTransparentShaders.empty() && !data.vOpaqueShaders.empty()) {
                const auto& last = data.vOpaqueShaders.back();
                iNewMeshIndex = last.iFirstMeshIndex + last.iMeshCount;
            } else {
                const auto& last = data.vTransparentShaders.back();
                iNewMeshIndex = static_cast<unsigned short>(last.iFirstMeshIndex + last.iMeshCount);
            }

            newShaderInfo.iFirstMeshIndex = iNewMeshIndex;
            data.vTransparentShaders.push_back(newShaderInfo);

            // Nothing to shift, we added to the end of the array.
        }
    }

    auto pNewHandle = std::unique_ptr<MeshRenderingHandle>(new MeshRenderingHandle(this, iNewMeshIndex));

    data.vMeshRenderData[iNewMeshIndex] = {};
    data.vIndexToHandle[iNewMeshIndex] = pNewHandle.get();

    data.iRegisteredMeshCount += 1;

#if defined(DEBUG)
    runDebugIndexValidation();
#endif

    return pNewHandle;
}

void MeshRenderer::onBeforeHandleDestroyed(MeshRenderingHandle* pHandle) {
    PROFILE_FUNC

    std::scoped_lock guard(mtxRenderData.first);
    auto& data = mtxRenderData.second;

    const auto iMeshIndex = pHandle->iMeshRenderDataIndex;

    // Remove from shader array.
    size_t iShiftItemCount = 0;
    const auto removeFromShadersUpdateOther =
        [iMeshIndex, &iShiftItemCount, &data](std::vector<RenderData::ShaderInfo>& shaders) -> bool {
        for (size_t i = 0; i < shaders.size(); i++) {
            auto& shaderInfo = shaders[i];

            if (iMeshIndex >= shaderInfo.iFirstMeshIndex &&
                iMeshIndex < shaderInfo.iFirstMeshIndex + shaderInfo.iMeshCount) {
                if (shaderInfo.iMeshCount == 1) {
                    // Shift all next indices to the left.
                    for (size_t j = i + 1; j < shaders.size(); j++) {
                        auto& shader = shaders[j];

                        // Also update handles.
                        for (unsigned short iHandleIndex = shader.iFirstMeshIndex;
                             iHandleIndex < shader.iFirstMeshIndex + shader.iMeshCount;
                             iHandleIndex++) {
                            data.vIndexToHandle[iHandleIndex]->iMeshRenderDataIndex -= 1;
                        }

                        shader.iFirstMeshIndex -= 1;
                        iShiftItemCount += shader.iMeshCount;
                    }

                    // Then remove shader.
                    shaders.erase(shaders.begin() + static_cast<long>(i));

                    return true;
                } else {
                    // Update handles in this shader.
                    for (size_t j = iMeshIndex + 1; j < shaderInfo.iFirstMeshIndex + shaderInfo.iMeshCount;
                         j++) {
                        data.vIndexToHandle[j]->iMeshRenderDataIndex -= 1;
                        iShiftItemCount += 1;
                    }

                    // Decrease mesh count in shader.
                    shaderInfo.iMeshCount -= 1;

                    // Shift all next indices to the left.
                    for (size_t j = i + 1; j < shaders.size(); j++) {
                        auto& shader = shaders[j];

                        // Also update handles.
                        for (unsigned short iHandleIndex = shader.iFirstMeshIndex;
                             iHandleIndex < shader.iFirstMeshIndex + shader.iMeshCount;
                             iHandleIndex++) {
                            data.vIndexToHandle[iHandleIndex]->iMeshRenderDataIndex -= 1;
                        }

                        shader.iFirstMeshIndex -= 1;
                        iShiftItemCount += shader.iMeshCount;
                    }

                    return true;
                }
            }
        }
        return false;
    };
    if (!removeFromShadersUpdateOther(data.vOpaqueShaders)) {
        if (!removeFromShadersUpdateOther(data.vTransparentShaders)) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("unable to unregister mesh with index {} from rendering", iMeshIndex));
        }
    } else {
        // Opaque shaders are updated. Shift all next indices to the left.
        for (size_t i = 0; i < data.vTransparentShaders.size(); i++) {
            auto& shader = data.vTransparentShaders[i];

            // Also update handles.
            for (unsigned short iHandleIndex = shader.iFirstMeshIndex;
                 iHandleIndex < shader.iFirstMeshIndex + shader.iMeshCount;
                 iHandleIndex++) {
                data.vIndexToHandle[iHandleIndex]->iMeshRenderDataIndex -= 1;
            }

            shader.iFirstMeshIndex -= 1;
            iShiftItemCount += shader.iMeshCount;
        }
    }

    if (iShiftItemCount > 0) {
        // Update render data and handles (shift next data to the left).
        size_t iDstIndex = iMeshIndex;
        size_t iSrcIndex = iMeshIndex + 1;
        std::memmove(
            &data.vMeshRenderData[iDstIndex],
            &data.vMeshRenderData[iSrcIndex],
            sizeof(data.vMeshRenderData[0]) * iShiftItemCount);
        std::memmove(
            &data.vIndexToHandle[iDstIndex],
            &data.vIndexToHandle[iSrcIndex],
            sizeof(data.vIndexToHandle[0]) * iShiftItemCount); // NOLINT: need sizeof pointer, not value
    }

    // Update count.
    if (data.iRegisteredMeshCount == 0) [[unlikely]] {
        Error::showErrorAndThrowException("unexpected 0 count");
    }
    data.iRegisteredMeshCount -= 1;

#if defined(DEBUG)
    runDebugIndexValidation();
#endif
}

#if defined(DEBUG)
void MeshRenderer::runDebugIndexValidation() {
    // expecting that the mutex is locked
    auto& data = mtxRenderData.second;

    // Self check: make sure all indices go in the ascending order.
    unsigned short iNextIndex = 0;
    for (const auto& shader : data.vOpaqueShaders) {
        if (shader.iMeshCount == 0) [[unlikely]] {
            Error::showErrorAndThrowException(std::format("found shader section with mesh count 0"));
        }

        if (iNextIndex == shader.iFirstMeshIndex) [[likely]] {
            for (auto i = shader.iFirstMeshIndex; i < shader.iFirstMeshIndex + shader.iMeshCount; i++) {
                if (data.vIndexToHandle[i]->iMeshRenderDataIndex != i) [[unlikely]] {
                    Error::showErrorAndThrowException(std::format(
                        "found handle with invalid index {}, expected index {}",
                        data.vIndexToHandle[i]->iMeshRenderDataIndex,
                        i));
                }
            }

            iNextIndex = shader.iFirstMeshIndex + shader.iMeshCount;
            continue;
        }

        Error::showErrorAndThrowException(std::format(
            "found unexpected \"first mesh index\" of {} for an opaque shader, expected {}",
            shader.iFirstMeshIndex,
            iNextIndex));
    }

    // Check transparent.
    for (const auto& shader : data.vTransparentShaders) {
        if (shader.iMeshCount == 0) [[unlikely]] {
            Error::showErrorAndThrowException(std::format("found shader section with mesh count 0"));
        }

        if (iNextIndex == shader.iFirstMeshIndex) {
            for (auto i = shader.iFirstMeshIndex; i < shader.iFirstMeshIndex + shader.iMeshCount; i++) {
                if (data.vIndexToHandle[i]->iMeshRenderDataIndex != i) [[unlikely]] {
                    Error::showErrorAndThrowException(std::format(
                        "found handle with invalid index {}, expected index {}",
                        data.vIndexToHandle[i]->iMeshRenderDataIndex,
                        i));
                }
            }

            iNextIndex = shader.iFirstMeshIndex + shader.iMeshCount;
            continue;
        }

        Error::showErrorAndThrowException(std::format(
            "found unexpected \"first mesh index\" of {} for a transparent shader, expected {}",
            shader.iFirstMeshIndex,
            iNextIndex));
    }

    // Check total count.
    if (iNextIndex != data.iRegisteredMeshCount) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "found mismatch between mesh indices sum ({}) and registered mesh count ({})",
            iNextIndex,
            data.iRegisteredMeshCount));
    }
}
#endif

MeshRenderDataGuard MeshRenderer::getMeshRenderData(MeshRenderingHandle& handle) {
    mtxRenderData.first.lock(); // guard will unlock it when destroyed
    auto& data = mtxRenderData.second;

    return MeshRenderDataGuard(this, &data.vMeshRenderData[handle.iMeshRenderDataIndex]);
}

void MeshRenderer::drawMeshes(
    Renderer* pRenderer, CameraProperties* pCameraProperties, LightSourceManager& lightSourceManager) {
    std::scoped_lock guard(mtxRenderData.first);
    auto& data = mtxRenderData.second;

#if defined(ENGINE_DEBUG_TOOLS)
    DebugConsole::getStats().iRenderedMeshCount = 0;
    DebugConsole::getStats().iRenderedLightSourceCount =
        lightSourceManager.getDirectionalLightsArray().getVisibleLightSourceCount() +
        lightSourceManager.getPointLightsArray().getVisibleLightSourceCount() +
        lightSourceManager.getSpotlightsArray().getVisibleLightSourceCount();
#endif

    // Prepare slot for diffuse texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    drawMeshes(data.vOpaqueShaders, data, pCameraProperties, lightSourceManager);

    if (!data.vTransparentShaders.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        {
            drawMeshes(data.vTransparentShaders, data, pCameraProperties, lightSourceManager);
        }
        glDisable(GL_BLEND);
    }
}

void MeshRenderer::drawMeshes(
    const std::vector<RenderData::ShaderInfo>& vShaders,
    const RenderData& data,
    CameraProperties* pCameraProperties,
    LightSourceManager& lightSourceManager) {
    PROFILE_FUNC

#if defined(ENGINE_DEBUG_TOOLS)
    auto& debugStats = DebugConsole::getStats();
#endif

    for (const auto& shaderInfo : vShaders) {
        PROFILE_SCOPE("render mesh nodes of shader program")
        PROFILE_ADD_SCOPE_TEXT(
            shaderInfo.pShaderProgram->getName().data(), shaderInfo.pShaderProgram->getName().size());

        // Set shader program.
        glUseProgram(shaderInfo.pShaderProgram->getShaderProgramId());

        // Set camera uniforms.
        pCameraProperties->getShaderConstantsSetter().setConstantsToShader(shaderInfo.pShaderProgram);

        // Set light arrays.
        lightSourceManager.setArrayPropertiesToShader(shaderInfo.pShaderProgram);

        for (unsigned short iMeshDataIndex = shaderInfo.iFirstMeshIndex;
             iMeshDataIndex < shaderInfo.iFirstMeshIndex + shaderInfo.iMeshCount;
             iMeshDataIndex++) {
            const auto& meshData = data.vMeshRenderData[iMeshDataIndex];

#if defined(ENGINE_EDITOR)
            // For GPU picking.
            glUniform1ui(shaderInfo.iNodeIdUniform, meshData.iNodeId);
#endif

            glBindVertexArray(meshData.iVertexArrayObject);

            // Binds 0 (no texture) if not set.
            glBindTexture(GL_TEXTURE_2D, meshData.iDiffuseTextureId);

            // Set uniforms.
            glUniformMatrix4fv(
                shaderInfo.iWorldMatrixUniform, 1, GL_FALSE, glm::value_ptr(meshData.worldMatrix));
            glUniformMatrix3fv(
                shaderInfo.iNormalMatrixUniform, 1, GL_FALSE, glm::value_ptr(meshData.normalMatrix));
            glUniform1i(
                shaderInfo.iIsUsingDiffuseTextureUniform, static_cast<int>(meshData.iDiffuseTextureId));
            glUniform4fv(shaderInfo.iDiffuseColorUniform, 1, glm::value_ptr(meshData.diffuseColor));
            glUniform2fv(
                shaderInfo.iTextureTilingMultiplierUniform,
                1,
                glm::value_ptr(meshData.textureTilingMultiplier));

            if (shaderInfo.iSkinningMatricesUniform != -1) {
                glUniformMatrix4fv(
                    shaderInfo.iSkinningMatricesUniform,
                    meshData.iSkinningMatrixCount,
                    GL_FALSE,
                    meshData.pSkinningMatrices);
            }

            glDrawElements(GL_TRIANGLES, meshData.iIndexCount, GL_UNSIGNED_SHORT, nullptr);
#if defined(ENGINE_DEBUG_TOOLS)
            debugStats.iRenderedMeshCount += 1;
#endif
        }
    }
}
