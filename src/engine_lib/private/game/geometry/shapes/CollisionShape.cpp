#include "game/geometry/shapes/CollisionShape.h"

// Custom.
#include "misc/Error.h"
#include "game/physics/CoordinateConversions.hpp"

// External.
#include "nameof.hpp"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"

namespace {
    constexpr std::string_view sCollisionShapeTypeGuid = "ced888f6-24f5-425b-a600-ad78f8868593";
    constexpr std::string_view sBoxCollisionShapeTypeGuid = "985ceb65-0afb-439f-b931-3cb649e325b8";
    constexpr std::string_view sSphereCollisionShapeTypeGuid = "6da8ba64-ade4-4be2-bc53-e9de3f88d60f";
    constexpr std::string_view sCapsuleCollisionShapeTypeGuid = "5383bd42-6857-45e2-a45e-8bc799abc387";
    constexpr std::string_view sCylinderCollisionShapeTypeGuid = "a9dca02e-4283-4d62-bed7-85de22c9af7e";
}

std::string CollisionShape::getTypeGuidStatic() { return sCollisionShapeTypeGuid.data(); }
std::string CollisionShape::getTypeGuid() const { return sCollisionShapeTypeGuid.data(); }
TypeReflectionInfo CollisionShape::getReflectionInfo() {
    ReflectedVariables variables;

    return TypeReflectionInfo(
        "",
        NAMEOF_SHORT_TYPE(CollisionShape).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<CollisionShape>(); },
        std::move(variables));
}

JPH::Result<JPH::Ref<JPH::Shape>> CollisionShape::createShape() {
    Error::showErrorAndThrowException("derived type not implemented this method");
}

// ------------------------------------------------------------------------------------------------

std::string BoxCollisionShape::getTypeGuidStatic() { return sBoxCollisionShapeTypeGuid.data(); }
std::string BoxCollisionShape::getTypeGuid() const { return sBoxCollisionShapeTypeGuid.data(); }
TypeReflectionInfo BoxCollisionShape::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec3s[NAMEOF_MEMBER(&BoxCollisionShape::halfExtent).data()] = ReflectedVariableInfo<glm::vec3>{
        .setter =
            [](Serializable* pThis, const glm::vec3& newValue) {
                reinterpret_cast<BoxCollisionShape*>(pThis)->setHalfExtent(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec3 {
            return reinterpret_cast<BoxCollisionShape*>(pThis)->getHalfExtent();
        }};

    return TypeReflectionInfo(
        CollisionShape::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(BoxCollisionShape).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<BoxCollisionShape>(); },
        std::move(variables));
}

void BoxCollisionShape::setHalfExtent(const glm::vec3& size) {
    halfExtent = glm::max(size, glm::vec3(0.01F));
}

JPH::Result<JPH::Ref<JPH::Shape>> BoxCollisionShape::createShape() {
    JPH::BoxShapeSettings settings(convertToJolt(halfExtent));
    return settings.Create();
}

// ------------------------------------------------------------------------------------------------

std::string SphereCollisionShape::getTypeGuidStatic() { return sSphereCollisionShapeTypeGuid.data(); }
std::string SphereCollisionShape::getTypeGuid() const { return sSphereCollisionShapeTypeGuid.data(); }
TypeReflectionInfo SphereCollisionShape::getReflectionInfo() {
    ReflectedVariables variables;

    variables.floats[NAMEOF_MEMBER(&SphereCollisionShape::radius).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<SphereCollisionShape*>(pThis)->setRadius(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SphereCollisionShape*>(pThis)->getRadius();
        }};

    return TypeReflectionInfo(
        CollisionShape::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SphereCollisionShape).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SphereCollisionShape>(); },
        std::move(variables));
}

void SphereCollisionShape::setRadius(float size) { radius = std::max(size, 0.01F); }

JPH::Result<JPH::Ref<JPH::Shape>> SphereCollisionShape::createShape() {
    JPH::SphereShapeSettings settings(radius);
    return settings.Create();
}

// ------------------------------------------------------------------------------------------------

std::string CapsuleCollisionShape::getTypeGuidStatic() { return sCapsuleCollisionShapeTypeGuid.data(); }
std::string CapsuleCollisionShape::getTypeGuid() const { return sCapsuleCollisionShapeTypeGuid.data(); }
TypeReflectionInfo CapsuleCollisionShape::getReflectionInfo() {
    ReflectedVariables variables;

    variables.floats[NAMEOF_MEMBER(&CapsuleCollisionShape::radius).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<CapsuleCollisionShape*>(pThis)->setRadius(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<CapsuleCollisionShape*>(pThis)->getRadius();
        }};

    variables.floats[NAMEOF_MEMBER(&CapsuleCollisionShape::halfHeight).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<CapsuleCollisionShape*>(pThis)->setHalfHeight(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<CapsuleCollisionShape*>(pThis)->getHalfHeight();
        }};

    return TypeReflectionInfo(
        CollisionShape::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(CapsuleCollisionShape).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<CapsuleCollisionShape>(); },
        std::move(variables));
}

void CapsuleCollisionShape::setRadius(float size) { radius = std::max(size, 0.01F); }
void CapsuleCollisionShape::setHalfHeight(float size) { halfHeight = std::max(size, 0.01F); }

JPH::Result<JPH::Ref<JPH::Shape>> CapsuleCollisionShape::createShape() {
    JPH::CapsuleShapeSettings settings(halfHeight, radius);
    return settings.Create();
}

// ------------------------------------------------------------------------------------------------

std::string CylinderCollisionShape::getTypeGuidStatic() { return sCylinderCollisionShapeTypeGuid.data(); }
std::string CylinderCollisionShape::getTypeGuid() const { return sCylinderCollisionShapeTypeGuid.data(); }
TypeReflectionInfo CylinderCollisionShape::getReflectionInfo() {
    ReflectedVariables variables;

    variables.floats[NAMEOF_MEMBER(&CylinderCollisionShape::radius).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<CylinderCollisionShape*>(pThis)->setRadius(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<CylinderCollisionShape*>(pThis)->getRadius();
        }};

    variables.floats[NAMEOF_MEMBER(&CylinderCollisionShape::halfHeight).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<CylinderCollisionShape*>(pThis)->setHalfHeight(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<CylinderCollisionShape*>(pThis)->getHalfHeight();
            }};

    return TypeReflectionInfo(
        CollisionShape::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(CylinderCollisionShape).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<CylinderCollisionShape>(); },
        std::move(variables));
}

void CylinderCollisionShape::setRadius(float size) { radius = std::max(size, 0.01F); }
void CylinderCollisionShape::setHalfHeight(float size) { halfHeight = std::max(size, 0.01F); }

JPH::Result<JPH::Ref<JPH::Shape>> CylinderCollisionShape::createShape() {
    JPH::CylinderShapeSettings settings(halfHeight, radius);
    return settings.Create();
}
