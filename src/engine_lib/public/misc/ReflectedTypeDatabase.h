#pragma once

// Standard.
#include <functional>
#include <unordered_set>
#include <string>
#include <memory>

// Custom.
#include "game/geometry/MeshNodeGeometry.h"
#include "game/geometry/SkeletalMeshNodeGeometry.h"
#include "math/GLMath.hpp"

// To suppress GCC's false-positive about dangling reference on `getTypeInfo`.
#if __GNUC__ >= 13
#define GCC_PUSH_DIAGNOSTIC_DISABLE_DANGLING_REF                                                             \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdangling-reference\"")

#define GCC_DIAGNOSTIC_POP _Pragma("GCC diagnostic pop")
#else
#define GCC_PUSH_DIAGNOSTIC_DISABLE_DANGLING_REF
#define GCC_DIAGNOSTIC_POP
#endif

class Serializable;

/** Groups information about a reflected field/variable of a type. */
template <typename VariableType> struct ReflectedVariableInfo {
    /** Function to set a new value. */
    std::function<void(Serializable* pThis, const VariableType& value)> setter;

    /** Function to get the value. */
    std::function<VariableType(Serializable* pThis)> getter;
};

/** Supported types of reflected variables. */
enum class ReflectedVariableType {
    BOOL,
    INT,
    UNSIGNED_INT,
    LONG_LONG,
    UNSIGNED_LONG_LONG,
    FLOAT,
    STRING,
    VEC2,
    VEC3,
    VEC4,
    VECTOR_INT,
    VECTOR_STRING,
    VECTOR_VEC3,
    MESH_GEOMETRY,
    SKELETAL_MESH_GEOMETRY,
};

/** Groups info about reflected variables. */
struct ReflectedVariables {
    /**
     * Checks that names of all reflected variables are unique and returns a set of variable names.
     *
     * @return Names of reflected variables.
     */
    std::unordered_set<std::string> collectVariableNames() const;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<bool>> bools;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<int>> ints;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<unsigned int>> unsignedInts;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<long long>> longLongs;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<unsigned long long>> unsignedLongLongs;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<float>> floats;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<std::string>> strings;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<glm::vec2>> vec2s;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<glm::vec3>> vec3s;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<glm::vec4>> vec4s;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<std::vector<int>>> vectorInts;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<std::vector<std::string>>> vectorStrings;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<std::vector<glm::vec3>>> vectorVec3s;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<MeshNodeGeometry>> meshNodeGeometries;

    /** Pairs of "variable name" - "variable info". */
    std::unordered_map<std::string, ReflectedVariableInfo<SkeletalMeshNodeGeometry>>
        skeletalMeshNodeGeometries;
};

/** Groups information about a reflected type. */
class TypeReflectionInfo {
    friend class Serializable;

public:
    TypeReflectionInfo() = delete;

    /**
     * Creates a new object.
     *
     * @param sParentTypeGuid    Specify empty string if this type does not have a Serializable-derived
     * parent, otherwise stores GUID of the parent.
     * @param sTypeName          Name of the class/struct.
     * @param createNewObject    Creates a new object of this type.
     * @param reflectedVariables Info about reflected variables of this type (don't include parent
     * variables, they will be automatically added after construction).
     */
    TypeReflectionInfo(
        const std::string& sParentTypeGuid,
        const std::string& sTypeName,
        const std::function<std::unique_ptr<Serializable>()>& createNewObject,
        ReflectedVariables&& reflectedVariables);

    /**
     * Info about reflected variables (including inherited variables).
     *
     * @warning You should not modify this after the object was constructed.
     */
    ReflectedVariables reflectedVariables;

    /** Empty if this type does not have a Serializable parent, otherwise stores GUID of the parent. */
    const std::string sParentTypeGuid;

    /** Name of the class/struct. */
    const std::string sTypeName;

    /** Creates a new object of this type. */
    const std::function<std::unique_ptr<Serializable>()> createNewObject;

private:
    /** For quick search into @ref reflectedVariables. Initialized during construction. */
    std::unordered_map<std::string, ReflectedVariableType> variableNameToType;
};

/** Stores reflection info of all reflected types. */
class ReflectedTypeDatabase {
    // Registers engine types.
    friend class GameManager;

public:
    /**
     * Registers reflection info of a type.
     *
     * @remark Nothing will happen if you try to register the same type with the same GUID again.
     *
     * @param sTypeGuid GUID of the type.
     * @param typeInfo  Type info.
     */
    static void registerType(const std::string& sTypeGuid, TypeReflectionInfo&& typeInfo);

    /**
     * Returns reflection info about a type.
     *
     * @remark If the GUID is already used by some other type an error message will be shown.
     *
     * @param sTypeGuid GUID of the type.
     *
     * @return Reflection info.
     */
    static const TypeReflectionInfo& getTypeInfo(const std::string& sTypeGuid);

    /**
     * Returns pairs of "type GUID" - "type info".
     *
     * @return Types info.
     */
    static const std::unordered_map<std::string, TypeReflectionInfo>& getReflectedTypes() {
        return reflectedTypes;
    }

private:
    /** Called by game manager to register serializable type of the engine. */
    static void registerEngineTypes();

    /** Pairs of "type GUID" - "type info". */
    static std::unordered_map<std::string, TypeReflectionInfo> reflectedTypes;
};
