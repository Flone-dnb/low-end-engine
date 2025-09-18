#pragma once

// Standard.
#include <functional>

// Custom.
#include "math/GLMath.hpp"
#include "io/Serializable.h"

namespace JPH {
    template <typename T> class Result;
    template <typename T> class Ref;
    class Shape;
}

/** Base class for various collision shapes. */
class CollisionShape : public Serializable {
    // Requests shape creation.
    friend class PhysicsManager;

public:
    CollisionShape() = default;
    virtual ~CollisionShape() = default;

    /**
     * Sets a callback that will be triggered after some property of the shape changed.
     *
     * @param callback Callback.
     */
    void setOnChanged(const std::function<void()>& callback);

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

protected:
    /**
     * Creates a shape for Jolt physics.
     *
     * @warning Must be implemented by child types, shows error in base implementation.
     *
     * @return Shape.
     */
    virtual JPH::Result<JPH::Ref<JPH::Shape>> createShape();

    /** Must be called by derived classes after they change some property of the shape. */
    void propertyChanged();

private:
    /** Called after some property of the shape was changed. */
    std::function<void()> onChanged;
};

// ------------------------------------------------------------------------------------------------

/** Box collision. */
class BoxCollisionShape : public CollisionShape {
public:
    BoxCollisionShape() = default;
    virtual ~BoxCollisionShape() override = default;

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Sets half the size of the box.
     *
     * @param size Size.
     */
    void setHalfExtent(const glm::vec3& size);

    /**
     * Returns half the size of the box.
     *
     * @return Size.
     */
    glm::vec3 getHalfExtent() const { return halfExtent; }

protected:
    /**
     * Creates a shape for Jolt physics.
     *
     * @return Shape.
     */
    virtual JPH::Result<JPH::Ref<JPH::Shape>> createShape() override;

private:
    /** Half the size of the box. */
    glm::vec3 halfExtent = glm::vec3(0.5F, 0.5F, 0.5F);
};

// ------------------------------------------------------------------------------------------------

/** Sphere collision. */
class SphereCollisionShape : public CollisionShape {
public:
    SphereCollisionShape() = default;
    virtual ~SphereCollisionShape() override = default;

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Sets radius of the sphere.
     *
     * @param size Radius.
     */
    void setRadius(float size);

    /**
     * Returns radius of the sphere.
     *
     * @return Radius.
     */
    float getRadius() const { return radius; }

protected:
    /**
     * Creates a shape for Jolt physics.
     *
     * @return Shape.
     */
    virtual JPH::Result<JPH::Ref<JPH::Shape>> createShape() override;

private:
    /** Radius of the sphere. */
    float radius = 0.5F;
};

// ------------------------------------------------------------------------------------------------

/** Capsule collision. */
class CapsuleCollisionShape : public CollisionShape {
public:
    CapsuleCollisionShape() = default;
    virtual ~CapsuleCollisionShape() override = default;

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Sets radius of the capsule.
     *
     * @param size Radius.
     */
    void setRadius(float size);

    /**
     * Sets half height of the capsule.
     *
     * @param size Size.
     */
    void setHalfHeight(float size);

    /**
     * Returns half height of the capsule.
     *
     * @return Size.
     */
    float getHalfHeight() const { return halfHeight; }

    /**
     * Returns radius of the capsule.
     *
     * @return Radius.
     */
    float getRadius() const { return radius; }

protected:
    /**
     * Creates a shape for Jolt physics.
     *
     * @return Shape.
     */
    virtual JPH::Result<JPH::Ref<JPH::Shape>> createShape() override;

private:
    /** Half height of the capsule. */
    float halfHeight = 1.0F;

    /** Radius of the capsule. */
    float radius = 0.15F;
};

// ------------------------------------------------------------------------------------------------

/** Cylinder collision. */
class CylinderCollisionShape : public CollisionShape {
public:
    CylinderCollisionShape() = default;
    virtual ~CylinderCollisionShape() override = default;

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Sets radius of the cylinder.
     *
     * @param size Radius.
     */
    void setRadius(float size);

    /**
     * Sets half height of the cylinder.
     *
     * @param size Size.
     */
    void setHalfHeight(float size);

    /**
     * Returns half height of the cylinder.
     *
     * @return Size.
     */
    float getHalfHeight() const { return halfHeight; }

    /**
     * Returns radius of the cylinder.
     *
     * @return Radius.
     */
    float getRadius() const { return radius; }

protected:
    /**
     * Creates a shape for Jolt physics.
     *
     * @return Shape.
     */
    virtual JPH::Result<JPH::Ref<JPH::Shape>> createShape() override;

private:
    /** Half height of the cylinder. */
    float halfHeight = 0.5F;

    /** Radius of the cylinder. */
    float radius = 0.25F;
};

// ------------------------------------------------------------------------------------------------

/** Convex collision. */
class ConvexCollisionShape : public CollisionShape {
public:
    ConvexCollisionShape() = default;
    virtual ~ConvexCollisionShape() override = default;

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Sets path (relative to the `res` directory) to the file that stores convex shape geometry.
     *
     * @param sRelativePath New path.
     */
    void setPathToGeometryRelativeRes(const std::string& sRelativePath);

    /**
     * Returns path (relative to the `res` directory) to the file that stores convex shape geometry.
     *
     * @return Relative path.
     */
    std::string getPathToGeometryRelativeRes() const { return sPathToGeometryRelativeRes; }

protected:
    /**
     * Creates a shape for Jolt physics.
     *
     * @return Shape.
     */
    virtual JPH::Result<JPH::Ref<JPH::Shape>> createShape() override;

private:
    /** Path (relative to the `res` directory) to the file that stores convex shape geometry. */
    std::string sPathToGeometryRelativeRes;
};
