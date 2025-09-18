#pragma once

// Standard.
#include <functional>
#include <string>
#include <memory>
#include <unordered_map>
#include <optional>
#include <filesystem>
#include <variant>

// Custom.
#include "misc/Error.h"
#include "io/ConfigManager.h"
#include "misc/ProjectPaths.h"
#include "io/Logger.h"
#include "misc/ReflectedTypeDatabase.h"

class Serializable;

/** Information about an object to be serialized. */
struct SerializableObjectInformation {
    SerializableObjectInformation() = delete;

    /**
     * Initialized object information for serialization.
     *
     * @param pObject          Object to serialize.
     * @param sObjectUniqueId  Object's unique ID. Don't use dots in IDs.
     * @param customAttributes Optional. Pairs of values to serialize with this object.
     * @param pOriginalObject  Optional. Use if the object was previously deserialized and
     * you now want to only serialize changed fields of this object and additionally store
     * the path to the original file (to deserialize unchanged fields).
     */
    SerializableObjectInformation(
        Serializable* pObject,
        const std::string& sObjectUniqueId,
        const std::unordered_map<std::string, std::string>& customAttributes = {},
        Serializable* pOriginalObject = nullptr) {
        this->pObject = pObject;
        this->sObjectUniqueId = sObjectUniqueId;
        this->customAttributes = customAttributes;
        this->pOriginalObject = pOriginalObject;
    }

    /** Object to serialize. */
    Serializable* pObject = nullptr;

    /**
     * Use if @ref pObject was previously deserialized and you now want to only serialize
     * changed fields of this object and additionally store the path to the original file
     * (to deserialize unchanged fields).
     */
    Serializable* pOriginalObject = nullptr;

    /** Unique object ID. Don't use dots in it. */
    std::string sObjectUniqueId;

    /** Map of object attributes (custom information) that will be also serialized/deserialized. */
    std::unordered_map<std::string, std::string> customAttributes;
};

/** Information about an object that was deserialized. */
template <typename SmartPointer, typename InnerType = typename SmartPointer::element_type>
    requires std::derived_from<InnerType, Serializable>
struct DeserializedObjectInformation {
public:
    DeserializedObjectInformation() = delete;

    /**
     * Initialized object information after deserialization.
     *
     * @param pObject          Deserialized object.
     * @param sObjectUniqueId  Object's unique ID.
     * @param customAttributes Object's custom attributes.
     */
    DeserializedObjectInformation(
        SmartPointer pObject,
        std::string sObjectUniqueId,
        std::unordered_map<std::string, std::string> customAttributes) {
        this->pObject = std::move(pObject);
        this->sObjectUniqueId = sObjectUniqueId;
        this->customAttributes = customAttributes;
    }

    /** Deserialized object. */
    SmartPointer pObject;

    /** Unique object ID. */
    std::string sObjectUniqueId;

    /** Map of object attributes (custom information) that were deserialized. */
    std::unordered_map<std::string, std::string> customAttributes;
};

/** Allows derived classes to be serialized and deserialized to/from file. */
class Serializable {
public:
    Serializable() = default;
    virtual ~Serializable() = default;

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const = 0;

    /**
     * Returns ending of the name for the directory that stores geometry of a node tree.
     * Full name of the directory consists of the node tree filename and this suffix.
     *
     * @return Directory name suffix.
     */
    static constexpr std::string_view getNodeTreeGeometryDirSuffix() { return sNodeTreeGeometryDirSuffix; }

    /**
     * Returns file extension (without the dot) that all serialized binary files have (for example mesh
     * geometry).
     *
     * @return File extension.
     */
    static constexpr std::string_view getBinaryFileExtension() { return sBinaryFileExtension; }

    /**
     * Deserializes object(s) from a file.
     *
     * @param pathToFile File to read reflected data from. The ".toml" extension will be added
     * automatically if not specified in the path.
     *
     * @return Error if something went wrong, otherwise first deserialized object.
     */
    template <typename T>
        requires std::derived_from<T, Serializable>
    static std::variant<std::unique_ptr<T>, Error> deserialize(const std::filesystem::path& pathToFile);

    /**
     * Deserializes object(s) from a file.
     *
     * @param pathToFile       File to read reflected data from. The ".toml" extension will be added
     * automatically if not specified in the path.
     * @param sObjectUniqueId  Unique ID (that was previously specified in @ref serializeMultiple) of an
     * object to deserialize (and ignore others).
     * @param customAttributes Pairs of values that were associated with this object.
     *
     * @return Error if something went wrong, otherwise first deserialized object.
     */
    template <typename T>
        requires std::derived_from<T, Serializable>
    static std::variant<std::unique_ptr<T>, Error> deserialize(
        std::filesystem::path pathToFile,
        const std::string& sObjectUniqueId,
        std::unordered_map<std::string, std::string>& customAttributes);

    /**
     * Deserializes object(s) from a file.
     *
     * @param pathToFile File to read reflected data from. The ".toml" extension will be added
     * automatically if not specified in the path.
     *
     * @return Error if something went wrong, otherwise an array of deserialized objects.
     */
    template <typename T>
        requires std::derived_from<T, Serializable>
    static std::variant<std::vector<DeserializedObjectInformation<std::unique_ptr<T>>>, Error>
    deserializeMultiple(std::filesystem::path pathToFile);

    /**
     * Serializes the object and all reflected fields (including inherited) into a file.
     * Serialized object can later be deserialized using @ref deserialize.
     *
     * @param pathToFile       File to write reflected data to. The ".toml" extension will be added
     * automatically if not specified in the path. If the specified file already exists it will be
     * overwritten. If the directories of the specified file do not exist they will be recursively
     * created.
     * @param bEnableBackup    If `true` will also create a backup (copy) file. @ref deserialize can use
     * backup file if the original file does not exist. Generally you want to use
     * a backup file if you are saving important information, such as player progress,
     * other cases such as player game settings and etc. usually do not need a backup but
     * you can use them if you want.
     * @param customAttributes Optional. Custom pairs of values that will be saved as this object's
     * additional information and could be later retrieved in @ref deserialize.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] std::optional<Error> serialize(
        std::filesystem::path pathToFile,
        bool bEnableBackup,
        const std::unordered_map<std::string, std::string>& customAttributes = {});

    /**
     * Serializes multiple objects, their reflected fields (including inherited) and provided
     * custom attributes (if any) into a file.
     *
     * @param pathToFile    File to write reflected data to. The ".toml" extension will be added
     * automatically if not specified in the path. If the specified file already exists it will be
     * overwritten.
     * @param vObjects      Array of objects to serialize, their unique IDs
     * (so they could be differentiated in the file) and custom attributes (if any). Don't use
     * dots in the entity IDs, dots are used internally.
     * @param bEnableBackup If `true` will also use a backup (copy) file. @ref deserializeMultiple can use
     * backup file if the original file does not exist. Generally you want to use
     * a backup file if you are saving important information, such as player progress,
     * other cases such as player game settings and etc. usually do not need a backup but
     * you can use it if you want.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] static std::optional<Error> serializeMultiple(
        std::filesystem::path pathToFile,
        const std::vector<SerializableObjectInformation>& vObjects,
        bool bEnableBackup);

    /**
     * If this object was deserialized from a file that is located in the `res` directory
     * of this project returns file path.
     *
     * This path will never point to a backup file and will always point to the original file
     * (even if the backup file was used in deserialization).
     *
     * Example: say this object is deserialized from the file located at `.../res/game/test.toml`,
     * this value will be equal to the following pair: {`game/test.toml`, `some.id`}.
     *
     * @return Empty if this object was not deserialized previously, otherwise a pair of values:
     * - path to this file relative to the `res` directory,
     * - unique ID of this object in this file.
     */
    std::optional<std::pair<std::string, std::string>> getPathDeserializedFromRelativeToRes() const;

protected:
    /** Called after this object was finished deserializing from file. */
    virtual void onAfterDeserialized() {}

private:
    /**
     * Deserializes object(s) from a toml value.
     *
     * @param tomlData         TOML data to read.
     * @param sObjectUniqueId  Unique ID (that was previously specified in @ref serializeMultiple) of an
     * object to deserialize (and ignore others).
     * @param customAttributes Pairs of values that were associated with this object.
     * @param pathToFile       Path to file that the TOML data was deserialized from
     * (used for variables that are stores as separate binary files).
     *
     * @return Error if something went wrong, otherwise first deserialized object.
     */
    template <typename T>
        requires std::derived_from<T, Serializable>
    static std::variant<std::unique_ptr<T>, Error> deserialize(
        const toml::value& tomlData,
        const std::string& sObjectUniqueId,
        std::unordered_map<std::string, std::string>& customAttributes,
        const std::filesystem::path& pathToFile);

    /**
     * Deserializes an object and all reflected fields (including inherited) from a TOML data.
     * Specify the type of an object (that is located in the file) as the T template parameter, which
     * can be entity's actual type or entity's parent (up to Serializable).
     *
     * @remark This is an overloaded function, see a more detailed documentation for the other
     * overload.
     *
     * @param tomlData         Toml value to retrieve an object from.
     * @param customAttributes Pairs of values that were associated with this object.
     * @param sSectionName     Name of the TOML section to deserialize.
     * @param sTypeGuid        GUID of the type to deserialize (taken from section name).
     * @param sEntityId        Entity ID chain string. This is a unique ID of the object to deserialize (and
     * ignore others).
     * @param pathToFile       Path to file that the TOML data was deserialized from
     * (used for variables that are stores as separate binary files).
     *
     * @return Error if something went wrong, otherwise a pointer to deserialized object.
     */
    template <typename T>
        requires std::derived_from<T, Serializable>
    static std::variant<std::unique_ptr<T>, Error> deserializeFromSection(
        const toml::value& tomlData,
        std::unordered_map<std::string, std::string>& customAttributes,
        const std::string& sSectionName,
        const std::string& sTypeGuid,
        const std::string& sEntityId,
        const std::filesystem::path& pathToFile);

    /**
     * Adds ".toml" extension to the path (if needed) and copies a backup file to the specified path
     * if the specified path does not exist but there is a backup file.
     *
     * @param pathToFile Path to toml file (may point to non-existing path or don't have ".toml"
     * extension).
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] static std::optional<Error> resolvePathToToml(std::filesystem::path& pathToFile);

    /**
     * Serializes the object and all reflected fields (including inherited) into the toml data.
     * Serialized object can later be deserialized using @ref deserialize.
     *
     * @param pathToFile         Path to the file that this TOML data will be serialized.
     * Used for fields that will be stored in a separate file (such as geometry).
     * @param tomlData           Toml value to append this object to.
     * @param pOriginalObject    Optional. Original object of the same type as the object being
     * serialized, this object is a deserialized version of the object being serialized,
     * used to compare serializable fields' values and only serialize changed values.
     * @param sEntityId          Unique ID of this object. When serializing multiple objects into
     * one toml value provide different IDs for each object so they could be differentiated. Don't use
     * dots in the entity ID, dots are used in recursion when this function is called from this
     * function to process reflected field (sub entity).
     * @param customAttributes   Optional. Custom pairs of values that will be saved as this object's
     * additional information and could be later retrieved in @ref deserialize.
     *
     * @return Error if something went wrong, otherwise name of the TOML section that was used to store this
     * entity.
     */
    [[nodiscard]] std::variant<std::string, Error> serialize(
        const std::filesystem::path& pathToFile,
        toml::value& tomlData,
        Serializable* pOriginalObject = nullptr,
        std::string sEntityId = "",
        const std::unordered_map<std::string, std::string>& customAttributes = {});

    /**
     * If this object was deserialized from a file that is located in the `res` directory
     * of this project stores 2 valid values:
     * - path to this file relative to the `res` directory,
     * - unique ID of this object in this file.
     *
     * This path will never point to a backup file and will always point to the original file
     * (even if the backup file was used in deserialization).
     *
     * Example: say this object is deserialized from the file located at `.../res/game/test.toml`,
     * this value will be equal to the following pair: {`game/test.toml`, `some.id`}.
     */
    std::optional<std::pair<std::string, std::string>> pathDeserializedFromRelativeToRes;

    /**
     * Name of the key which we use when we serialize an object that was previously
     * deserialized from the `res` directory.
     */
    static constexpr std::string_view sTomlKeyPathToOriginalRelativeToRes = ".path_to_original";

    /** Text that we add to custom (user-specified) attributes in TOML files. */
    static constexpr std::string_view sTomlKeyCustomAttributePrefix = "..";

    /** Extension that all serialized binary files have (for example mesh geometry). */
    static constexpr std::string_view sBinaryFileExtension = "bin";

    /**
     * Ending of the name for the directory that stores geometry of a node tree.
     * Full name of the directory consists of the node tree filename and this suffix.
     */
    static constexpr std::string_view sNodeTreeGeometryDirSuffix = "_geo";
};

template <typename T>
    requires std::derived_from<T, Serializable>
inline std::variant<std::unique_ptr<T>, Error> Serializable::deserialize(
    const toml::value& tomlData,
    const std::string& sObjectUniqueId,
    std::unordered_map<std::string, std::string>& customAttributes,
    const std::filesystem::path& pathToFile) {
    // Get TOML as table.
    const auto& fileTable = tomlData.as_table();
    if (fileTable.empty()) [[unlikely]] {
        return Error(std::format(
            "provided toml value has 0 sections while expected at least 1 section (file path {})",
            pathToFile.string()));
    }

    // Find a section that starts with the specified entity ID.
    // Each entity section has the following format: [entityId.GUID]
    // For sub entities (field with reflected type) format: [parentEntityId.childEntityId.childGUID]
    std::string sTargetSection;
    std::string sTypeGuid;
    for (const auto& [sSectionName, tomlValue] : fileTable) {
        // We can't just use `sSectionName.starts_with(sEntityId)` because we might make a mistake in the
        // following situation: [100...] with entity ID equal to "10" and even if we add a dot
        // to `sEntityId` we still might make a mistake in the following situation:
        // [10.30.GUID] while we look for just [10.GUID].

        // Get ID end position (GUID start position).
        const auto iIdEndDotPos = sSectionName.rfind('.');
        if (iIdEndDotPos == std::string::npos) [[unlikely]] {
            return Error(std::format("section name \"{}\" does not contain entity ID", sSectionName));
        }
        if (iIdEndDotPos + 1 == sSectionName.size()) [[unlikely]] {
            return Error(std::format("section name \"{}\" does not have a GUID", sSectionName));
        }
        if (iIdEndDotPos == 0) [[unlikely]] {
            return Error(std::format("section \"{}\" is not full", sSectionName));
        }

        // Get ID chain (either entity ID or something like "parentEntityId.childEntityId").
        if (std::string_view(sSectionName.data(), iIdEndDotPos) != sObjectUniqueId) { // compare without copy
            continue;
        }

        // Save target section name.
        sTargetSection = sSectionName;

        // Save this entity's GUID.
        sTypeGuid = sSectionName.substr(iIdEndDotPos + 1);

        break;
    }

    // Make sure something was found.
    if (sTargetSection.empty()) [[unlikely]] {
        return Error(std::format("could not find entity with ID \"{}\"", sObjectUniqueId));
    }

    return deserializeFromSection<T>(
        tomlData, customAttributes, sTargetSection, sTypeGuid, sObjectUniqueId, pathToFile);
}

template <typename T>
    requires std::derived_from<T, Serializable>
inline std::variant<std::unique_ptr<T>, Error> Serializable::deserialize(
    std::filesystem::path pathToFile,
    const std::string& sObjectUniqueId,
    std::unordered_map<std::string, std::string>& customAttributes) {
    // Resolve path.
    auto optionalError = resolvePathToToml(pathToFile);
    if (optionalError.has_value()) [[unlikely]] {
        auto error = std::move(optionalError.value());
        error.addCurrentLocationToErrorStack();
        return error;
    }

    // Parse file.
    toml::value tomlData;
    try {
        tomlData = toml::parse(pathToFile);
    } catch (std::exception& exception) {
        return Error(std::format(
            "failed to parse TOML file at \"{}\", error: {}", pathToFile.string(), exception.what()));
    }

    return deserialize<T>(tomlData, sObjectUniqueId, customAttributes, pathToFile);
}

template <typename T>
    requires std::derived_from<T, Serializable>
inline std::variant<std::unique_ptr<T>, Error>
Serializable::deserialize(const std::filesystem::path& pathToFile) {
    auto result = deserializeMultiple<T>(pathToFile);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        return std::get<Error>(std::move(result));
    }
    auto vDeserializedObjects =
        std::get<std::vector<DeserializedObjectInformation<std::unique_ptr<T>>>>(std::move(result));

    if (vDeserializedObjects.empty()) [[unlikely]] {
        return Error(std::format("nothing was deserialized from file \"{}\"", pathToFile.string()));
    }
    if (vDeserializedObjects.size() > 1) [[unlikely]] {
        return Error(std::format(
            "deserialized {} objects while expected only 1, this function assumes that there's only 1 "
            "object to deserialize, otherwise use another `deserialize` function and specify an object "
            "ID to deserialize (file \"{}\")",
            vDeserializedObjects.size(),
            pathToFile.string()));
    }

    return std::move(vDeserializedObjects[0].pObject);
}

template <typename T>
    requires std::derived_from<T, Serializable>
inline std::variant<std::vector<DeserializedObjectInformation<std::unique_ptr<T>>>, Error>
Serializable::deserializeMultiple(std::filesystem::path pathToFile) {
    // Resolve path.
    auto optionalError = resolvePathToToml(pathToFile);
    if (optionalError.has_value()) [[unlikely]] {
        auto error = std::move(optionalError.value());
        error.addCurrentLocationToErrorStack();
        return error;
    }

    // Parse file.
    toml::value tomlData;
    try {
        tomlData = toml::parse(pathToFile);
    } catch (std::exception& exception) {
        return Error(std::format(
            "failed to parse TOML file at \"{}\", error: {}", pathToFile.string(), exception.what()));
    }

    // Get TOML as table.
    const auto fileTable = tomlData.as_table();
    if (fileTable.empty()) [[unlikely]] {
        return Error(std::format(
            "provided toml value has 0 sections while expected at least 1 section (file path {})",
            pathToFile.string()));
    }

    // Deserialize.
    std::vector<DeserializedObjectInformation<std::unique_ptr<T>>> vDeserializedObjects;
    for (const auto& [sSectionName, tomlValue] : fileTable) {
        // Get type GUID.
        const auto iIdEndDotPos = sSectionName.rfind('.');
        if (iIdEndDotPos == std::string::npos) [[unlikely]] {
            return Error("provided toml value does not contain entity ID");
        }
        const auto sTypeGuid = sSectionName.substr(iIdEndDotPos + 1);

        // Get entity ID chain.
        const auto sEntityId = sSectionName.substr(0, iIdEndDotPos);

        // Check if this is a sub-entity.
        if (sEntityId.find('.') != std::string::npos) {
            // Only deserialize top-level entities because sub-entities (fields)
            // will be deserialized while we deserialize top-level entities.
            continue;
        }

        // Deserialize object from this section.
        std::unordered_map<std::string, std::string> customAttributes;
        auto result = deserializeFromSection<T>(
            tomlData, customAttributes, sSectionName, sTypeGuid, sEntityId, pathToFile);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            return error;
        }

        // Save object info.
        DeserializedObjectInformation objectInfo(
            std::get<std::unique_ptr<T>>(std::move(result)), sEntityId, customAttributes);
        vDeserializedObjects.push_back(std::move(objectInfo));
    }

    return vDeserializedObjects;
}

template <typename T>
    requires std::derived_from<T, Serializable>
inline std::variant<std::unique_ptr<T>, Error> Serializable::deserializeFromSection(
    const toml::value& tomlData,
    std::unordered_map<std::string, std::string>& customAttributes,
    const std::string& sSectionName,
    const std::string& sTypeGuid,
    const std::string& sEntityId,
    const std::filesystem::path& pathToFile) {
    // Get all keys (field names) from this section.
    const auto& targetSection = tomlData.at(sSectionName);
    if (!targetSection.is_table()) [[unlikely]] {
        return Error(std::format("found \"{}\" section is not a section", sSectionName));
    }

    // Collect keys from target section.
    const auto& sectionTable = targetSection.as_table();
    std::unordered_map<std::string_view, const toml::value*> fieldsToDeserialize;
    std::optional<std::pair<std::string, std::string>> originalObjectPathAndId;
    for (const auto& [sKey, value] : sectionTable) {
        if (sKey == sTomlKeyPathToOriginalRelativeToRes) {
            if (!value.is_array()) [[unlikely]] {
                return Error(
                    std::format("found key \"{}\" has wrong type", sTomlKeyPathToOriginalRelativeToRes));
            }
            auto tomlArray = value.as_array();

            if (tomlArray.size() != 2) [[unlikely]] {
                return Error(std::format(
                    "found array key \"{}\" with unexpected size", sTomlKeyPathToOriginalRelativeToRes));
            }
            if (!tomlArray[0].is_string() || !tomlArray[1].is_string()) [[unlikely]] {
                return Error(std::format(
                    "found array key \"{}\" has unexpected element type",
                    sTomlKeyPathToOriginalRelativeToRes));
            }

            originalObjectPathAndId = {tomlArray[0].as_string(), tomlArray[1].as_string()};
        } else if (sKey.starts_with(sTomlKeyCustomAttributePrefix)) {
            // Custom attribute.
            // Make sure it's a string.
            if (!value.is_string()) [[unlikely]] {
                return Error(std::format("found custom attribute \"{}\" is not a string", sKey));
            }

            // Add attribute.
            customAttributes[sKey.substr(sTomlKeyCustomAttributePrefix.size())] = value.as_string();
        } else {
            fieldsToDeserialize[sKey] = &value;
        }
    }

    // Prepare to create a new object to fill with deserialized info.
    const auto& typeInfo = ReflectedTypeDatabase::getTypeInfo(sTypeGuid);
    std::unique_ptr<T> pDeserializedObject = nullptr;
    bool bUsedOriginalObject = false;

    if (originalObjectPathAndId.has_value()) {
        const auto& [sPathRelativeResToOriginal, sOriginalObjectUniqueId] = *originalObjectPathAndId;

        // Use the original entity instead of creating a new one.
        auto deserializeResult = Serializable::deserialize<T>(
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathRelativeResToOriginal,
            sOriginalObjectUniqueId,
            customAttributes);
        if (std::holds_alternative<Error>(deserializeResult)) [[unlikely]] {
            auto error = std::get<Error>(std::move(deserializeResult));
            error.addCurrentLocationToErrorStack();
            return error;
        }
        pDeserializedObject = std::get<std::unique_ptr<T>>(std::move(deserializeResult));

        bUsedOriginalObject = true;
    } else {
        auto pTemp = typeInfo.createNewObject();
        if (dynamic_cast<T*>(pTemp.get()) == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("createNewObject returned unexpected type for \"{}\"", typeInfo.sTypeName));
        }
        pDeserializedObject = std::unique_ptr<T>(reinterpret_cast<T*>(pTemp.release()));
    }

    // Deserialize fields.
    for (auto& [sFieldName, pFieldTomlValue] : fieldsToDeserialize) {
        // Find type.
        const auto varIt = typeInfo.variableNameToType.find(sFieldName.data());
        if (varIt == typeInfo.variableNameToType.end()) [[unlikely]] {
            Logger::get().warn(std::format(
                "field name \"{}\" exists in the specified toml value but does not exist in the actual "
                "object (if you removed/renamed this reflected field from your type - ignore this "
                "warning)",
                sFieldName));
            continue;
        }
        const auto variableType = varIt->second;

        // Deserialize value.
        switch (variableType) {
        case (ReflectedVariableType::BOOL): {
            if (!pFieldTomlValue->is_boolean()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            // Set value.
            const auto variableInfoIt = typeInfo.reflectedVariables.bools.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.bools.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }
            variableInfoIt->second.setter(pDeserializedObject.get(), pFieldTomlValue->as_boolean());

            break;
        }
        case (ReflectedVariableType::INT): {
            if (!pFieldTomlValue->is_integer()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            // Set value.
            const auto variableInfoIt = typeInfo.reflectedVariables.ints.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.ints.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }
            variableInfoIt->second.setter(
                pDeserializedObject.get(), static_cast<int>(pFieldTomlValue->as_integer()));

            break;
        }
        case (ReflectedVariableType::UNSIGNED_INT): {
            if (!pFieldTomlValue->is_integer()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const long long& iTomlValue = pFieldTomlValue->as_integer();
            unsigned int iValue = 0;
            if (iTomlValue > 0 && iTomlValue <= std::numeric_limits<unsigned int>::max()) {
                iValue = static_cast<unsigned int>(iTomlValue);
            }

            // Set value.
            const auto variableInfoIt = typeInfo.reflectedVariables.unsignedInts.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.unsignedInts.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }
            variableInfoIt->second.setter(pDeserializedObject.get(), iValue);

            break;
        }
        case (ReflectedVariableType::LONG_LONG): {
            if (!pFieldTomlValue->is_integer()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            // Set value.
            const auto variableInfoIt = typeInfo.reflectedVariables.longLongs.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.longLongs.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }
            variableInfoIt->second.setter(pDeserializedObject.get(), pFieldTomlValue->as_integer());

            break;
        }
        case (ReflectedVariableType::UNSIGNED_LONG_LONG): {
            // Stored as a string because toml11 uses `long long` for integers.
            if (!pFieldTomlValue->is_string()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const auto variableInfoIt = typeInfo.reflectedVariables.unsignedLongLongs.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.unsignedLongLongs.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            try {
                unsigned long long iValue = std::stoull(pFieldTomlValue->as_string());
                variableInfoIt->second.setter(pDeserializedObject.get(), iValue);
            } catch (std::exception& exception) {
                return Error(std::format(
                    "failed to convert string to unsigned long long for field \"{}\" on type \"{}\", "
                    "error: {}",
                    sFieldName,
                    typeInfo.sTypeName,
                    exception.what()));
            }

            break;
        }
        case (ReflectedVariableType::FLOAT): {
            if (!pFieldTomlValue->is_floating()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const auto variableInfoIt = typeInfo.reflectedVariables.floats.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.floats.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            variableInfoIt->second.setter(
                pDeserializedObject.get(), static_cast<float>(pFieldTomlValue->as_floating()));

            break;
        }
        case (ReflectedVariableType::STRING): {
            if (!pFieldTomlValue->is_string()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const auto variableInfoIt = typeInfo.reflectedVariables.strings.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.strings.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }
            variableInfoIt->second.setter(pDeserializedObject.get(), pFieldTomlValue->as_string());

            break;
        }
        case (ReflectedVariableType::SERIALIZABLE): {
            std::unordered_map<std::string, std::string> tempCustomAttributes;
            // TODO: specifying empty path to file for now because it might not work as intended
            auto result = deserialize<Serializable>(*pFieldTomlValue, "0", tempCustomAttributes, "");
            if (std::holds_alternative<Error>(result)) [[unlikely]] {
                auto error = std::get<Error>(std::move(result));
                error.addCurrentLocationToErrorStack();
                return error;
            }
            auto pDeserializedValue = std::get<std::unique_ptr<Serializable>>(std::move(result));

            const auto variableInfoIt = typeInfo.reflectedVariables.serializables.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.serializables.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }
            variableInfoIt->second.setter(pDeserializedObject.get(), std::move(pDeserializedValue));

            break;
        }

        case (ReflectedVariableType::VEC2): {
            if (!pFieldTomlValue->is_array()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const auto variableInfoIt = typeInfo.reflectedVariables.vec2s.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.vec2s.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            auto tomlArray = pFieldTomlValue->as_array();
            if (tomlArray.size() != 2) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "unexpected size of the array on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }
            glm::vec2 result = glm::vec2(0.0F, 0.0F);

            if (!tomlArray[0].is_floating() || !tomlArray[1].is_floating()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "unexpected element type of the array on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            result.x = static_cast<float>(tomlArray[0].as_floating());
            result.y = static_cast<float>(tomlArray[1].as_floating());

            variableInfoIt->second.setter(pDeserializedObject.get(), result);

            break;
        }
        case (ReflectedVariableType::VEC3): {
            if (!pFieldTomlValue->is_array()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const auto variableInfoIt = typeInfo.reflectedVariables.vec3s.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.vec3s.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            auto tomlArray = pFieldTomlValue->as_array();
            if (tomlArray.size() != 3) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "unexpected size of the array on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }
            glm::vec3 result = glm::vec3(0.0F, 0.0F, 0.0F);

            if (!tomlArray[0].is_floating() || !tomlArray[1].is_floating() || !tomlArray[2].is_floating())
                [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "unexpected element type of the array on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            result.x = static_cast<float>(tomlArray[0].as_floating());
            result.y = static_cast<float>(tomlArray[1].as_floating());
            result.z = static_cast<float>(tomlArray[2].as_floating());

            variableInfoIt->second.setter(pDeserializedObject.get(), result);

            break;
        }
        case (ReflectedVariableType::VEC4): {
            if (!pFieldTomlValue->is_array()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const auto variableInfoIt = typeInfo.reflectedVariables.vec4s.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.vec4s.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            auto tomlArray = pFieldTomlValue->as_array();
            if (tomlArray.size() != 4) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "unexpected size of the array on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }
            glm::vec4 result = glm::vec4(0.0F, 0.0F, 0.0F, 0.0F);

            if (!tomlArray[0].is_floating() || !tomlArray[1].is_floating() || !tomlArray[2].is_floating() ||
                !tomlArray[3].is_floating()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "unexpected element type of the array on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            result.x = static_cast<float>(tomlArray[0].as_floating());
            result.y = static_cast<float>(tomlArray[1].as_floating());
            result.z = static_cast<float>(tomlArray[2].as_floating());
            result.w = static_cast<float>(tomlArray[3].as_floating());

            variableInfoIt->second.setter(pDeserializedObject.get(), result);

            break;
        }
        case (ReflectedVariableType::VECTOR_INT): {
            if (!pFieldTomlValue->is_array()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const auto variableInfoIt = typeInfo.reflectedVariables.vectorInts.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.vectorInts.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            auto& tomlArray = pFieldTomlValue->as_array();
            std::vector<int> vArray(tomlArray.size(), 0);
            for (size_t i = 0; i < tomlArray.size(); i++) {
                if (!tomlArray[i].is_integer()) [[unlikely]] {
                    Error::showErrorAndThrowException(std::format(
                        "found unexpected element type in TOML data on variable \"{}\" from \"{}\"",
                        sFieldName,
                        typeInfo.sTypeName));
                }
                vArray[i] = static_cast<int>(tomlArray[i].as_integer());
            }

            variableInfoIt->second.setter(pDeserializedObject.get(), vArray);

            break;
        }
        case (ReflectedVariableType::VECTOR_STRING): {
            if (!pFieldTomlValue->is_array()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const auto variableInfoIt = typeInfo.reflectedVariables.vectorStrings.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.vectorStrings.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            auto& tomlArray = pFieldTomlValue->as_array();
            std::vector<std::string> vArray(tomlArray.size());
            for (size_t i = 0; i < tomlArray.size(); i++) {
                if (!tomlArray[i].is_string()) [[unlikely]] {
                    Error::showErrorAndThrowException(std::format(
                        "found unexpected element type in TOML data on variable \"{}\" from \"{}\"",
                        sFieldName,
                        typeInfo.sTypeName));
                }
                vArray[i] = tomlArray[i].as_string();
            }

            variableInfoIt->second.setter(pDeserializedObject.get(), vArray);

            break;
        }
        case (ReflectedVariableType::VECTOR_VEC3): {
            if (!pFieldTomlValue->is_array()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "variable \"{}\" from \"{}\" has unexpected type in the TOML data",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            const auto variableInfoIt = typeInfo.reflectedVariables.vectorVec3s.find(sFieldName.data());
            if (variableInfoIt == typeInfo.reflectedVariables.vectorVec3s.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "found mismatch between internal structured on variable \"{}\" from \"{}\"",
                    sFieldName,
                    typeInfo.sTypeName));
            }

            auto tomlArray = pFieldTomlValue->as_array();
            if (tomlArray.size() % 3 != 0) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "unexpected array size on variable \"{}\" from \"{}\"", sFieldName, typeInfo.sTypeName));
            }

            const auto iFinalArraySize = tomlArray.size() / 3;
            std::vector<glm::vec3> vResultingArray;
            vResultingArray.resize(iFinalArraySize);
            for (size_t i = 0, iTomlArrayIndex = 0; i < iFinalArraySize; i += 1, iTomlArrayIndex += 3) {
                if (!tomlArray[iTomlArrayIndex].is_floating() ||
                    !tomlArray[iTomlArrayIndex + 1].is_floating() ||
                    !tomlArray[iTomlArrayIndex + 2].is_floating()) [[unlikely]] {
                    Error::showErrorAndThrowException(std::format(
                        "unexpected element type in array on variable \"{}\" from \"{}\"",
                        sFieldName,
                        typeInfo.sTypeName));
                }
                vResultingArray[i].x = static_cast<float>(tomlArray[iTomlArrayIndex].as_floating());
                vResultingArray[i].y = static_cast<float>(tomlArray[iTomlArrayIndex + 1].as_floating());
                vResultingArray[i].z = static_cast<float>(tomlArray[iTomlArrayIndex + 2].as_floating());
            }

            variableInfoIt->second.setter(pDeserializedObject.get(), vResultingArray);

            break;
        }
        case (ReflectedVariableType::MESH_GEOMETRY): {
            Error::showErrorAndThrowException("mesh geometry is not expected to be stored in the TOML file");
        }
        default: {
            Error::showErrorAndThrowException("unhandled case");
        }
        }
#if defined(WIN32) && defined(DEBUG)
        static_assert(sizeof(TypeReflectionInfo) == 1216, "add new variables here");
#elif defined(DEBUG)
        static_assert(sizeof(TypeReflectionInfo) == 1048, "add new variables here");
#endif
    }

    // Deserialize geometry.
    if (std::filesystem::exists(pathToFile)) {
        if (!pathToFile.has_parent_path()) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("expected the path to have a parent path \"{}\"", pathToFile.string()));
        }

        // Construct path to the directory that stores geometry files.
        const std::string sFilename = pathToFile.stem().string();
        const auto pathToGeoDir =
            pathToFile.parent_path() / (sFilename + std::string(sNodeTreeGeometryDirSuffix));

        if (std::filesystem::exists(pathToGeoDir)) {
            // Prepare a handle lambda.
            const auto getPathToGeometryFile = [&](const std::string& sVariableName) {
                return pathToGeoDir /
                       (sEntityId + "." + sVariableName + "." + std::string(sBinaryFileExtension));
            };

            size_t iNotFoundMeshGeometryCount = 0;

            // Mesh geometry.
            if (!typeInfo.reflectedVariables.meshNodeGeometries.empty()) {
                for (const auto& [sVariableName, variableInfo] :
                     typeInfo.reflectedVariables.meshNodeGeometries) {
                    const auto pathToMeshGeometry = getPathToGeometryFile(sVariableName);
                    if (!std::filesystem::exists(pathToMeshGeometry)) {
                        if (!bUsedOriginalObject) {
                            // There may be node geometry file in case this is a SkeletalMeshNode which has
                            // skeletal mesh node geometry but empty mesh node geometry.
                            iNotFoundMeshGeometryCount += 1; // make sure there will be a skeletal geometry
                        }
                        continue;
                    }

                    auto meshGeometry = MeshNodeGeometry::deserialize(pathToMeshGeometry);
                    variableInfo.setter(pDeserializedObject.get(), meshGeometry);
                }
            }

            // Skeletal mesh geometry.
            if (!typeInfo.reflectedVariables.skeletalMeshNodeGeometries.empty()) {
                for (const auto& [sVariableName, variableInfo] :
                     typeInfo.reflectedVariables.skeletalMeshNodeGeometries) {
                    const auto pathToMeshGeometry = getPathToGeometryFile(sVariableName);
                    if (!std::filesystem::exists(pathToMeshGeometry)) {
                        if (!bUsedOriginalObject) {
                            Logger::get().warn(std::format(
                                "unable to find geometry file for variable \"{}\" for file \"{}\" (expected "
                                "file \"{}\")",
                                sVariableName,
                                pathToFile.filename().string(),
                                pathToMeshGeometry.filename().string()));
                        }
                        continue;
                    }

                    if (iNotFoundMeshGeometryCount > 0) {
                        iNotFoundMeshGeometryCount -= 1;
                    }

                    auto meshGeometry = SkeletalMeshNodeGeometry::deserialize(pathToMeshGeometry);
                    variableInfo.setter(pDeserializedObject.get(), meshGeometry);
                }
            }

            if (iNotFoundMeshGeometryCount > 0) {
                Logger::get().warn(std::format(
                    "unable to find geometry file(s) for {} variable(s) for file \"{}\", make sure these "
                    "files "
                    "exist and have correct names",
                    iNotFoundMeshGeometryCount,
                    pathToFile.filename().string()));
            }
        }
    }

    // In case if we used an original object we should have "path deserialized from" already initialized
    // with the path to the original object and it should stay like so, in this case we should reference the
    // original object path (not the path we are deserialized from) so that if we have multiple modified
    // copies of an object they all will point to the same original file instead of creating a weird reference
    // scheme. Plus node trees (parent node trees) that use external node tree(s) need this when they (parent
    // node trees) are being overwritten once again.

    if (!bUsedOriginalObject && pathToFile.string().starts_with(
                                    ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT).string())) {
        // File is located in the `res` directory, save a relative path to the `res` directory.
        auto sRelativePath =
            std::filesystem::relative(
                pathToFile.string(), ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT))
                .string();

        // Replace all '\' characters with '/' just to be consistent.
        std::replace(sRelativePath.begin(), sRelativePath.end(), '\\', '/');

        // Remove the forward slash at the beginning (if exists).
        if (sRelativePath.starts_with('/')) {
            sRelativePath = sRelativePath.substr(1);
        }

        // Double check that everything is correct.
        const auto pathToOriginalFile =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sRelativePath;
        if (!std::filesystem::exists(pathToOriginalFile)) [[unlikely]] {
            return Error(std::format(
                "failed to save the relative path to the `res` directory for the file at \"{}\", "
                "reason: constructed path \"{}\" does not exist",
                pathToFile.string(),
                pathToOriginalFile.string()));
        }

        // Save deserialization path.
        pDeserializedObject->pathDeserializedFromRelativeToRes = {sRelativePath, sEntityId};
    }

    // Notify about deserialization finished.
    pDeserializedObject->onAfterDeserialized();

    return pDeserializedObject;
}
