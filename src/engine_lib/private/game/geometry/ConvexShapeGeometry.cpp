#include "game/geometry/ConvexShapeGeometry.h"

// Standard.
#include <fstream>
#include <format>

// Custom.
#include "misc/Error.h"

void ConvexShapeGeometry::serialize(const std::filesystem::path& pathToFile) const {
    std::ofstream file(pathToFile, std::ios::binary);
    if (!file.is_open()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unable to create file \"{}\"", pathToFile.string()));
    }

    // Write position count.
    if (vPositions.size() > std::numeric_limits<unsigned int>::max()) [[unlikely]] {
        Error::showErrorAndThrowException("position count exceeds type limit");
    }
    unsigned int iPosCount = static_cast<unsigned int>(vPositions.size());
    file.write(reinterpret_cast<char*>(&iPosCount), sizeof(iPosCount));

    // Write position data.
    if (!vPositions.empty()) {
        file.write(
            reinterpret_cast<const char*>(vPositions.data()),
            static_cast<long>(vPositions.size() * sizeof(vPositions[0])));
    }

#if defined(DEBUG) && defined(WIN32)
    static_assert(sizeof(ConvexShapeGeometry) == 24, "add new variables here");
#elif defined(DEBUG)
    static_assert(sizeof(ConvexShapeGeometry) == 24, "add new variables here");
#endif
}

ConvexShapeGeometry ConvexShapeGeometry::deserialize(const std::filesystem::path& pathToFile) {
    std::ifstream file(pathToFile, std::ios::binary);
    if (!file.is_open()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unable to open the file \"{}\"", pathToFile.string()));
    }

    // Get file size.
    file.seekg(0, std::ios::end);
    const size_t iFileSizeInBytes = static_cast<size_t>(file.tellg());
    file.seekg(0);

    size_t iReadByteCount = 0;

    // Read position count.
    unsigned int iPosCount = 0;
    if (iReadByteCount + sizeof(iPosCount) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    file.read(reinterpret_cast<char*>(&iPosCount), sizeof(iPosCount));
    iReadByteCount += sizeof(iPosCount);

    // Read positions.
    if (iReadByteCount + iPosCount * sizeof(glm::vec3) > iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format("unexpected end of file \"{}\"", pathToFile.string()));
    }
    std::vector<glm::vec3> vPositionData(iPosCount);
    file.read(
        reinterpret_cast<char*>(vPositionData.data()),
        static_cast<long>(vPositionData.size() * sizeof(vPositionData[0])));
    iReadByteCount += vPositionData.size() * sizeof(vPositionData[0]);

    if (iReadByteCount != iFileSizeInBytes) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "read byte count vs file size mismatch {} != {}, file \"{}\"",
            iReadByteCount,
            iFileSizeInBytes,
            pathToFile.string()));
    }

#if defined(DEBUG) && defined(WIN32)
    static_assert(sizeof(ConvexShapeGeometry) == 24, "add new variables here");
#elif defined(DEBUG)
    static_assert(sizeof(ConvexShapeGeometry) == 24, "add new variables here");
#endif

    ConvexShapeGeometry geometry;
    geometry.vPositions = std::move(vPositionData);

    return geometry;
}
