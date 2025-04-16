#include "io/Serializable.h"

// Standard.
#include <format>

// Custom.
#include "io/Logger.h"
#include "misc/Error.h"

std::optional<Error> Serializable::serialize(
    std::filesystem::path pathToFile,
    bool bEnableBackup,
    const std::unordered_map<std::string, std::string>& customAttributes) {
    // Add TOML extension to file.
    if (!pathToFile.string().ends_with(".toml")) {
        pathToFile += ".toml";
    }

    // Make sure file directories exist.
    if (!std::filesystem::exists(pathToFile.parent_path())) {
        std::filesystem::create_directories(pathToFile.parent_path());
    }

#if defined(WIN32)
    // Check if the path length is too long.
    constexpr auto iMaxPathLimitBound = 15;
    constexpr auto iMaxPath = 260; // value of Windows' MAXPATH macro
    constexpr auto iMaxPathLimit = iMaxPath - iMaxPathLimitBound;
    const auto iFilePathLength = pathToFile.string().length();
    if (iFilePathLength > iMaxPathLimit - (iMaxPathLimitBound * 2) && iFilePathLength < iMaxPathLimit) {
        Logger::get().warn(
            std::format(
                "file path length {} is close to the platform limit of {} characters (path: {})",
                iFilePathLength,
                iMaxPathLimit,
                pathToFile.string()));
    } else if (iFilePathLength >= iMaxPathLimit) {
        return Error(
            std::format(
                "file path length {} exceeds the platform limit of {} characters (path: {})",
                iFilePathLength,
                iMaxPathLimit,
                pathToFile.string()));
    }
#endif

    // Serialize data to TOML value.
    toml::value tomlData;
    auto result = serialize(pathToFile, tomlData, nullptr, "", customAttributes);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        return error;
    }

    // Handle backup file.
    std::filesystem::path backupFile = pathToFile;
    backupFile += ConfigManager::getBackupFileExtension();

    if (bEnableBackup) {
        // Check if we already have a file from previous serialization.
        if (std::filesystem::exists(pathToFile)) {
            // Make this old file a backup file.
            if (std::filesystem::exists(backupFile)) {
                std::filesystem::remove(backupFile);
            }
            std::filesystem::rename(pathToFile, backupFile);
        }
    }

    // Save TOML data to file.
    std::ofstream file(pathToFile, std::ios::binary);
    if (!file.is_open()) [[unlikely]] {
        return Error(
            std::format(
                "failed to open the file \"{}\" (maybe because it's marked as read-only)",
                pathToFile.string()));
    }
    file << toml::format(tomlData);
    file.close();

    if (bEnableBackup) {
        // Create backup file if it does not exist.
        if (!std::filesystem::exists(backupFile)) {
            std::filesystem::copy_file(pathToFile, backupFile);
        }
    }

    return {};
}

std::variant<std::string, Error> Serializable::serialize(
    std::filesystem::path pathToFile,
    toml::value& tomlData,
    Serializable* pOriginalObject,
    std::string sEntityId,
    const std::unordered_map<std::string, std::string>& customAttributes) {
    if (sEntityId.empty()) {
        // Put something as entity ID so it would not look weird.
        sEntityId = "0";
    }

    // Check that custom attribute key names are not empty.
    if (!customAttributes.empty() && customAttributes.find("") != customAttributes.end()) {
        return Error("empty attributes are not allowed");
    }

    // If the original object is specified we want to only serialize changed values and then write a path to
    // the original object (to deserialize unchanged values from there) but we don't require the original
    // object to have "path deserialized from" to exist because this function can be called for a field of
    // type `Serializable` which could have an original object (if field's owner has original object) but
    // fields don't have path to the file they were deserialized from.

    // Prepare TOML section name.
    const auto sTypeGuid = getTypeGuid();
    auto sSectionName = std::format("{}.{}", sEntityId, sTypeGuid);

    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);
    if (sTypeGuid.empty()) [[unlikely]] {
        return Error(std::format("type \"{}\" has empty GUID", typeInfo.sTypeName));
    }

    // Serialize only changed values.
    {
        // Define a helper macro.
#define COMPARE_AND_ADD_TO_TOML(array)                                                                       \
    for (const auto& [sVariableName, variableInfo] : array) {                                                \
        const auto currentValue = variableInfo.getter(this);                                                 \
        if (pOriginalObject != nullptr && variableInfo.getter(pOriginalObject) == currentValue) {            \
            continue;                                                                                        \
        }                                                                                                    \
        tomlData[sSectionName][sVariableName] = currentValue;                                                \
    }
        COMPARE_AND_ADD_TO_TOML(typeInfo.reflectedVariables.bools);
        COMPARE_AND_ADD_TO_TOML(typeInfo.reflectedVariables.ints);
        COMPARE_AND_ADD_TO_TOML(typeInfo.reflectedVariables.unsignedInts);
        COMPARE_AND_ADD_TO_TOML(typeInfo.reflectedVariables.longLongs);

        constexpr float floatEpsilon = 0.00001f;

        // Unsigned long long.
        for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.unsignedLongLongs) {
            const auto currentValue = variableInfo.getter(this);
            if (pOriginalObject != nullptr && variableInfo.getter(pOriginalObject) == currentValue) {
                continue;
            }
            // Store as a string because toml11 uses `long long` for integers.
            tomlData[sSectionName][sVariableName] = std::to_string(currentValue);
        }

        // Floats.
        for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.floats) {
            const auto currentValue = variableInfo.getter(this);
            if (pOriginalObject != nullptr &&
                (fabs(variableInfo.getter(pOriginalObject) - currentValue) < floatEpsilon)) {
                continue;
            }
            // Stored as string for better precision.
            tomlData[sSectionName][sVariableName] = currentValue;
        }

        // String.
        for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.strings) {
            const auto currentValue = variableInfo.getter(this);
            if (pOriginalObject != nullptr && variableInfo.getter(pOriginalObject) == currentValue) {
                continue;
            }
            tomlData[sSectionName][sVariableName] = currentValue;
        }

        // Vec2.
        for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.vec2s) {
            const auto currentValue = variableInfo.getter(this);
            if (pOriginalObject != nullptr &&
                glm::all(
                    glm::epsilonEqual(variableInfo.getter(pOriginalObject), currentValue, floatEpsilon))) {
                continue;
            }
            std::array<float, 2> vArray = {currentValue.x, currentValue.y};
            tomlData[sSectionName][sVariableName] = vArray;
        }

        // Vec3.
        for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.vec3s) {
            const auto currentValue = variableInfo.getter(this);
            if (pOriginalObject != nullptr &&
                glm::all(
                    glm::epsilonEqual(variableInfo.getter(pOriginalObject), currentValue, floatEpsilon))) {
                continue;
            }
            std::array<float, 3> vArray = {currentValue.x, currentValue.y, currentValue.z};
            tomlData[sSectionName][sVariableName] = vArray;
        }

        // Vec4.
        for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.vec4s) {
            const auto currentValue = variableInfo.getter(this);
            if (pOriginalObject != nullptr &&
                glm::all(
                    glm::epsilonEqual(variableInfo.getter(pOriginalObject), currentValue, floatEpsilon))) {
                continue;
            }
            std::array<float, 4> vArray = {currentValue.x, currentValue.y, currentValue.z, currentValue.w};
            tomlData[sSectionName][sVariableName] = vArray;
        }

        // Vector<int>
        for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.vectorInts) {
            const auto currentValue = variableInfo.getter(this);
            if (pOriginalObject != nullptr && variableInfo.getter(pOriginalObject) == currentValue) {
                continue;
            }
            tomlData[sSectionName][sVariableName] = currentValue;
        }

        // Vector<string>
        for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.vectorStrings) {
            const auto currentValue = variableInfo.getter(this);
            if (pOriginalObject != nullptr && variableInfo.getter(pOriginalObject) == currentValue) {
                continue;
            }
            tomlData[sSectionName][sVariableName] = currentValue;
        }

        // Vector<vec3>
        for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.vectorVec3s) {
            const auto vCurrentValue = variableInfo.getter(this);
            if (pOriginalObject != nullptr) {
                const auto vOriginalValue = variableInfo.getter(pOriginalObject);
                if (vOriginalValue.size() == vCurrentValue.size()) {
                    bool bEqual = true;
                    for (size_t i = 0; i < vOriginalValue.size(); i++) {
                        bEqual =
                            bEqual &&
                            glm::all(glm::epsilonEqual(vOriginalValue[i], vCurrentValue[i], floatEpsilon));
                    }
                    if (bEqual) {
                        continue;
                    }
                }
            }
            // Store as array of strings for better precision.
            std::vector<float> vArray;
            vArray.resize(vCurrentValue.size() * 3);
            for (size_t iOriginalIndex = 0, iFlatIndex = 0; iFlatIndex < vArray.size();
                 iOriginalIndex += 1, iFlatIndex += 3) {
                vArray[iFlatIndex] = vCurrentValue[iOriginalIndex].x;
                vArray[iFlatIndex + 1] = vCurrentValue[iOriginalIndex].y;
                vArray[iFlatIndex + 2] = vCurrentValue[iOriginalIndex].z;
            }
            tomlData[sSectionName][sVariableName] = vArray;
        }

        // MeshGeometry
        if (!typeInfo.reflectedVariables.meshGeometries.empty()) {
            if (!pathToFile.has_parent_path()) [[unlikely]] {
                return Error(std::format("expected a parent path to exist for \"{}\"", pathToFile.string()));
            }

            if (!std::filesystem::exists(pathToFile.parent_path())) {
                std::filesystem::create_directories(pathToFile.parent_path());
            }
            auto basePathToBinaryFile = pathToFile.parent_path() / pathToFile.stem().string();

            for (const auto& [sVariableName, variableInfo] : typeInfo.reflectedVariables.meshGeometries) {
                const auto currentValue = variableInfo.getter(this);
                if (currentValue.getIndices().empty() && currentValue.getVertices().empty()) {
                    continue;
                }
                if (pOriginalObject != nullptr && variableInfo.getter(pOriginalObject) == currentValue) {
                    continue;
                }

                currentValue.serialize(basePathToBinaryFile.string() + "." + sVariableName);
            }
        }

#if defined(WIN32) && defined(DEBUG)
        static_assert(sizeof(TypeReflectionInfo) == 1344, "add new variables here"); // NOLINT: current size
#endif
    }

    if (pOriginalObject != nullptr && pOriginalObject->getPathDeserializedFromRelativeToRes().has_value()) {
        // Write path to the original entity.
        tomlData[sSectionName][sTomlKeyPathRelativeToRes.data()] =
            (*pOriginalObject->getPathDeserializedFromRelativeToRes()).first;
    }

    // Write custom attributes, they will be written with two dots in the beginning.
    for (const auto& [sKey, sValue] : customAttributes) {
        tomlData[sSectionName][std::string(sTomlKeyCustomAttributePrefix) + sKey] = sValue;
    }

    return sSectionName;
}

std::optional<std::pair<std::string, std::string>>
Serializable::getPathDeserializedFromRelativeToRes() const {
    return pathDeserializedFromRelativeToRes;
}

std::optional<Error> Serializable::resolvePathToToml(std::filesystem::path& pathToFile) {
    // Add TOML extension to file.
    if (!pathToFile.string().ends_with(".toml")) {
        pathToFile += ".toml";
    }

    // Prepare path to backup file.
    const std::filesystem::path backupFile = pathToFile.string() + ConfigManager::getBackupFileExtension();

    // Check file original file exists.
    if (std::filesystem::exists(pathToFile)) {
        // Path is ready.
        return {};
    }

    // Make sure a backup file exists.
    if (!std::filesystem::exists(backupFile)) [[unlikely]] {
        return Error("requested file or a backup file do not exist");
    }

    // Duplicate and rename backup file to be original file.
    std::filesystem::copy_file(backupFile, pathToFile);

    return {};
}
