#include "render/shader/LightSourceShaderArray.h"

// Standard.
#include <format>

// Custom.
#include "game/node/Node.h"
#include "misc/Error.h"
#include "render/LightSourceManager.h"
#include "render/GpuResourceManager.h"
#include "render/ShaderAlignmentConstants.hpp"

LightSourceShaderArray::LightSourceShaderArray(
    LightSourceManager* pLightSourceManager,
    unsigned int iLightStructSizeInBytes,
    unsigned int iArrayMaxSize,
    const std::string& sUniformBlockName,
    const std::string& sLightCountUniformName)
    : pLightSourceManager(pLightSourceManager), iActualLightStructSize(iLightStructSizeInBytes),
      iArrayMaxSize(iArrayMaxSize), sUniformBlockName(sUniformBlockName),
      sLightCountUniformName(sLightCountUniformName) {
    // GLSL array elements must be multiple of vec4 (because we create an array of lights).
    if (iLightStructSizeInBytes % ShaderAlignmentConstants::iVec4 != 0) {
        if (iLightStructSizeInBytes < ShaderAlignmentConstants::iVec4) {
            iLightStructSizeInBytes += ShaderAlignmentConstants::iVec4 - iLightStructSizeInBytes;
        } else {
            iLightStructSizeInBytes +=
                ShaderAlignmentConstants::iVec4 - (iLightStructSizeInBytes % ShaderAlignmentConstants::iVec4);
        }
    }
    iPaddedLightStructSize = iLightStructSizeInBytes;

    std::scoped_lock guard(mtxData.first);

    // Create index manager.
    mtxData.second.pArrayIndexManager =
        std::make_unique<ShaderArrayIndexManager>(sUniformBlockName, iArrayMaxSize);

    // Create UBO for shaders.
    mtxData.second.pUniformBufferObject =
        GpuResourceManager::createUniformBuffer(iLightStructSizeInBytes * iArrayMaxSize, true);
}

LightSourceShaderArray::~LightSourceShaderArray() {
    std::scoped_lock guard(mtxData.first);

    const auto iLightSourceCount = mtxData.second.visibleLightNodes.size();
    if (iLightSourceCount != 0) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "light source array is being destroyed but there are still {} light source(s) active",
            iLightSourceCount));
    }
}

std::unique_ptr<ActiveLightSourceHandle>
LightSourceShaderArray::addLightSourceToRendering(Node* pLightSource, const void* pProperties) {
    std::unique_ptr<ActiveLightSourceHandle> pHandle;

    {
        std::scoped_lock guard(mtxData.first);

        if (mtxData.second.visibleLightNodes.size() == iArrayMaxSize) {
            Log::warn(std::format(
                "light array \"{}\" is unable to add the light node \"{}\" to be rendered because the "
                "array has reached the maximum number of visible light sources of {}",
                sUniformBlockName,
                pLightSource->getNodeName(),
                iArrayMaxSize));
            return nullptr;
        }

        // Add light.
        const auto [it, isAdded] = mtxData.second.visibleLightNodes.insert(pLightSource);
        if (!isAdded) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("light node \"{}\" is already added to rendering", pLightSource->getNodeName()));
        }

        pHandle = std::unique_ptr<ActiveLightSourceHandle>(new ActiveLightSourceHandle(
            this, mtxData.second.pArrayIndexManager->reserveIndex(), pLightSource));
    }

    // Copy initial data.
    pHandle->copyNewProperties(pProperties);

    return pHandle;
}

void LightSourceShaderArray::removeLightSourceFromRendering(Node* pLightSource) {
    std::scoped_lock guard(mtxData.first);

    const auto it = mtxData.second.visibleLightNodes.find(pLightSource);
    if (it == mtxData.second.visibleLightNodes.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("light node \"{}\" is not found", pLightSource->getNodeName()));
    }

    mtxData.second.visibleLightNodes.erase(it);
}

ActiveLightSourceHandle::~ActiveLightSourceHandle() { pArray->removeLightSourceFromRendering(pLightNode); }

void ActiveLightSourceHandle::copyNewProperties(const void* pData) {
    std::scoped_lock guard(pArray->mtxData.first);

    // Copy new data.
    // TODO: Doing this instantly (upon request) is not a very clever idea since it will be
    // best to queue this request and process them all later at the same time (before rendering a frame)
    // but since light sources generally don't change their properties that often (during a single frame) I
    // think it's not that bad. Plus this keeps the code simple.
    pArray->mtxData.second.pUniformBufferObject->copyDataToBuffer(
        pArray->iPaddedLightStructSize * pArrayIndex->getActualIndex(),
        pArray->iActualLightStructSize,
        pData);
}
