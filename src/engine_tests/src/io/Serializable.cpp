// Standard.
#include <filesystem>

// Custom.
#include "io/Serializable.h"
#include "TestFilePaths.hpp"

// External.
#include "nameof.hpp"
#include "catch2/catch_test_macros.hpp"

class TestSerializable : public Serializable {
public:
    static bool bDestructorCalled;
    static bool bOnAfterDeserializedCalled;

    TestSerializable() {
        bDestructorCalled = false;
        bOnAfterDeserializedCalled = false;
    };
    virtual ~TestSerializable() override { bDestructorCalled = true; };

    static std::string getTypeGuidStatic() { return "test-guid"; }
    virtual std::string getTypeGuid() const override { return "test-guid"; }

    static TypeReflectionInfo getReflectionInfo() {
        ReflectedVariables variables;

        variables.bools[NAMEOF_MEMBER(&TestSerializable::bBool).data()] = ReflectedVariableInfo<bool>{
            .setter =
                [](Serializable* pThis, const bool& bNewValue) {
                    reinterpret_cast<TestSerializable*>(pThis)->bBool = bNewValue;
                },
            .getter = [](Serializable* pThis) -> bool {
                return reinterpret_cast<TestSerializable*>(pThis)->bBool;
            }};

        variables.ints[NAMEOF_MEMBER(&TestSerializable::iInt).data()] = ReflectedVariableInfo<int>{
            .setter =
                [](Serializable* pThis, const int& iNewValue) {
                    reinterpret_cast<TestSerializable*>(pThis)->iInt = iNewValue;
                },
            .getter = [](Serializable* pThis) -> int {
                return reinterpret_cast<TestSerializable*>(pThis)->iInt;
            }};

        variables.unsignedInts[NAMEOF_MEMBER(&TestSerializable::iUnsignedInt).data()] =
            ReflectedVariableInfo<unsigned int>{
                .setter =
                    [](Serializable* pThis, const unsigned int& iNewValue) {
                        reinterpret_cast<TestSerializable*>(pThis)->iUnsignedInt = iNewValue;
                    },
                .getter = [](Serializable* pThis) -> unsigned int {
                    return reinterpret_cast<TestSerializable*>(pThis)->iUnsignedInt;
                }};

        variables.longLongs[NAMEOF_MEMBER(&TestSerializable::iLongLong).data()] =
            ReflectedVariableInfo<long long>{
                .setter =
                    [](Serializable* pThis, const long long& iNewValue) {
                        reinterpret_cast<TestSerializable*>(pThis)->iLongLong = iNewValue;
                    },
                .getter = [](Serializable* pThis) -> long long {
                    return reinterpret_cast<TestSerializable*>(pThis)->iLongLong;
                }};

        variables.unsignedLongLongs[NAMEOF_MEMBER(&TestSerializable::iUnsignedLongLong).data()] =
            ReflectedVariableInfo<unsigned long long>{
                .setter =
                    [](Serializable* pThis, const unsigned long long& iNewValue) {
                        reinterpret_cast<TestSerializable*>(pThis)->iUnsignedLongLong = iNewValue;
                    },
                .getter = [](Serializable* pThis) -> unsigned long long {
                    return reinterpret_cast<TestSerializable*>(pThis)->iUnsignedLongLong;
                }};

        variables.floats[NAMEOF_MEMBER(&TestSerializable::float_).data()] = ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<TestSerializable*>(pThis)->float_ = newValue;
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<TestSerializable*>(pThis)->float_;
            }};

        variables.strings[NAMEOF_MEMBER(&TestSerializable::sString).data()] =
            ReflectedVariableInfo<std::string>{
                .setter =
                    [](Serializable* pThis, const std::string& sNewValue) {
                        reinterpret_cast<TestSerializable*>(pThis)->sString = sNewValue;
                    },
                .getter = [](Serializable* pThis) -> std::string {
                    return reinterpret_cast<TestSerializable*>(pThis)->sString;
                }};

        variables.vec2s[NAMEOF_MEMBER(&TestSerializable::vec2).data()] = ReflectedVariableInfo<glm::vec2>{
            .setter =
                [](Serializable* pThis, const glm::vec2& newValue) {
                    reinterpret_cast<TestSerializable*>(pThis)->vec2 = newValue;
                },
            .getter = [](Serializable* pThis) -> glm::vec2 {
                return reinterpret_cast<TestSerializable*>(pThis)->vec2;
            }};

        variables.vec3s[NAMEOF_MEMBER(&TestSerializable::vec3).data()] = ReflectedVariableInfo<glm::vec3>{
            .setter =
                [](Serializable* pThis, const glm::vec3& newValue) {
                    reinterpret_cast<TestSerializable*>(pThis)->vec3 = newValue;
                },
            .getter = [](Serializable* pThis) -> glm::vec3 {
                return reinterpret_cast<TestSerializable*>(pThis)->vec3;
            }};

        variables.vec4s[NAMEOF_MEMBER(&TestSerializable::vec4).data()] = ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<TestSerializable*>(pThis)->vec4 = newValue;
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<TestSerializable*>(pThis)->vec4;
            }};

        variables.vectorInts[NAMEOF_MEMBER(&TestSerializable::vVectorInts).data()] =
            ReflectedVariableInfo<std::vector<int>>{
                .setter =
                    [](Serializable* pThis, const std::vector<int>& vNewValue) {
                        reinterpret_cast<TestSerializable*>(pThis)->vVectorInts = vNewValue;
                    },
                .getter = [](Serializable* pThis) -> std::vector<int> {
                    return reinterpret_cast<TestSerializable*>(pThis)->vVectorInts;
                }};

        variables.vectorStrings[NAMEOF_MEMBER(&TestSerializable::vVectorStrings).data()] =
            ReflectedVariableInfo<std::vector<std::string>>{
                .setter =
                    [](Serializable* pThis, const std::vector<std::string>& vNewValue) {
                        reinterpret_cast<TestSerializable*>(pThis)->vVectorStrings = vNewValue;
                    },
                .getter = [](Serializable* pThis) -> std::vector<std::string> {
                    return reinterpret_cast<TestSerializable*>(pThis)->vVectorStrings;
                }};

        variables.vectorVec3s[NAMEOF_MEMBER(&TestSerializable::vVectorVec3s).data()] =
            ReflectedVariableInfo<std::vector<glm::vec3>>{
                .setter =
                    [](Serializable* pThis, const std::vector<glm::vec3>& vNewValue) {
                        reinterpret_cast<TestSerializable*>(pThis)->vVectorVec3s = vNewValue;
                    },
                .getter = [](Serializable* pThis) -> std::vector<glm::vec3> {
                    return reinterpret_cast<TestSerializable*>(pThis)->vVectorVec3s;
                }};

        variables.meshGeometries[NAMEOF_MEMBER(&TestSerializable::meshGeometry).data()] =
            ReflectedVariableInfo<MeshGeometry>{
                .setter =
                    [](Serializable* pThis, const MeshGeometry& newValue) {
                        reinterpret_cast<TestSerializable*>(pThis)->meshGeometry = newValue;
                    },
                .getter = [](Serializable* pThis) -> MeshGeometry {
                    return reinterpret_cast<TestSerializable*>(pThis)->meshGeometry;
                }};

#if defined(WIN32) && defined(DEBUG)
        static_assert(sizeof(ReflectedVariables) == 1120, "add new variables here"); // NOLINT: current size
#endif

        return TypeReflectionInfo(
            "",
            NAMEOF_SHORT_TYPE(TestSerializable).data(),
            []() -> std::unique_ptr<Serializable> { return std::make_unique<TestSerializable>(); },
            std::move(variables));
    }

    virtual void onAfterDeserialized() override { bOnAfterDeserializedCalled = true; }

    bool bBool = false;
    int iInt = 0;
    unsigned int iUnsignedInt = 0;
    long long iLongLong = 0;
    unsigned long long iUnsignedLongLong = 0;
    float float_ = 0.0f;
    double double_ = 0.0;
    std::string sString;
    glm::vec2 vec2 = glm::vec2(0.0F, 0.0F);
    glm::vec3 vec3 = glm::vec3(0.0F, 0.0F, 0.0F);
    glm::vec4 vec4 = glm::vec4(0.0F, 0.0F, 0.0F, 0.0F);
    std::vector<int> vVectorInts;
    std::vector<std::string> vVectorStrings;
    std::vector<glm::vec3> vVectorVec3s;
    MeshGeometry meshGeometry;
};
bool TestSerializable::bDestructorCalled = false;
bool TestSerializable::bOnAfterDeserializedCalled = false;

class TestSerializableDerived : public TestSerializable {
public:
    TestSerializableDerived() = default;
    virtual ~TestSerializableDerived() override = default;

    static std::string getTypeGuidStatic() { return "test-derived-guid"; }
    virtual std::string getTypeGuid() const override { return "test-derived-guid"; }

    static TypeReflectionInfo getReflectionInfo() {
        ReflectedVariables variables;

        variables.ints[NAMEOF_MEMBER(&TestSerializableDerived::iDerivedInt).data()] =
            ReflectedVariableInfo<int>{
                .setter =
                    [](Serializable* pThis, const int& iNewValue) {
                        reinterpret_cast<TestSerializableDerived*>(pThis)->iDerivedInt = iNewValue;
                    },
                .getter = [](Serializable* pThis) -> int {
                    return reinterpret_cast<TestSerializableDerived*>(pThis)->iDerivedInt;
                }};

        return TypeReflectionInfo(
            TestSerializable::getTypeGuidStatic(),
            NAMEOF_SHORT_TYPE(TestSerializableDerived).data(),
            []() -> std::unique_ptr<Serializable> { return std::make_unique<TestSerializableDerived>(); },
            std::move(variables));
    }

    int iDerivedInt = 0;
};

TEST_CASE("serialize and deserialize a sample type") {
    // Register type.
    ReflectedTypeDatabase::registerType(
        TestSerializable::getTypeGuidStatic(), TestSerializable::getReflectionInfo());

    // Prepare object.
    auto pToSerialize = std::make_unique<TestSerializable>();
    pToSerialize->bBool = true;
    pToSerialize->iInt = -42;
    pToSerialize->iUnsignedInt = std::numeric_limits<unsigned int>::max();
    pToSerialize->iLongLong = std::numeric_limits<long long>::min();
    pToSerialize->iUnsignedLongLong = std::numeric_limits<unsigned long long>::max();
    pToSerialize->float_ = 3.1415926535f;
    pToSerialize->sString = "Hello! 今日は!";
    pToSerialize->vec2 = glm::vec2(1.0F, 2.0F);
    pToSerialize->vec3 = glm::vec3(1.0F, 2.0F, 3.0F);
    pToSerialize->vec4 = glm::vec4(1.0F, 2.0F, 3.0F, 4.0F);
    pToSerialize->vVectorInts = {-1, 0, 1, 2, 3};
    pToSerialize->vVectorStrings = {"Hello!", "今日は!"};
    pToSerialize->vVectorVec3s = {glm::vec3(1.0F, 2.0F, 3.0F), glm::vec3(3.0F, 2.0F, 1.0F)};
    MeshVertex vertex;
    vertex.position = glm::vec3(1.0F, 2.0F, 3.0F);
    vertex.normal = glm::vec3(1.0F, 0.0F, 0.0F);
    vertex.uv = glm::vec2(0.5F, 0.5F);
    pToSerialize->meshGeometry.getVertices().push_back(vertex);
    vertex.position = glm::vec3(4.0F, 5.0F, 6.0F);
    pToSerialize->meshGeometry.getVertices().push_back(vertex);
    vertex.position = glm::vec3(7.0F, 8.0F, 9.0F);
    pToSerialize->meshGeometry.getVertices().push_back(vertex);
    pToSerialize->meshGeometry.getIndices() = {0, 1, 2};

    // Serialize.
    const auto pathToFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sTestDirName / vUsedTestFileNames[0];
    auto optionalError = pToSerialize->serialize(pathToFile, false);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        INFO(optionalError->getFullErrorMessage());
        REQUIRE(false);
    }

    // Deserialize.
    REQUIRE(!TestSerializable::bOnAfterDeserializedCalled);
    auto result = Serializable::deserialize<TestSerializable>(pathToFile);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        INFO(error.getFullErrorMessage());
        REQUIRE(false);
    }
    auto pDeserialized = std::get<std::unique_ptr<TestSerializable>>(std::move(result));
    REQUIRE(!TestSerializable::bDestructorCalled);
    REQUIRE(TestSerializable::bOnAfterDeserializedCalled);

    constexpr float floatEpsilon = 0.00001f;

    REQUIRE(pDeserialized->bBool == pToSerialize->bBool);
    REQUIRE(pDeserialized->iInt == pToSerialize->iInt);
    REQUIRE(pDeserialized->iUnsignedInt == pToSerialize->iUnsignedInt);
    REQUIRE(pDeserialized->iLongLong == pToSerialize->iLongLong);
    REQUIRE(pDeserialized->iUnsignedLongLong == pToSerialize->iUnsignedLongLong);
    REQUIRE(glm::epsilonEqual(pDeserialized->float_, pToSerialize->float_, floatEpsilon));
    REQUIRE(pDeserialized->sString == pToSerialize->sString);
    REQUIRE(glm::all(glm::epsilonEqual(pDeserialized->vec2, pToSerialize->vec2, floatEpsilon)));
    REQUIRE(glm::all(glm::epsilonEqual(pDeserialized->vec3, pToSerialize->vec3, floatEpsilon)));
    REQUIRE(glm::all(glm::epsilonEqual(pDeserialized->vec4, pToSerialize->vec4, floatEpsilon)));
    REQUIRE(pDeserialized->vVectorInts == pToSerialize->vVectorInts);
    REQUIRE(pDeserialized->vVectorStrings == pToSerialize->vVectorStrings);
    REQUIRE(pDeserialized->vVectorVec3s.size() == pToSerialize->vVectorVec3s.size());
    for (size_t i = 0; i < pDeserialized->vVectorVec3s.size(); i++) {
        REQUIRE(
            glm::all(
                glm::epsilonEqual(
                    pDeserialized->vVectorVec3s[i], pToSerialize->vVectorVec3s[i], floatEpsilon)));
    }
    REQUIRE(pDeserialized->meshGeometry == pToSerialize->meshGeometry);

#if defined(WIN32) && defined(DEBUG)
    static_assert(sizeof(ReflectedVariables) == 1120, "add new variables here"); // NOLINT: current size
#endif

    pDeserialized = nullptr;
    REQUIRE(TestSerializable::bDestructorCalled);
}

TEST_CASE("serialize and deserialize a derived type") {
    // Register types.
    ReflectedTypeDatabase::registerType(
        TestSerializable::getTypeGuidStatic(), TestSerializable::getReflectionInfo());
    ReflectedTypeDatabase::registerType(
        TestSerializableDerived::getTypeGuidStatic(), TestSerializableDerived::getReflectionInfo());

    // Prepare object.
    auto pToSerialize = std::make_unique<TestSerializableDerived>();
    pToSerialize->iInt = -42;
    pToSerialize->iDerivedInt = 123;

    // Serialize.
    const auto pathToFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sTestDirName / vUsedTestFileNames[1];
    auto optionalError = pToSerialize->serialize(pathToFile, false);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        INFO(optionalError->getFullErrorMessage());
        REQUIRE(false);
    }

    // Deserialize.
    auto result = Serializable::deserialize<TestSerializableDerived>(pathToFile);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        INFO(error.getFullErrorMessage());
        REQUIRE(false);
    }
    auto pDeserialized = std::get<std::unique_ptr<TestSerializableDerived>>(std::move(result));
    REQUIRE(!TestSerializable::bDestructorCalled);

    REQUIRE(pDeserialized->iInt == pToSerialize->iInt); // parent variables were also saved
    REQUIRE(pDeserialized->iDerivedInt == pToSerialize->iDerivedInt);
}

TEST_CASE("deserialize with original object") {
    const auto pathToOriginalFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sTestDirName / vUsedTestFileNames[5];
    const auto pathToModifiedFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sTestDirName / vUsedTestFileNames[6];

    {
        // Create 2 objects to serialize in a single file.
        auto pToSerialize1 = std::make_unique<TestSerializable>();
        auto pToSerialize2 = std::make_unique<TestSerializable>();

        pToSerialize1->iInt = 100;
        pToSerialize2->iInt = 200;

        // Serialize.
        auto optionalError = Serializable::serializeMultiple(
            pathToOriginalFile,
            {SerializableObjectInformation(pToSerialize1.get(), "0"),
             SerializableObjectInformation(pToSerialize2.get(), "1")},
            false);
        if (optionalError.has_value()) [[unlikely]] {
            optionalError->addCurrentLocationToErrorStack();
            INFO(optionalError->getFullErrorMessage());
            REQUIRE(false);
        }
    }

    {
        // Deserialize.
        auto result = Serializable::deserializeMultiple<Serializable>(pathToOriginalFile);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            INFO(error.getFullErrorMessage());
            REQUIRE(false);
        }
        auto vDeserializedObjects =
            std::get<std::vector<DeserializedObjectInformation<std::unique_ptr<Serializable>>>>(
                std::move(result));

        // Check correctness.
        REQUIRE(vDeserializedObjects.size() == 2);

        const auto pDeserialized1 = dynamic_cast<TestSerializable*>(vDeserializedObjects[0].pObject.get());
        const auto pDeserialized2 = dynamic_cast<TestSerializable*>(vDeserializedObjects[1].pObject.get());
        REQUIRE(pDeserialized1 != nullptr);
        REQUIRE(pDeserialized2 != nullptr);

        REQUIRE(pDeserialized1->iInt == 100);
        REQUIRE(pDeserialized2->iInt == 200);

        // Modify 2nd object.
        pDeserialized2->sString = "Hello!";

        // Serialize 2nd object, it should have a reference to the original object (1 of the objects from the
        // original file, we need to make sure it references the correct object but not just the first one).
        auto optionalError = pDeserialized2->serialize(pathToModifiedFile, false);
        if (optionalError.has_value()) [[unlikely]] {
            optionalError->addCurrentLocationToErrorStack();
            INFO(optionalError->getFullErrorMessage());
            REQUIRE(false);
        }

        // Find reference to the original.
        ConfigManager modifiedToml;
        optionalError = modifiedToml.loadFile(pathToModifiedFile);
        if (optionalError.has_value()) [[unlikely]] {
            optionalError->addCurrentLocationToErrorStack();
            INFO(optionalError->getFullErrorMessage());
            REQUIRE(false);
        }

        auto vKeys = modifiedToml.getAllKeysOfSection("0.test-guid");
        REQUIRE(!vKeys.empty());
        bool bFoundReferenceToOriginal = false;
        for (const auto& sKeyName : vKeys) {
            if (sKeyName.starts_with(".path_to_original")) {
                bFoundReferenceToOriginal = true;
                break;
            }
        }
        REQUIRE(bFoundReferenceToOriginal);
    }

    // Deserialize 2nd object.
    auto result = Serializable::deserialize<TestSerializable>(pathToModifiedFile);
    if (std::holds_alternative<Error>(result)) [[unlikely]] {
        auto error = std::get<Error>(std::move(result));
        error.addCurrentLocationToErrorStack();
        INFO(error.getFullErrorMessage());
        REQUIRE(false);
    }
    auto pDeserialized2 = std::get<std::unique_ptr<TestSerializable>>(std::move(result));

    REQUIRE(pDeserialized2->iInt == 200);
    REQUIRE(pDeserialized2->sString == "Hello!");
}

TEST_CASE("deserialize, change, serialize in the same file (no reference to the original)") {
    const auto pathToFile =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sTestDirName / vUsedTestFileNames[7];

    {
        auto pToSerialize = std::make_unique<TestSerializable>();
        pToSerialize->iInt = 100;

        // Serialize.
        auto optionalError = pToSerialize->serialize(pathToFile, false);
        if (optionalError.has_value()) [[unlikely]] {
            optionalError->addCurrentLocationToErrorStack();
            INFO(optionalError->getFullErrorMessage());
            REQUIRE(false);
        }
    }

    {
        // Deserialize.
        auto result = Serializable::deserialize<TestSerializable>(pathToFile);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            INFO(error.getFullErrorMessage());
            REQUIRE(false);
        }
        auto pDeserialized = std::get<std::unique_ptr<TestSerializable>>(std::move(result));

        REQUIRE(pDeserialized->iInt == 100);

        pDeserialized->iInt = 200;

        // Serialize.
        auto optionalError = pDeserialized->serialize(pathToFile, false);
        if (optionalError.has_value()) [[unlikely]] {
            optionalError->addCurrentLocationToErrorStack();
            INFO(optionalError->getFullErrorMessage());
            REQUIRE(false);
        }

        // Find reference to the original.
        ConfigManager modifiedToml;
        optionalError = modifiedToml.loadFile(pathToFile);
        if (optionalError.has_value()) [[unlikely]] {
            optionalError->addCurrentLocationToErrorStack();
            INFO(optionalError->getFullErrorMessage());
            REQUIRE(false);
        }

        auto vKeys = modifiedToml.getAllKeysOfSection("0.test-guid");
        REQUIRE(!vKeys.empty());
        bool bFoundReferenceToOriginal = false;
        for (const auto& sKeyName : vKeys) {
            if (sKeyName.starts_with(".path_to_original")) {
                bFoundReferenceToOriginal = true;
                break;
            }
        }
        REQUIRE(
            !bFoundReferenceToOriginal); // because we overwritten the same file there should be no reference
    }
}
