#include "game/geometry/SkeletalMeshNodeGeometry.h"

// Standard.
#include <fstream>
#include <format>
#include <cstdint>

// Custom.
#include "misc/Error.h"

// External.
#include "glad/glad.h"

namespace {
    constexpr uint16_t iSupportedFileVersion = 0;
}

void SkeletalMeshNodeGeometry::serialize(const std::filesystem::path& pathToFile) const {
    std::ofstream file(pathToFile, std::ios::binary);
    if (!file.is_open()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unable to create file \"{}\"", pathToFile.string()));
    }

    // Write file version.
    file.write(reinterpret_cast<const char*>(&iSupportedFileVersion), sizeof(iSupportedFileVersion));

    // Write indices.
    if (vIndices.size() > std::numeric_limits<unsigned int>::max()) [[unlikely]] {
        Error::showErrorAndThrowException("index count exceeds type limit");
    }
    unsigned int iIndexCount = static_cast<unsigned int>(vIndices.size());
    file.write(reinterpret_cast<char*>(&iIndexCount), sizeof(iIndexCount));
    file.write(reinterpret_cast<const char*>(vIndices.data()), iIndexCount * sizeof(vIndices[0]));

    // Prepare arrays to fill.
    std::vector<uint8_t> vPositionData(vVertices.size() * sizeof(SkeletalMeshNodeVertex::position));
    std::vector<uint8_t> vNormalData(vVertices.size() * sizeof(SkeletalMeshNodeVertex::normal));
    std::vector<uint8_t> vUvData(vVertices.size() * sizeof(SkeletalMeshNodeVertex::uv));
    std::vector<uint8_t> vBoneIndicesData(vVertices.size() * sizeof(SkeletalMeshNodeVertex::vBoneIndices));
    std::vector<uint8_t> vBoneWeightsData(vVertices.size() * sizeof(SkeletalMeshNodeVertex::vBoneWeights));

    for (size_t iVertexIndex = 0; iVertexIndex < vVertices.size(); iVertexIndex += 1) {
        const auto& vertex = vVertices[iVertexIndex];

        std::memcpy(
            &vPositionData[iVertexIndex * sizeof(SkeletalMeshNodeVertex::position)],
            glm::value_ptr(vertex.position),
            sizeof(SkeletalMeshNodeVertex::position));

        std::memcpy(
            &vNormalData[iVertexIndex * sizeof(SkeletalMeshNodeVertex::normal)],
            glm::value_ptr(vertex.normal),
            sizeof(SkeletalMeshNodeVertex::normal));

        std::memcpy(
            &vUvData[iVertexIndex * sizeof(SkeletalMeshNodeVertex::uv)],
            glm::value_ptr(vertex.uv),
            sizeof(SkeletalMeshNodeVertex::uv));

        std::memcpy(
            &vBoneIndicesData[iVertexIndex * sizeof(SkeletalMeshNodeVertex::vBoneIndices)],
            vertex.vBoneIndices.data(),
            sizeof(SkeletalMeshNodeVertex::vBoneIndices));

        std::memcpy(
            &vBoneWeightsData[iVertexIndex * sizeof(SkeletalMeshNodeVertex::vBoneWeights)],
            vertex.vBoneWeights.data(),
            sizeof(SkeletalMeshNodeVertex::vBoneWeights));
    }

    // Write vertex count.
    if (vVertices.size() > std::numeric_limits<unsigned int>::max()) [[unlikely]] {
        Error::showErrorAndThrowException("vertex count exceeds type limit");
    }
    unsigned int iVertexCount = static_cast<unsigned int>(vVertices.size());
    file.write(reinterpret_cast<char*>(&iVertexCount), sizeof(iVertexCount));

    // Write vertex data.
    file.write(reinterpret_cast<const char*>(vPositionData.data()), static_cast<long>(vPositionData.size()));
    file.write(reinterpret_cast<const char*>(vNormalData.data()), static_cast<long>(vNormalData.size()));
    file.write(reinterpret_cast<const char*>(vUvData.data()), static_cast<long>(vUvData.size()));
    file.write(
        reinterpret_cast<const char*>(vBoneIndicesData.data()), static_cast<long>(vBoneIndicesData.size()));
    file.write(
        reinterpret_cast<const char*>(vBoneWeightsData.data()), static_cast<long>(vBoneWeightsData.size()));

#if defined(DEBUG)
    static_assert(sizeof(SkeletalMeshNodeVertex) == 52, "add new variables here");
    static_assert(sizeof(SkeletalMeshNodeGeometry) == 48, "add new variables here");
#endif
}

SkeletalMeshNodeGeometry SkeletalMeshNodeGeometry::deserialize(const std::filesystem::path& pathToFile) {
    std::ifstream file(pathToFile, std::ios::binary);
    if (!file.is_open()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unable to open the file \"{}\"", pathToFile.string()));
    }

    // Get file size.
    file.seekg(0, std::ios::end);
    const size_t iFileSizeInBytes = static_cast<size_t>(file.tellg());
    file.seekg(0);

    size_t iReadByteCount = 0;

    // Read file version.
    uint16_t iFileVersion = 0;
    file.read(reinterpret_cast<char*>(&iFileVersion), sizeof(iFileVersion));
    iReadByteCount += sizeof(iFileVersion);

    // Check file version.
    // TODO: add backwards compatibility here when needed.
    if (iSupportedFileVersion != iFileVersion) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "file \"{}\" has unsupported format version {} while the supported version is {}",
            pathToFile.string(),
            iFileVersion,
            iSupportedFileVersion));
    }

    // Read index count.
    unsigned int iIndexCount = 0;
    if (iReadByteCount + sizeof(iIndexCount) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    file.read(reinterpret_cast<char*>(&iIndexCount), sizeof(iIndexCount));
    iReadByteCount += sizeof(iIndexCount);

    // Read indices.
    if (iReadByteCount + iIndexCount * sizeof(MeshIndexType) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<MeshIndexType> vIndices(iIndexCount);
    file.read(
        reinterpret_cast<char*>(vIndices.data()), static_cast<long>(vIndices.size() * sizeof(vIndices[0])));
    iReadByteCount += vIndices.size() * sizeof(vIndices[0]);

    // Read vertex count.
    unsigned int iVertexCount = 0;
    if (iReadByteCount + sizeof(iVertexCount) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    file.read(reinterpret_cast<char*>(&iVertexCount), sizeof(iVertexCount));
    iReadByteCount += sizeof(iVertexCount);

    // Read positions.
    if (iReadByteCount + iVertexCount * sizeof(SkeletalMeshNodeVertex::position) > iFileSizeInBytes)
        [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<glm::vec3> vPositionData(iVertexCount);
    file.read(
        reinterpret_cast<char*>(vPositionData.data()),
        static_cast<long>(vPositionData.size() * sizeof(vPositionData[0])));
    iReadByteCount += vPositionData.size() * sizeof(vPositionData[0]);

    // Read normals.
    if (iReadByteCount + iVertexCount * sizeof(SkeletalMeshNodeVertex::normal) > iFileSizeInBytes)
        [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<glm::vec3> vNormalData(iVertexCount);
    file.read(
        reinterpret_cast<char*>(vNormalData.data()),
        static_cast<long>(vNormalData.size() * sizeof(vNormalData[0])));
    iReadByteCount += vNormalData.size() * sizeof(vNormalData[0]);

    // Read UVs.
    if (iReadByteCount + iVertexCount * sizeof(SkeletalMeshNodeVertex::uv) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<glm::vec2> vUvData(iVertexCount);
    file.read(
        reinterpret_cast<char*>(vUvData.data()), static_cast<long>(vUvData.size() * sizeof(vUvData[0])));
    iReadByteCount += vUvData.size() * sizeof(vUvData[0]);

    // Read bone indices.
    if (iReadByteCount + iVertexCount * sizeof(SkeletalMeshNodeVertex::vBoneIndices) > iFileSizeInBytes)
        [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<std::array<SkeletalMeshNodeVertex::BoneIndexType, 4>> vBoneIndicesData(iVertexCount);
    file.read(
        reinterpret_cast<char*>(vBoneIndicesData.data()),
        static_cast<long>(vBoneIndicesData.size() * sizeof(vBoneIndicesData[0])));
    iReadByteCount += vBoneIndicesData.size() * sizeof(vBoneIndicesData[0]);

    // Read bone indices.
    if (iReadByteCount + iVertexCount * sizeof(SkeletalMeshNodeVertex::vBoneWeights) > iFileSizeInBytes)
        [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<std::array<float, 4>> vBoneWeightsData(iVertexCount);
    file.read(
        reinterpret_cast<char*>(vBoneWeightsData.data()),
        static_cast<long>(vBoneWeightsData.size() * sizeof(vBoneWeightsData[0])));
    iReadByteCount += vBoneWeightsData.size() * sizeof(vBoneWeightsData[0]);

    // Make sure we read all data.
    if (iReadByteCount != iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "read byte count vs file size mismatch {} != {}, file \"{}\"",
            iReadByteCount,
            iFileSizeInBytes,
            pathToFile.string()));
    }

    std::vector<SkeletalMeshNodeVertex> vVertices(iVertexCount);
    for (size_t i = 0; i < vVertices.size(); i++) {
        vVertices[i].position = vPositionData[i];
        vVertices[i].normal = vNormalData[i];
        vVertices[i].uv = vUvData[i];
        vVertices[i].vBoneIndices = vBoneIndicesData[i];
        vVertices[i].vBoneWeights = vBoneWeightsData[i];
    }

#if defined(DEBUG)
    //                        ALSO UPDATE FILE VERSION and backwards compatibility checks
    static_assert(sizeof(SkeletalMeshNodeVertex) == 52, "add new variables here");
    static_assert(sizeof(SkeletalMeshNodeGeometry) == 48, "add new variables here");
#endif

    SkeletalMeshNodeGeometry geometry;
    geometry.vVertices = std::move(vVertices);
    geometry.vIndices = std::move(vIndices);

    return geometry;
}

void SkeletalMeshNodeVertex::setVertexAttributes() {
    // Prepare offsets of fields.
    const auto iPositionOffset = offsetof(SkeletalMeshNodeVertex, position);
    const auto iNormalOffset = offsetof(SkeletalMeshNodeVertex, normal);
    const auto iUvOffset = offsetof(SkeletalMeshNodeVertex, uv);
    const auto iBoneIndicesOffset = offsetof(SkeletalMeshNodeVertex, vBoneIndices);
    const auto iBoneWeightOffset = offsetof(SkeletalMeshNodeVertex, vBoneWeights);

    // Position.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                                         // attribute index (layout location)
        3,                                         // number of components
        GL_FLOAT,                                  // type of component
        GL_FALSE,                                  // whether data should be normalized or not
        sizeof(SkeletalMeshNodeVertex),            // stride (size in bytes between elements)
        reinterpret_cast<void*>(iPositionOffset)); // beginning offset

    // Normal.
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                                       // attribute index (layout location)
        3,                                       // number of components
        GL_FLOAT,                                // type of component
        GL_FALSE,                                // whether data should be normalized or not
        sizeof(SkeletalMeshNodeVertex),          // stride (size in bytes between elements)
        reinterpret_cast<void*>(iNormalOffset)); // beginning offset

    // UV.
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,                                   // attribute index (layout location)
        2,                                   // number of components
        GL_FLOAT,                            // type of component
        GL_FALSE,                            // whether data should be normalized or not
        sizeof(SkeletalMeshNodeVertex),      // stride (size in bytes between elements)
        reinterpret_cast<void*>(iUvOffset)); // beginning offset

    // Joint indices.
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(                           // <- note `I` here, passing array of integers
        3,                                            // attribute index (layout location)
        4,                                            // number of components
        GL_UNSIGNED_BYTE,                             // type of component
        sizeof(SkeletalMeshNodeVertex),               // stride (size in bytes between elements)
        reinterpret_cast<void*>(iBoneIndicesOffset)); // beginning offset

    // Joint weights.
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4,                                           // attribute index (layout location)
        4,                                           // number of components
        GL_FLOAT,                                    // type of component
        GL_FALSE,                                    // whether data should be normalized or not
        sizeof(SkeletalMeshNodeVertex),              // stride (size in bytes between elements)
        reinterpret_cast<void*>(iBoneWeightOffset)); // beginning offset
}

bool SkeletalMeshNodeVertex::operator==(const SkeletalMeshNodeVertex& other) const {
    constexpr auto delta = 0.00001f;

#if defined(DEBUG)
    static_assert(sizeof(SkeletalMeshNodeVertex) == 52, "add new fields here");
#endif

    const auto bPosEqual = glm::all(glm::epsilonEqual(position, other.position, delta));
    const auto bNormalEqual = glm::all(glm::epsilonEqual(position, other.position, delta));
    const auto bUvEqual = glm::all(glm::epsilonEqual(uv, other.uv, delta));

    bool bJointIndicesEqual = true;
    for (size_t i = 0; i < vBoneIndices.size(); i++) {
        if (vBoneIndices[i] != other.vBoneIndices[i]) {
            bJointIndicesEqual = false;
            break;
        }
    }

    bool bJointWeightsEqual = true;
    for (size_t i = 0; i < vBoneWeights.size(); i++) {
        if (vBoneWeights[i] != other.vBoneWeights[i]) {
            bJointWeightsEqual = false;
            break;
        }
    }

    return bPosEqual && bNormalEqual && bUvEqual && bJointIndicesEqual && bJointWeightsEqual;
}

bool SkeletalMeshNodeGeometry::operator==(const SkeletalMeshNodeGeometry& other) const {
    if (vVertices.size() != other.vVertices.size()) {
        return false;
    }
    if (vVertices != other.vVertices) {
        return false;
    }

    if (vIndices.size() != other.vIndices.size()) {
        return false;
    }
    if (vIndices != other.vIndices) {
        return false;
    }

#if defined(DEBUG)
    static_assert(sizeof(SkeletalMeshNodeGeometry) == 48, "add new variables here");
#endif

    return true;
}
