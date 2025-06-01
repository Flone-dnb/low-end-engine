#include "io/Serializable.h"

// Standard.
#include <format>

// Custom.
#include "io/Logger.h"
#include "misc/Error.h"

// To suppress GCC's false-positive about dangling reference.
#if __GNUC__ >= 13
#define GCC_PUSH_DIAGNOSTIC_DISABLE_DANGLING_REF                                                             \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdangling-reference\"")

#define GCC_DIAGNOSTIC_POP _Pragma("GCC diagnostic pop")
#else
#define GCC_PUSH_DIAGNOSTIC_DISABLE_DANGLING_REF
#define GCC_DIAGNOSTIC_POP
#endif

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
        Logger::get().warn(std::format(
            "file path length {} is close to the platform limit of {} characters (path: {})",
            iFilePathLength,
            iMaxPathLimit,
            pathToFile.string()));
    } else if (iFilePathLength >= iMaxPathLimit) {
        return Error(std::format(
            "file path length {} exceeds the platform limit of {} characters (path: {})",
            iFilePathLength,
            iMaxPathLimit,
            pathToFile.string()));
    }
#endif

    std::unique_ptr<Serializable> pOriginalObject;
    if (getPathDeserializedFromRelativeToRes().has_value()) {
        // Get path and ID.
        const auto sPathDeserializedFromRelativeRes = getPathDeserializedFromRelativeToRes()->first;
        const auto sObjectIdInDeserializedFile = getPathDeserializedFromRelativeToRes()->second;

        auto pathToOriginal =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathDeserializedFromRelativeRes;
        if (!pathToOriginal.string().ends_with(".toml")) {
            pathToOriginal += ".toml";
        }

        // Make sure to not use an original object if the same file is being overwritten.
        if (!std::filesystem::exists(pathToFile) ||
            std::filesystem::canonical(pathToFile) != std::filesystem::canonical(pathToOriginal)) {
            // This object was previously deserialized from the `res` directory and now is serialized
            // into a different file in the `res` directory.

            // We should only serialize fields with changed values and additionally serialize
            // the path to the original file so that the rest of the fields can be deserialized from that
            // file.

            // Check that the original file exists.
            if (!std::filesystem::exists(pathToOriginal)) [[unlikely]] {
                GCC_PUSH_DIAGNOSTIC_DISABLE_DANGLING_REF
                const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(getTypeGuid());
                return Error(std::format(
                    "object of type \"{}\" has the path it was deserialized from ({}, ID {}) but "
                    "this "
                    "file \"{}\" does not exist",
                    typeInfo.sTypeName,
                    sPathDeserializedFromRelativeRes,
                    sObjectIdInDeserializedFile,
                    pathToOriginal.string()));
                GCC_DIAGNOSTIC_POP
            }

            // Deserialize the original.
            std::unordered_map<std::string, std::string> customAttributes;
            auto result =
                deserialize<Serializable>(pathToOriginal, sObjectIdInDeserializedFile, customAttributes);
            if (std::holds_alternative<Error>(result)) [[unlikely]] {
                auto error = std::get<Error>(result);
                error.addCurrentLocationToErrorStack();
                return error;
            }
            // Save original object to only save changed fields later.
            pOriginalObject = std::get<std::unique_ptr<Serializable>>(std::move(result));
        }
    }

    // Serialize data to TOML value.
    toml::value tomlData;
    auto result = serialize(pathToFile, tomlData, pOriginalObject.get(), "", customAttributes);
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
        return Error(std::format(
            "failed to open the file \"{}\" (maybe because it's marked as read-only)", pathToFile.string()));
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

std::optional<Error> Serializable::serializeMultiple(
    std::filesystem::path pathToFile,
    const std::vector<SerializableObjectInformation>& vObjects,
    bool bEnableBackup) {
    // Check that all objects are unique.
    for (size_t i = 0; i < vObjects.size(); i++) {
        for (size_t j = 0; j < vObjects.size(); j++) {
            if (i != j && vObjects[i].pObject == vObjects[j].pObject) [[unlikely]] {
                return Error("the specified array of objects has multiple instances of the same object");
            }
        }
    }

    // Check that IDs are unique and don't have dots in them.
    for (const auto& objectData : vObjects) {
        if (objectData.sObjectUniqueId.empty()) [[unlikely]] {
            return Error("specified an empty object ID");
        }

        if (objectData.sObjectUniqueId.find('.') != std::string::npos) [[unlikely]] {
            return Error(std::format(
                "the specified object ID \"{}\" is not allowed to have dots in it",
                objectData.sObjectUniqueId));
        }
        for (const auto& compareObject : vObjects) {
            if (objectData.pObject != compareObject.pObject &&
                objectData.sObjectUniqueId == compareObject.sObjectUniqueId) [[unlikely]] {
                return Error("object IDs are not unique");
            }
        }
    }

    if (!pathToFile.string().ends_with(".toml")) {
        pathToFile += ".toml";
    }

    if (!std::filesystem::exists(pathToFile.parent_path())) {
        std::filesystem::create_directories(pathToFile.parent_path());
    }

    // Handle backup.
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

#if defined(WIN32)
    // Check if the path length is too long.
    constexpr auto iMaxPathLimitBound = 15;
    constexpr auto iMaxPath = 260; // value of Windows' MAXPATH macro
    constexpr auto iMaxPathLimit = iMaxPath - iMaxPathLimitBound;
    const auto iFilePathLength = pathToFile.string().length();
    if (iFilePathLength > iMaxPathLimit - (iMaxPathLimitBound * 2) && iFilePathLength < iMaxPathLimit) {
        Logger::get().warn(std::format(
            "file path length {} is close to the platform limit of {} characters (path: {})",
            iFilePathLength,
            iMaxPathLimit,
            pathToFile.string()));
    } else if (iFilePathLength >= iMaxPathLimit) {
        return Error(std::format(
            "file path length {} exceeds the platform limit of {} characters (path: {})",
            iFilePathLength,
            iMaxPathLimit,
            pathToFile.string()));
    }
#endif

    // Serialize.
    toml::value tomlData;
    for (const auto& objectData : vObjects) {
        auto result = objectData.pObject->serialize(
            pathToFile,
            tomlData,
            objectData.pOriginalObject,
            objectData.sObjectUniqueId,
            objectData.customAttributes);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto err = std::get<Error>(std::move(result));
            err.addCurrentLocationToErrorStack();
            return err;
        }
    }

    // Save TOML data to file.
    std::ofstream file(pathToFile, std::ios::binary);
    if (!file.is_open()) [[unlikely]] {
        return Error(std::format(
            "failed to open the file \"{}\" (maybe because it's marked as read-only)", pathToFile.string()));
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

std::variant<std::string, Error> Serializable::serialize( // NOLINT: too complex
    const std::filesystem::path& pathToFile,
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

        constexpr float floatEpsilon = 0.00001F;

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

                currentValue.serialize(std::format(
                    "{}.{}.{}.{}",
                    basePathToBinaryFile.string(),
                    sEntityId,
                    sVariableName,
                    std::string(sBinaryFileExtension)));
            }
        }

#if defined(WIN32) && defined(DEBUG)
        static_assert(sizeof(TypeReflectionInfo) == 1088, "add new variables here"); // NOLINT: current size
#elif defined(DEBUG)
        static_assert(sizeof(TypeReflectionInfo) == 936, "add new variables here"); // NOLINT: current size
#endif
    }

    if (pOriginalObject != nullptr && pOriginalObject->getPathDeserializedFromRelativeToRes().has_value()) {
        // Write path to the original and original ID.
        toml::value tomlArray = toml::array();
        tomlArray.push_back((*pOriginalObject->getPathDeserializedFromRelativeToRes()).first);
        tomlArray.push_back((*pOriginalObject->getPathDeserializedFromRelativeToRes()).second);
        tomlData[sSectionName][sTomlKeyPathToOriginalRelativeToRes.data()] = tomlArray;
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
