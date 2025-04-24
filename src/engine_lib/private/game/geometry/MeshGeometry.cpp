#include "game/geometry/MeshGeometry.h"

// Standard.
#include <fstream>

// Custom.
#include "misc/Error.h"

// External.
#include "glad/glad.h"

void MeshGeometry::serialize(const std::filesystem::path& pathToFile) const {
    std::ofstream file(pathToFile, std::ios::binary);
    if (!file.is_open()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unable to create file \"{}\"", pathToFile.string()));
    }

    // Write indices first.
    if (vIndices.size() > std::numeric_limits<unsigned int>::max()) [[unlikely]] {
        Error::showErrorAndThrowException("index count exceeds type limit");
    }
    unsigned int iIndexCount = static_cast<unsigned int>(vIndices.size());
    file.write(reinterpret_cast<char*>(&iIndexCount), sizeof(iIndexCount));
    file.write(reinterpret_cast<const char*>(vIndices.data()), iIndexCount * sizeof(vIndices[0]));

    // Prepare vertices.
    std::vector<uint8_t> vPositionData(vVertices.size() * sizeof(MeshVertex::position));
    std::vector<uint8_t> vNormalData(vVertices.size() * sizeof(MeshVertex::normal));
    std::vector<uint8_t> vUvData(vVertices.size() * sizeof(MeshVertex::uv));
    for (size_t iVertexIndex = 0; iVertexIndex < vVertices.size(); iVertexIndex += 1) {
        const auto& vertex = vVertices[iVertexIndex];

        std::memcpy(
            &vPositionData[iVertexIndex * sizeof(MeshVertex::position)],
            glm::value_ptr(vertex.position),
            sizeof(MeshVertex::position));

        std::memcpy(
            &vNormalData[iVertexIndex * sizeof(MeshVertex::normal)],
            glm::value_ptr(vertex.normal),
            sizeof(MeshVertex::normal));

        std::memcpy(
            &vUvData[iVertexIndex * sizeof(MeshVertex::uv)],
            glm::value_ptr(vertex.uv),
            sizeof(MeshVertex::uv));
    }

    // Write vertex count.
    if (vVertices.size() > std::numeric_limits<unsigned int>::max()) [[unlikely]] {
        Error::showErrorAndThrowException("vertex count exceeds type limit");
    }
    unsigned int iVertexCount = static_cast<unsigned int>(vVertices.size());
    file.write(reinterpret_cast<char*>(&iVertexCount), sizeof(iVertexCount));

    // Write vertex data.
    file.write(reinterpret_cast<const char*>(vPositionData.data()), vPositionData.size());
    file.write(reinterpret_cast<const char*>(vNormalData.data()), vNormalData.size());
    file.write(reinterpret_cast<const char*>(vUvData.data()), vUvData.size());

#if defined(WIN32) && defined(DEBUG)
    static_assert(sizeof(MeshGeometry) == 48, "add new variables here"); // NOLINT: current size
#endif
}

MeshGeometry MeshGeometry::deserialize(const std::filesystem::path& pathToFile) {
    std::ifstream file(pathToFile, std::ios::binary);
    if (!file.is_open()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unable to open the file \"{}\"", pathToFile.string()));
    }

    // Get file size.
    file.seekg(0, std::ios::end);
    const size_t iFileSizeInBytes = file.tellg();
    file.seekg(0);

    size_t iReadByteCount = 0;

    // Read index count.
    unsigned int iIndexCount = 0;
    if (iReadByteCount + sizeof(iIndexCount) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    file.read(reinterpret_cast<char*>(&iIndexCount), sizeof(iIndexCount));
    iReadByteCount += sizeof(iIndexCount);

    // Read indices.
    if (iReadByteCount + iIndexCount * sizeof(MeshGeometry::MeshIndexType) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<MeshGeometry::MeshIndexType> vIndices(iIndexCount);
    file.read(reinterpret_cast<char*>(vIndices.data()), vIndices.size() * sizeof(vIndices[0]));
    iReadByteCount += vIndices.size() * sizeof(vIndices[0]);

    // Read vertex count.
    unsigned int iVertexCount = 0;
    if (iReadByteCount + sizeof(iVertexCount) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    file.read(reinterpret_cast<char*>(&iVertexCount), sizeof(iVertexCount));
    iReadByteCount += sizeof(iVertexCount);

    // Read positions.
    if (iReadByteCount + iVertexCount * sizeof(MeshVertex::position) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<glm::vec3> vPositionData(iVertexCount);
    file.read(reinterpret_cast<char*>(vPositionData.data()), vPositionData.size() * sizeof(vPositionData[0]));
    iReadByteCount += vPositionData.size() * sizeof(vPositionData[0]);

    // Read normals.
    if (iReadByteCount + iVertexCount * sizeof(MeshVertex::normal) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<glm::vec3> vNormalData(iVertexCount);
    file.read(reinterpret_cast<char*>(vNormalData.data()), vNormalData.size() * sizeof(vNormalData[0]));
    iReadByteCount += vNormalData.size() * sizeof(vNormalData[0]);

    // Read UVs.
    if (iReadByteCount + iVertexCount * sizeof(MeshVertex::uv) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<glm::vec2> vUvData(iVertexCount);
    file.read(reinterpret_cast<char*>(vUvData.data()), vUvData.size() * sizeof(vUvData[0]));
    iReadByteCount += vUvData.size() * sizeof(vUvData[0]);

    if (iReadByteCount != iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format(
                "read byte count vs file size mismatch {} != {}, file \"{}\"",
                iReadByteCount,
                iFileSizeInBytes,
                pathToFile.string()));
    }

    std::vector<MeshVertex> vVertices(iVertexCount);
    for (size_t i = 0; i < vVertices.size(); i++) {
        vVertices[i].position = vPositionData[i];
        vVertices[i].normal = vNormalData[i];
        vVertices[i].uv = vUvData[i];
    }

#if defined(WIN32) && defined(DEBUG)
    static_assert(sizeof(MeshGeometry) == 48, "add new variables here"); // NOLINT: current size
#endif

    MeshGeometry geometry;
    geometry.vVertices = std::move(vVertices);
    geometry.vIndices = std::move(vIndices);

    return geometry;
}

void MeshVertex::setVertexAttributes() {
    static_assert(
        sizeof(MeshGeometry::MeshIndexType) == sizeof(unsigned short),
        "change index type in renderer's draw command");

    // Prepare offsets of fields.
    const auto iPositionOffset = offsetof(MeshVertex, position);
    const auto iNormalOffset = offsetof(MeshVertex, normal);
    const auto iUvOffset = offsetof(MeshVertex, uv);

    // Specify position.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                                         // attribute index (layout location)
        3,                                         // number of components
        GL_FLOAT,                                  // type of component
        GL_FALSE,                                  // whether data should be normalized or not
        sizeof(MeshVertex),                        // stride (size in bytes between elements)
        reinterpret_cast<void*>(iPositionOffset)); // NOLINT: beginning offset

    // Specify normal.
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                                       // attribute index (layout location)
        3,                                       // number of components
        GL_FLOAT,                                // type of component
        GL_FALSE,                                // whether data should be normalized or not
        sizeof(MeshVertex),                      // stride (size in bytes between elements)
        reinterpret_cast<void*>(iNormalOffset)); // NOLINT: beginning offset

    // Specify UV.
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,                                   // attribute index (layout location)
        2,                                   // number of components
        GL_FLOAT,                            // type of component
        GL_FALSE,                            // whether data should be normalized or not
        sizeof(MeshVertex),                  // stride (size in bytes between elements)
        reinterpret_cast<void*>(iUvOffset)); // NOLINT: beginning offset
}

bool MeshVertex::operator==(const MeshVertex& other) const {
    constexpr auto delta = 0.00001F;

    static_assert(sizeof(MeshVertex) == 32, "add new fields here"); // NOLINT: current size

    return glm::all(glm::epsilonEqual(position, other.position, delta)) &&
           glm::all(glm::epsilonEqual(normal, other.normal, delta)) &&
           glm::all(glm::epsilonEqual(uv, other.uv, delta));
}

bool MeshGeometry::operator==(const MeshGeometry& other) const {
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

#if defined(WIN32) && defined(DEBUG)
    static_assert(sizeof(MeshGeometry) == 48, "add new variables here"); // NOLINT: current size
#endif

    return true;
}
