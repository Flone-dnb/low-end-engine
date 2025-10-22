#include "misc/ReflectedTypeDatabase.h"

// Standard.
#include <format>

// Custom.
#include "misc/Error.h"
#include "game/node/Node.h"
#include "game/node/Sound2dNode.h"
#include "game/node/Sound3dNode.h"
#include "game/node/SpatialNode.h"
#include "game/node/MeshNode.h"
#include "game/node/CameraNode.h"
#include "game/node/SkeletonNode.h"
#include "game/node/SkeletonBoneAttachmentNode.h"
#include "game/node/SkeletalMeshNode.h"
#include "game/node/light/DirectionalLightNode.h"
#include "game/node/light/PointLightNode.h"
#include "game/node/light/SpotlightNode.h"
#include "game/node/ui/UiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "game/node/ui/SliderUiNode.h"
#include "game/node/ui/CheckboxUiNode.h"
#include "game/node/ui/ProgressBarUiNode.h"
#include "game/geometry/shapes/CollisionShape.h"
#include "game/node/physics/CollisionNode.h"
#include "game/node/physics/CompoundCollisionNode.h"
#include "game/node/physics/SimulatedBodyNode.h"
#include "game/node/physics/MovingBodyNode.h"
#include "game/node/physics/CharacterBodyNode.h"
#include "game/node/physics/SimpleCharacterBodyNode.h"
#include "game/node/physics/TriggerVolumeNode.h"

std::unordered_map<std::string, TypeReflectionInfo> ReflectedTypeDatabase::reflectedTypes{};

void ReflectedTypeDatabase::registerEngineTypes() {
    // General.
    registerType(Node::getTypeGuidStatic(), Node::getReflectionInfo());
    registerType(SpatialNode::getTypeGuidStatic(), SpatialNode::getReflectionInfo());
    registerType(CameraNode::getTypeGuidStatic(), CameraNode::getReflectionInfo());
    registerType(MeshNode::getTypeGuidStatic(), MeshNode::getReflectionInfo());

    // Sound.
    registerType(Sound2dNode::getTypeGuidStatic(), Sound2dNode::getReflectionInfo());
    registerType(Sound3dNode::getTypeGuidStatic(), Sound3dNode::getReflectionInfo());

    // Skeleton.
    registerType(SkeletalMeshNode::getTypeGuidStatic(), SkeletalMeshNode::getReflectionInfo());
    registerType(SkeletonNode::getTypeGuidStatic(), SkeletonNode::getReflectionInfo());
    registerType(
        SkeletonBoneAttachmentNode::getTypeGuidStatic(), SkeletonBoneAttachmentNode::getReflectionInfo());

    // Light.
    registerType(DirectionalLightNode::getTypeGuidStatic(), DirectionalLightNode::getReflectionInfo());
    registerType(PointLightNode::getTypeGuidStatic(), PointLightNode::getReflectionInfo());
    registerType(SpotlightNode::getTypeGuidStatic(), SpotlightNode::getReflectionInfo());

    // UI.
    registerType(UiNode::getTypeGuidStatic(), UiNode::getReflectionInfo());
    registerType(TextUiNode::getTypeGuidStatic(), TextUiNode::getReflectionInfo());
    registerType(RectUiNode::getTypeGuidStatic(), RectUiNode::getReflectionInfo());
    registerType(LayoutUiNode::getTypeGuidStatic(), LayoutUiNode::getReflectionInfo());
    registerType(ButtonUiNode::getTypeGuidStatic(), ButtonUiNode::getReflectionInfo());
    registerType(TextEditUiNode::getTypeGuidStatic(), TextEditUiNode::getReflectionInfo());
    registerType(SliderUiNode::getTypeGuidStatic(), SliderUiNode::getReflectionInfo());
    registerType(CheckboxUiNode::getTypeGuidStatic(), CheckboxUiNode::getReflectionInfo());
    registerType(ProgressBarUiNode::getTypeGuidStatic(), ProgressBarUiNode::getReflectionInfo());

    // Physics.
    registerType(CollisionShape::getTypeGuidStatic(), CollisionShape::getReflectionInfo());
    registerType(BoxCollisionShape::getTypeGuidStatic(), BoxCollisionShape::getReflectionInfo());
    registerType(SphereCollisionShape::getTypeGuidStatic(), SphereCollisionShape::getReflectionInfo());
    registerType(CapsuleCollisionShape::getTypeGuidStatic(), CapsuleCollisionShape::getReflectionInfo());
    registerType(CylinderCollisionShape::getTypeGuidStatic(), CylinderCollisionShape::getReflectionInfo());
    registerType(ConvexCollisionShape::getTypeGuidStatic(), ConvexCollisionShape::getReflectionInfo());
    registerType(CollisionNode::getTypeGuidStatic(), CollisionNode::getReflectionInfo());
    registerType(CompoundCollisionNode::getTypeGuidStatic(), CompoundCollisionNode::getReflectionInfo());
    registerType(SimulatedBodyNode::getTypeGuidStatic(), SimulatedBodyNode::getReflectionInfo());
    registerType(MovingBodyNode::getTypeGuidStatic(), MovingBodyNode::getReflectionInfo());
    registerType(CharacterBodyNode::getTypeGuidStatic(), CharacterBodyNode::getReflectionInfo());
    registerType(SimpleCharacterBodyNode::getTypeGuidStatic(), SimpleCharacterBodyNode::getReflectionInfo());
    registerType(TriggerVolumeNode::getTypeGuidStatic(), TriggerVolumeNode::getReflectionInfo());
}

void ReflectedTypeDatabase::registerType(const std::string& sTypeGuid, TypeReflectionInfo&& typeInfo) {
    // Make sure the GUID is not used.
    const auto typeIt = reflectedTypes.find(sTypeGuid);
    if (typeIt != reflectedTypes.end() && typeIt->second.sTypeName != typeInfo.sTypeName) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "GUID of the type \"{}\" is already used by a type named \"{}\", pick some GUID for the type",
            typeInfo.sTypeName,
            typeIt->second.sTypeName));
    }

    // Make sure the GUID does not have dots in it (just in case because our serialization does not expect
    // this).
    if (sTypeGuid.find('.') != std::string::npos) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("GUID of the type \"{}\" is invalid, dots are not allowed", typeInfo.sTypeName));
    }

    // Register type.
    reflectedTypes.emplace(sTypeGuid, std::move(typeInfo));
}

const TypeReflectionInfo& ReflectedTypeDatabase::getTypeInfo(const std::string& sTypeGuid) {
    const auto typeIt = reflectedTypes.find(sTypeGuid);
    if (typeIt == reflectedTypes.end()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "unable to find a type with GUID \"{}\" in the reflected type database (is it not registered "
            "yet?)",
            sTypeGuid));
    }

    return typeIt->second;
}

std::unordered_set<std::string> ReflectedVariables::collectVariableNames() const {
    std::unordered_set<std::string> names;

#define ADD_VARIABLES_OF_TYPE(type)                                                                          \
    for (const auto& [sVariableName, value] : type) {                                                        \
        if (!names.insert(sVariableName).second) [[unlikely]] {                                              \
            Error::showErrorAndThrowException(std::format(                                                   \
                "found 2 reflected variables with the same name \"{}\" - reflected variable names must "     \
                "be unique",                                                                                 \
                sVariableName));                                                                             \
        }                                                                                                    \
    }

    ADD_VARIABLES_OF_TYPE(bools);
    ADD_VARIABLES_OF_TYPE(ints);
    ADD_VARIABLES_OF_TYPE(unsignedInts);
    ADD_VARIABLES_OF_TYPE(longLongs);
    ADD_VARIABLES_OF_TYPE(unsignedLongLongs);
    ADD_VARIABLES_OF_TYPE(floats);
    ADD_VARIABLES_OF_TYPE(strings);
    ADD_VARIABLES_OF_TYPE(serializables);
    ADD_VARIABLES_OF_TYPE(vec2s);
    ADD_VARIABLES_OF_TYPE(vec3s);
    ADD_VARIABLES_OF_TYPE(vec4s);
    ADD_VARIABLES_OF_TYPE(vectorInts);
    ADD_VARIABLES_OF_TYPE(vectorStrings);
    ADD_VARIABLES_OF_TYPE(vectorVec3s);
    ADD_VARIABLES_OF_TYPE(meshNodeGeometries);
    ADD_VARIABLES_OF_TYPE(skeletalMeshNodeGeometries);
#if defined(WIN32) && defined(DEBUG)
    static_assert(sizeof(TypeReflectionInfo) == 1216, "add new variables here");
#elif defined(DEBUG)
    static_assert(sizeof(TypeReflectionInfo) == 1048, "add new variables here");
#endif

    return names;
}

TypeReflectionInfo::TypeReflectionInfo(
    const std::string& sParentTypeGuid,
    const std::string& sTypeName,
    const std::function<std::unique_ptr<Serializable>()>& createNewObject,
    ReflectedVariables&& reflectedVariables)
    : sParentTypeGuid(sParentTypeGuid), sTypeName(sTypeName), createNewObject(createNewObject) {
    if (!sParentTypeGuid.empty()) {
        // This parent info already includes inherited variables (if the parent also has Serializable
        // parents).
        const auto& parentTypeInfo = ReflectedTypeDatabase::getTypeInfo(sParentTypeGuid);
        const auto inheritedVariableNames = parentTypeInfo.reflectedVariables.collectVariableNames();

        // Make sure there won't be variables with the same name.
        const auto myVariableNames = reflectedVariables.collectVariableNames();
        for (const auto& sVariableName : myVariableNames) {
            const auto nameIt = inheritedVariableNames.find(sVariableName);
            if (nameIt != inheritedVariableNames.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "reflected variable \"{}\" of type \"{}\" has a non-unique name, the name \"{}\" is "
                    "already used on a reflected variable in one of the parents of \"{}\"",
                    sVariableName,
                    sTypeName,
                    sVariableName,
                    sTypeName));
            }
        }

        // Add parent variables.
#define ADD_PARENT_VARIABLES(type)                                                                           \
    for (const auto& [sVariableName, value] : parentTypeInfo.reflectedVariables.type) {                      \
        if (!reflectedVariables.type.insert({sVariableName, value}).second) [[unlikely]] {                   \
            Error::showErrorAndThrowException(std::format(                                                   \
                "type \"{}\" variable \"{}\": variable name is already used by some parent type",            \
                sTypeName,                                                                                   \
                sVariableName));                                                                             \
        }                                                                                                    \
    }

        ADD_PARENT_VARIABLES(bools);
        ADD_PARENT_VARIABLES(ints);
        ADD_PARENT_VARIABLES(unsignedInts);
        ADD_PARENT_VARIABLES(longLongs);
        ADD_PARENT_VARIABLES(unsignedLongLongs);
        ADD_PARENT_VARIABLES(floats);
        ADD_PARENT_VARIABLES(strings);
        ADD_PARENT_VARIABLES(serializables);
        ADD_PARENT_VARIABLES(vec2s);
        ADD_PARENT_VARIABLES(vec3s);
        ADD_PARENT_VARIABLES(vec4s);
        ADD_PARENT_VARIABLES(vectorInts);
        ADD_PARENT_VARIABLES(vectorStrings);
        ADD_PARENT_VARIABLES(vectorVec3s);
        ADD_PARENT_VARIABLES(meshNodeGeometries);
        ADD_PARENT_VARIABLES(skeletalMeshNodeGeometries);
#if defined(WIN32) && defined(DEBUG)
        static_assert(sizeof(TypeReflectionInfo) == 1216, "add new variables here");
#elif defined(DEBUG)
        static_assert(sizeof(TypeReflectionInfo) == 1048, "add new variables here");
#endif
    }

    // Save reflected variables.
    this->reflectedVariables = std::move(reflectedVariables);

    // Fill map "variable name" - "type".
#define VARIABLE_TYPE_TO_MAP(arrayType, variableType)                                                        \
    for (const auto& [sVariableName, value] : this->reflectedVariables.arrayType) {                          \
        variableNameToType[sVariableName] = variableType;                                                    \
    }

    VARIABLE_TYPE_TO_MAP(bools, ReflectedVariableType::BOOL);
    VARIABLE_TYPE_TO_MAP(ints, ReflectedVariableType::INT);
    VARIABLE_TYPE_TO_MAP(unsignedInts, ReflectedVariableType::UNSIGNED_INT);
    VARIABLE_TYPE_TO_MAP(longLongs, ReflectedVariableType::LONG_LONG);
    VARIABLE_TYPE_TO_MAP(unsignedLongLongs, ReflectedVariableType::UNSIGNED_LONG_LONG);
    VARIABLE_TYPE_TO_MAP(floats, ReflectedVariableType::FLOAT);
    VARIABLE_TYPE_TO_MAP(strings, ReflectedVariableType::STRING);
    VARIABLE_TYPE_TO_MAP(serializables, ReflectedVariableType::SERIALIZABLE);
    VARIABLE_TYPE_TO_MAP(vec2s, ReflectedVariableType::VEC2);
    VARIABLE_TYPE_TO_MAP(vec3s, ReflectedVariableType::VEC3);
    VARIABLE_TYPE_TO_MAP(vec4s, ReflectedVariableType::VEC4);
    VARIABLE_TYPE_TO_MAP(vectorInts, ReflectedVariableType::VECTOR_INT);
    VARIABLE_TYPE_TO_MAP(vectorStrings, ReflectedVariableType::VECTOR_STRING);
    VARIABLE_TYPE_TO_MAP(vectorVec3s, ReflectedVariableType::VECTOR_VEC3);
    VARIABLE_TYPE_TO_MAP(meshNodeGeometries, ReflectedVariableType::MESH_GEOMETRY);
    VARIABLE_TYPE_TO_MAP(skeletalMeshNodeGeometries, ReflectedVariableType::SKELETAL_MESH_GEOMETRY);
#if defined(WIN32) && defined(DEBUG)
    static_assert(sizeof(TypeReflectionInfo) == 1216, "add new variables here");
#elif defined(DEBUG)
    static_assert(sizeof(TypeReflectionInfo) == 1048, "add new variables here");
#endif
}
