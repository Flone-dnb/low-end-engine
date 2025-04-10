#include "render/shader/ShaderArrayIndexManager.h"

// Custom.
#include "io/Logger.h"
#include "misc/Error.h"

ShaderArrayIndexManager::ShaderArrayIndexManager(const std::string& sName, unsigned int iArraySize)
    : iArraySize(iArraySize), sName(sName) {
    // Make sure size is not zero.
    if (iArraySize == 0) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("index manager \"{}\" received zero as size", sName));
    }
}

ShaderArrayIndexManager::~ShaderArrayIndexManager() {
    std::scoped_lock guard(mtxData.first);

    // Make sure there are no active (not destroyed) index objects that reference this manager.
    if (mtxData.second.iActiveIndexCount != 0) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "index manager \"{}\" is being destroyed but its counter of active (used) indices is {} "
            "(not zero), this might mean that you release references to used pipeline and only then "
            "release used shader resources while it should be vice versa: release shader resources "
            "first and only then release the pipeline",
            sName,
            mtxData.second.iActiveIndexCount));
    }
}

std::unique_ptr<ShaderArrayIndex> ShaderArrayIndexManager::reserveIndex() {
    std::scoped_lock guard(mtxData.first);

    // Determine what index to return.
    unsigned int iIndexToReturn = 0;

    // See if there are any unused indices.
    if (!mtxData.second.noLongerUsedIndices.empty()) {
        // Use one of the unused indices.
        iIndexToReturn = mtxData.second.noLongerUsedIndices.front();
        mtxData.second.noLongerUsedIndices.pop();
    } else {
        // Generate a new index.
        iIndexToReturn = mtxData.second.iNextFreeIndex;
        mtxData.second.iNextFreeIndex += 1;

        // Make sure we won't hit type limit.
        if (mtxData.second.iNextFreeIndex == UINT_MAX) [[unlikely]] {
            Logger::get().warn(std::format(
                "an index manager \"{}\"reached type limit for next free index of {}",
                sName,
                mtxData.second.iNextFreeIndex));
        }

        // Make sure we don't reach array size limit.
        if (mtxData.second.iNextFreeIndex == iArraySize) [[unlikely]] {
            Logger::get().warn(std::format(
                "index manager \"{}\" just reached array's size limit of {}, the next requested index "
                "(if no unused indices exist) will reference out of array bounds",
                sName,
                iArraySize));
        }
    }

    // Increment the number of active indices.
    mtxData.second.iActiveIndexCount += 1;

    // Return a new index.
    return std::unique_ptr<ShaderArrayIndex>(new ShaderArrayIndex(this, iIndexToReturn));
}

void ShaderArrayIndexManager::onIndexNoLongerUsed(unsigned int iIndex) {
    std::scoped_lock guard(mtxData.first);

    // Make sure the number of active indices will not go below zero.
    if (mtxData.second.iActiveIndexCount == 0) [[unlikely]] {
        Logger::get().error(std::format(
            "some index object ({}) notified owner index manager \"{}\" about no longer being used "
            "but index manager's counter of active (used) indices is already zero",
            sName,
            iIndex));
        return;
    }

    // Decrement the number of active indices.
    mtxData.second.iActiveIndexCount -= 1;

    // Add to queue of unused indices.
    mtxData.second.noLongerUsedIndices.push(iIndex);
}

ShaderArrayIndex::ShaderArrayIndex(ShaderArrayIndexManager* pManager, unsigned int iIndexIntoShaderArray) {
    this->pManager = pManager;
    this->iIndexIntoShaderArray = iIndexIntoShaderArray;
}

unsigned int ShaderArrayIndex::getActualIndex() const { return iIndexIntoShaderArray; }

ShaderArrayIndex::~ShaderArrayIndex() { pManager->onIndexNoLongerUsed(iIndexIntoShaderArray); }
