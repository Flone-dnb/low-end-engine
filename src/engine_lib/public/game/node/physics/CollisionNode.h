#pragma once

// Standard.
#include <memory>

// Custom.
#include "game/node/SpatialNode.h"

namespace JPH {
    class Body;
}

class CollisionShape;

/**
 * Used to create walls, floors and other solid objects that do not allow to move
 * through them. Note that moving or rotating such nodes is perfectly fine even when they are spawned
 * (unless they are part of a CompoundCollisionNode, then moving/rotating them is not recommended).
 */
class CollisionNode : public SpatialNode {
    // Manages internal physics body.
    friend class PhysicsManager;

public:
    CollisionNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    CollisionNode(const std::string& sNodeName);

    virtual ~CollisionNode() override = default;

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
     * Used to temporary disable collision (even when spawned).
     *
     * @param bEnable New state.
     */
    void setIsCollisionEnabled(bool bEnable);

    /**
     * Sets a new shape.
     *
     * @param pNewShape New shape.
     */
    void setShape(std::unique_ptr<CollisionShape> pNewShape);

    /**
     * Returns used collision shape.
     *
     * @return Shape.
     */
    CollisionShape& getShape() const;

    /**
     * Tells if the collision is temporary disabled (can be used while spawned).
     *
     * @return Is active.
     */
    bool isCollisionEnabled() const { return bIsCollisionEnabled; }

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node to execute custom spawn logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     *
     * @remark This function is called before any of the child nodes are spawned. If you
     * need to do some logic after child nodes are spawned use @ref onChildNodesSpawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onSpawning() override;

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     */
    virtual void onDespawning() override;

    /**
     * Called after node's world location/rotation/scale was changed.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark If you change location/rotation/scale inside of this function,
     * this function will not be called again (no recursion will occur).
     */
    virtual void onWorldLocationRotationScaleChanged() override;

    /**
     * Called after this node or one of the node's parents (in the parent hierarchy)
     * was attached to a new parent node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This function will also be called on all child nodes after this function
     * is finished.
     *
     * @param bThisNodeBeingAttached `true` if this node was attached to a parent,
     * `false` if some node in the parent hierarchy was attached to a parent.
     */
    virtual void onAfterAttachedToNewParent(bool bThisNodeBeingAttached) override;

private:
    /** Sets `onChanged` callback to @ref pShape. */
    void setOnShapeChangedCallback();

    /** Collision shape. */
    std::unique_ptr<CollisionShape> pShape;

    /** Not `nullptr` when spawned. */
    JPH::Body* pBody = nullptr;

    /** Used to temporary disable collision while spawned. */
    bool bIsCollisionEnabled = true;
};
