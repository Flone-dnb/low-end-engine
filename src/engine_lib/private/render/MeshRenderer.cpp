#include "MeshRenderer.h"

// Custom.
#include "game/node/MeshNode.h"
#include "render/LightSourceManager.h"
#include "render/shader/LightSourceShaderArray.h"
#include "render/wrapper/ShaderProgram.h"
#include "game/DebugConsole.h"
#include "render/Renderer.h"
#include "misc/Profiler.hpp"
#include "render/RenderingHandle.h"
#include "render/wrapper/Framebuffer.h"
#include "render/wrapper/Texture.h"
#include "game/node/light/SpotlightNode.h"
#include "game/node/light/PointLightNode.h"
#include "render/GpuTimeQuery.hpp"
#include "render/GpuDebugMarker.hpp"

// External.
#include "SDL3/SDL.h"
#include "glad/glad.h"

MeshRenderDataGuard::~MeshRenderDataGuard() {
    if (pData->iDiffuseTextureId == 0) {
        // shaders expect -1 if diffuse texture is not set
        pData->textureTilingMultiplier = glm::vec2(-1.0f, -1.0f);
    }

    pMeshRenderer->mtxRenderData.first.unlock();
}

MeshRenderer::RenderData::ShaderInfo
MeshRenderer::RenderData::ShaderInfo::create(ShaderProgram* pShaderProgram) {
    if (pShaderProgram == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected a valid shader program");
    }

    ShaderInfo info{};
    info.pShaderProgram = pShaderProgram;

    // Collect uniforms from a shader program that only has vertex shader linked.
    {
        const auto iVertexOnlyProgramId = pShaderProgram->getVertexOnlyShaderProgramId();

        const auto getVertexOnlyUniform = [iVertexOnlyProgramId](std::string_view sName) -> int {
            const auto iLocation = glGetUniformLocation(iVertexOnlyProgramId, sName.data());
            if (iLocation < 0) {
                return -1;
            }
            return iLocation;
        };

        info.iVertexOnlyWorldMatrixUniform = getVertexOnlyUniform("worldMatrix");
        info.iVertexOnlyNormalMatrixUniform = getVertexOnlyUniform("normalMatrix");
        info.iVertexOnlySkinningMatricesUniform = getVertexOnlyUniform("vSkinningMatrices[0]");
        info.iVertexOnlyViewMatrixUniform = getVertexOnlyUniform("viewMatrix");
        info.iVertexOnlyViewProjectionMatrixUniform = getVertexOnlyUniform("viewProjectionMatrix");
    }

    info.iWorldMatrixUniform = pShaderProgram->getShaderUniformLocation("worldMatrix");
    info.iNormalMatrixUniform = pShaderProgram->getShaderUniformLocation("normalMatrix");
    info.iDiffuseColorUniform = pShaderProgram->getShaderUniformLocation("diffuseColor");
    info.iTextureTilingMultiplierUniform =
        pShaderProgram->getShaderUniformLocation("textureTilingMultiplier");
    info.iDiffuseTextureUniform = pShaderProgram->getShaderUniformLocation("diffuseTexture");

    info.iSkinningMatricesUniform = pShaderProgram->tryGetShaderUniformLocation("vSkinningMatrices[0]");
    info.iSpotShadowMapsUniform = pShaderProgram->getShaderUniformLocation("spotShadowMaps");
    info.iIsSpotlightCulledUniform = pShaderProgram->getShaderUniformLocation("isSpotlightCulled[0]");
    info.iIsPointLightCulledUniform = pShaderProgram->getShaderUniformLocation("isPointLightCulled[0]");

    info.iAmbientLightColorUniform = pShaderProgram->getShaderUniformLocation("ambientLightColor");
    info.iPointLightCountUniform = pShaderProgram->getShaderUniformLocation("iPointLightCount");
    info.iSpotlightCountUniform = pShaderProgram->getShaderUniformLocation("iSpotlightCount");
    info.iDirectionalLightCountUniform = pShaderProgram->getShaderUniformLocation("iDirectionalLightCount");
    info.iPointLightsUniformBlockBindingIndex =
        pShaderProgram->getShaderUniformBlockBindingIndex("PointLights");
    info.iSpotlightsUniformBlockBindingIndex =
        pShaderProgram->getShaderUniformBlockBindingIndex("Spotlights");
    info.iDirectionalLightsUniformBlockBindingIndex =
        pShaderProgram->getShaderUniformBlockBindingIndex("DirectionalLights");

    info.iDistanceFogColorUniform = pShaderProgram->getShaderUniformLocation("distanceFogColor");
    info.iDistanceFogRangeUniform = pShaderProgram->getShaderUniformLocation("distanceFogRange");

    info.iViewMatrixUniform = pShaderProgram->getShaderUniformLocation("viewMatrix");
    info.iViewProjectionMatrixUniform = pShaderProgram->getShaderUniformLocation("viewProjectionMatrix");

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
            static_cast<unsigned short>(static_cast<float>(data.vMeshRenderData.size()) * 0.9f)) {
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
    Renderer* pRenderer,
    const glm::ivec4& viewportSize,
    const glm::mat4& viewMatrix,
    const glm::mat4& viewProjectionMatrix,
    const Frustum& cameraFrustum,
    LightSourceManager& lightSourceManager,
    unsigned int iGlDrawShadowPassQuery,
    unsigned int iGlDrawDepthPrepassQuery,
    unsigned int iGlDrawMeshesQuery) {
    std::scoped_lock guard(mtxRenderData.first);
    auto& data = mtxRenderData.second;

    // Prepare light parameters.
    glm::vec3 ambientLightColor = lightSourceManager.getAmbientLightColor();
    auto& mtxPointLightData = lightSourceManager.getPointLightsArray().getInternalLightData();
    auto& mtxSpotlightData = lightSourceManager.getSpotlightsArray().getInternalLightData();
    auto& mtxDirectionalLightData = lightSourceManager.getDirectionalLightsArray().getInternalLightData();

    std::scoped_lock pointLightDataGuard(mtxPointLightData.first);
    std::scoped_lock spotlightDataGuard(mtxSpotlightData.first);
    std::scoped_lock directionalLightDataGuard(mtxDirectionalLightData.first);

#if defined(ENGINE_DEBUG_TOOLS)
    DebugConsole::getStats().iRenderedMeshCount = 0;
    DebugConsole::getStats().iActiveLightSourceCount =
        mtxPointLightData.second.visibleLightNodes.size() + mtxSpotlightData.second.visibleLightNodes.size() +
        mtxDirectionalLightData.second.visibleLightNodes.size();
#endif

    auto lightCullingInfo = drawShadowPass(
        data, cameraFrustum, mtxPointLightData.second, mtxSpotlightData.second, iGlDrawShadowPassQuery);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // <- restore framebuffer from shadow pass

    glViewport(viewportSize.x, viewportSize.y, viewportSize.z, viewportSize.w);

    drawDepthPrepass(
        pRenderer, data, cameraFrustum, viewMatrix, viewProjectionMatrix, iGlDrawDepthPrepassQuery);

    {
        MEASURE_GPU_TIME_SCOPED(iGlDrawMeshesQuery);
#if defined(ENGINE_DEBUG_TOOLS)
        const auto cpuSubmitStartCounter = SDL_GetPerformanceCounter();
#endif

        glDepthMask(GL_FALSE); // disable depth write (after z prepass depth is already filled)

        {
            GPU_MARKER_SCOPED("main pass opaque meshes");
            drawMeshes(
                pRenderer,
                data.vOpaqueShaders,
                data,
                viewMatrix,
                viewProjectionMatrix,
                cameraFrustum,
                ambientLightColor,
                mtxPointLightData.second,
                mtxSpotlightData.second,
                mtxDirectionalLightData.second,
                lightSourceManager.getSpotlightShadowMapArray(),
                lightCullingInfo);
        }

        if (!data.vTransparentShaders.empty()) {
            GPU_MARKER_SCOPED("main pass transparent meshes");
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            {
                drawMeshes(
                    pRenderer,
                    data.vTransparentShaders,
                    data,
                    viewMatrix,
                    viewProjectionMatrix,
                    cameraFrustum,
                    ambientLightColor,
                    mtxPointLightData.second,
                    mtxSpotlightData.second,
                    mtxDirectionalLightData.second,
                    lightSourceManager.getSpotlightShadowMapArray(),
                    lightCullingInfo);
            }
            glDisable(GL_BLEND);
        }

        glDepthMask(GL_TRUE);

#if defined(ENGINE_DEBUG_TOOLS)
        // ADD not overwrite because there might be multiple worlds that use this function
        DebugConsole::getStats().cpuTimeToSubmitMeshesMs +=
            static_cast<float>(SDL_GetPerformanceCounter() - cpuSubmitStartCounter) * 1000.0f /
            static_cast<float>(SDL_GetPerformanceFrequency());
#endif
    }
}

void MeshRenderer::drawMeshesVertexShaderOnly(
    const RenderData& data,
    const std::vector<RenderData::ShaderInfo>& vShaders,
    const glm::mat4& viewMatrix,
    const glm::mat4& viewProjectionMatrix,
    const Frustum& cameraFrustum) {
    for (const auto& shaderInfo : vShaders) {
        glUseProgram(shaderInfo.pShaderProgram->getVertexOnlyShaderProgramId());

        glUniformMatrix4fv(shaderInfo.iVertexOnlyViewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(
            shaderInfo.iVertexOnlyViewProjectionMatrixUniform,
            1,
            GL_FALSE,
            glm::value_ptr(viewProjectionMatrix));

        for (unsigned short iMeshDataIndex = shaderInfo.iFirstMeshIndex;
             iMeshDataIndex < shaderInfo.iFirstMeshIndex + shaderInfo.iMeshCount;
             iMeshDataIndex++) {
            const auto& meshData = data.vMeshRenderData[iMeshDataIndex];

            // Frustum culling (don't cull skeletal meshes due to animations).
            if (shaderInfo.iVertexOnlySkinningMatricesUniform == -1 &&
                !cameraFrustum.isAabbInFrustum(meshData.aabbWorld)) {
                continue;
            }

            glBindVertexArray(meshData.iVertexArrayObject);

            glUniformMatrix4fv(
                shaderInfo.iVertexOnlyWorldMatrixUniform, 1, GL_FALSE, glm::value_ptr(meshData.worldMatrix));
            glUniformMatrix3fv(
                shaderInfo.iVertexOnlyNormalMatrixUniform,
                1,
                GL_FALSE,
                glm::value_ptr(meshData.normalMatrix));
            if (shaderInfo.iVertexOnlySkinningMatricesUniform != -1) {
                glUniformMatrix4fv(
                    shaderInfo.iVertexOnlySkinningMatricesUniform,
                    meshData.iSkinningMatrixCount,
                    GL_FALSE,
                    meshData.pSkinningMatrices);
            }

            glDrawElements(GL_TRIANGLES, meshData.iIndexCount, GL_UNSIGNED_SHORT, nullptr);
        }
    }
}

MeshRenderer::LightCullingInfo MeshRenderer::drawShadowPass(
    const RenderData& data,
    const Frustum& cameraFrustum,
    LightSourceShaderArray::LightData& pointLightData,
    LightSourceShaderArray::LightData& spotlightData,
    unsigned int iGlDrawShadowPassQuery) {
    PROFILE_FUNC
    GPU_MARKER_SCOPED("shadow pass");
    MEASURE_GPU_TIME_SCOPED(iGlDrawShadowPassQuery);

#if defined(ENGINE_DEBUG_TOOLS)
    const auto cpuSubmitStartCounter = SDL_GetPerformanceCounter();
#endif

    LightCullingInfo lightCullingInfo{};
    std::memset(&lightCullingInfo, 0, sizeof(lightCullingInfo));

#if defined(ENGINE_DEBUG_TOOLS)
    auto& debugStats = DebugConsole::getStats();
    debugStats.iCulledLightSourceCount = 0;
#endif

    // Process point lights.
    for (const auto& pNode : pointLightData.visibleLightNodes) {
        const auto pPointLightNode = reinterpret_cast<PointLightNode*>(pNode);

        if (!cameraFrustum.isSphereInFrustum(pPointLightNode->getSphereShapeWorld())) {
            // No light in camera's frustum.
            lightCullingInfo
                .vIsSpotlightCulled[pPointLightNode->getInternalLightSourceHandle()->getActualIndex()] = 1;
#if defined(ENGINE_DEBUG_TOOLS)
            debugStats.iCulledLightSourceCount += 1;
#endif
            continue;
        }
    }

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.9f, 0.0f); // <- slope bias for shadow mapping

    // Process spotlights.
    for (const auto& pNode : spotlightData.visibleLightNodes) {
        const auto pSpotlightNode = reinterpret_cast<SpotlightNode*>(pNode);

        if (!cameraFrustum.isConeInFrustum(pSpotlightNode->getConeShapeWorld())) {
            // No shadows in camera's frustum.
            lightCullingInfo
                .vIsSpotlightCulled[pSpotlightNode->getInternalLightSourceHandle()->getActualIndex()] = 1;
#if defined(ENGINE_DEBUG_TOOLS)
            debugStats.iCulledLightSourceCount += 1;
#endif
            continue;
        }

        const auto pShadowData = pSpotlightNode->getInternalShadowMapData();
        if (pShadowData == nullptr) {
            // Shadow casting not enabled.
            continue;
        }

        const auto [iFramebufferWidth, iFramebufferHeight] = pShadowData->pFramebuffer->getSize();

        glBindFramebuffer(GL_FRAMEBUFFER, pShadowData->pFramebuffer->getFramebufferId());
        glViewport(0, 0, iFramebufferWidth, iFramebufferHeight);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glClear(GL_DEPTH_BUFFER_BIT);

        drawMeshesVertexShaderOnly(
            data,
            data.vOpaqueShaders,
            pShadowData->viewMatrix,
            pSpotlightNode->getLightViewProjectionMatrix(),
            pShadowData->frustumWorld);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    // Restore state.
    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Insert a barrier before reading shadow maps.
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

#if defined(ENGINE_DEBUG_TOOLS)
    // ADD not overwrite because there might be multiple worlds that use this function
    DebugConsole::getStats().cpuTimeToSubmitShadowPassMs +=
        static_cast<float>(SDL_GetPerformanceCounter() - cpuSubmitStartCounter) * 1000.0f /
        static_cast<float>(SDL_GetPerformanceFrequency());
#endif

    return lightCullingInfo;
}

void MeshRenderer::drawDepthPrepass(
    Renderer* pRenderer,
    const RenderData& data,
    const Frustum& cameraFrustum,
    const glm::mat4& viewMatrix,
    const glm::mat4& viewProjectionMatrix,
    unsigned int iGlDrawDepthPrepassQuery) {
    PROFILE_FUNC
    GPU_MARKER_SCOPED("depth prepass");
    MEASURE_GPU_TIME_SCOPED(iGlDrawDepthPrepassQuery);

#if defined(ENGINE_DEBUG_TOOLS)
    const auto cpuSubmitStartCounter = SDL_GetPerformanceCounter();
#endif

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthFunc(GL_LESS);

    drawMeshesVertexShaderOnly(data, data.vOpaqueShaders, viewMatrix, viewProjectionMatrix, cameraFrustum);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(pRenderer->getCurrentGlDepthFunc());
#if defined(DEBUG)
    // Self check:
    if (pRenderer->getCurrentGlDepthFunc() != GL_LEQUAL) [[unlikely]] {
        Error::showErrorAndThrowException("expected LEQUAL depth func");
    }
#endif

#if defined(ENGINE_DEBUG_TOOLS)
    // ADD not overwrite because there might be multiple worlds that use this function
    DebugConsole::getStats().cpuTimeToSubmitDepthPrepassMs +=
        static_cast<float>(SDL_GetPerformanceCounter() - cpuSubmitStartCounter) * 1000.0f /
        static_cast<float>(SDL_GetPerformanceFrequency());
#endif
}

void MeshRenderer::drawMeshes(
    Renderer* pRenderer,
    const std::vector<RenderData::ShaderInfo>& vShaders,
    const RenderData& data,
    const glm::mat4& viewMatrix,
    const glm::mat4& viewProjectionMatrix,
    const Frustum& cameraFrustum,
    const glm::vec3& ambientLightColor,
    LightSourceShaderArray::LightData& pointLightData,
    LightSourceShaderArray::LightData& spotlightData,
    LightSourceShaderArray::LightData& directionalLightData,
    Texture& spotShadowMapArray,
    const LightCullingInfo& lightCullingInfo) {
    PROFILE_FUNC

#if defined(ENGINE_DEBUG_TOOLS)
    auto& debugStats = DebugConsole::getStats();
#endif

    const auto& optDistanceFog = pRenderer->getDistanceFogSettings();

    for (const auto& shaderInfo : vShaders) {
        PROFILE_SCOPE("render mesh nodes of shader program")
        PROFILE_ADD_SCOPE_TEXT(
            shaderInfo.pShaderProgram->getName().data(), shaderInfo.pShaderProgram->getName().size());

        glUseProgram(shaderInfo.pShaderProgram->getShaderProgramId());

        shaderConstantsSetter.setConstantsToShader(shaderInfo.pShaderProgram);

        // Set camera uniforms.
        glUniformMatrix4fv(shaderInfo.iViewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(
            shaderInfo.iViewProjectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));

        // Set light uniforms.
        glUniform3fv(shaderInfo.iAmbientLightColorUniform, 1, glm::value_ptr(ambientLightColor));

        // Point lights.
        glUniform1ui(
            shaderInfo.iPointLightCountUniform,
            static_cast<unsigned int>(pointLightData.visibleLightNodes.size()));
        glBindBufferBase(
            GL_UNIFORM_BUFFER,
            shaderInfo.iPointLightsUniformBlockBindingIndex,
            pointLightData.pUniformBufferObject->getBufferId());

        // Spotlights.
        glUniform1ui(
            shaderInfo.iSpotlightCountUniform,
            static_cast<unsigned int>(spotlightData.visibleLightNodes.size()));
        glBindBufferBase(
            GL_UNIFORM_BUFFER,
            shaderInfo.iSpotlightsUniformBlockBindingIndex,
            spotlightData.pUniformBufferObject->getBufferId());

        // Directional lights.
        glUniform1ui(
            shaderInfo.iDirectionalLightCountUniform,
            static_cast<unsigned int>(directionalLightData.visibleLightNodes.size()));
        glBindBufferBase(
            GL_UNIFORM_BUFFER,
            shaderInfo.iDirectionalLightsUniformBlockBindingIndex,
            directionalLightData.pUniformBufferObject->getBufferId());

        // Bind shadow maps.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, spotShadowMapArray.getTextureId());
        glUniform1i(shaderInfo.iSpotShadowMapsUniform,
                    0); // <- assign texture unit

        // Light culling info.
        glUniform1iv(
            shaderInfo.iIsSpotlightCulledUniform,
            static_cast<int>(lightCullingInfo.vIsSpotlightCulled.size()),
            lightCullingInfo.vIsSpotlightCulled.data());
        glUniform1iv(
            shaderInfo.iIsPointLightCulledUniform,
            static_cast<int>(lightCullingInfo.vIsPointLightCulled.size()),
            lightCullingInfo.vIsPointLightCulled.data());

        // Distance fog.
        glm::vec2 distanceFogRange = glm::vec2(-1.0f, -1.0f); // <- means disabled
        if (optDistanceFog.has_value()) {
            distanceFogRange = optDistanceFog->getFogRange();
            glm::vec3 color = optDistanceFog->getColor();
            glUniform3fv(shaderInfo.iDistanceFogColorUniform, 1, glm::value_ptr(color));
        }
        glUniform2fv(shaderInfo.iDistanceFogRangeUniform, 1, glm::value_ptr(distanceFogRange));

        // Prepare slot for diffuse texture.
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);                   // <- empty for now
        glUniform1i(shaderInfo.iDiffuseTextureUniform, 1); // <- assign texture unit

        // Submit meshes.
        for (unsigned short iMeshDataIndex = shaderInfo.iFirstMeshIndex;
             iMeshDataIndex < shaderInfo.iFirstMeshIndex + shaderInfo.iMeshCount;
             iMeshDataIndex++) {
            const auto& meshData = data.vMeshRenderData[iMeshDataIndex];

            // Frustum culling (don't cull skeletal meshes due to animations).
            if (shaderInfo.iSkinningMatricesUniform == -1 &&
                !cameraFrustum.isAabbInFrustum(meshData.aabbWorld)) {
                continue;
            }

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
