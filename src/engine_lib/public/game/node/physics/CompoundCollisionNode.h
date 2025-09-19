#pragma once

// Custom.
#include "game/node/SpatialNode.h"

namespace JPH {
    class Body;
}

/**
 * Used to combine (group) child CollisionNodes to speed up collision detection and thus improve
 * performance. It's a good idea to group your level's CollisionNodes under a compound.
 */
class CompoundCollisionNode : public SpatialNode {
    // Notifies if its shape changes to recreate the compound.
    friend class CollisionNode;

    // Manages internal physics body.
    friend class PhysicsManager;

public:
    CompoundCollisionNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    CompoundCollisionNode(const std::string& sNodeName);

    virtual ~CompoundCollisionNode() override = default;

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
     * Called after @ref onSpawning when this node and all of node's child nodes (at the moment
     * of spawning) were spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark Generally you might want to prefer to use @ref onSpawning, this function
     * is mostly used to do some logic related to child nodes after all child nodes were spawned
     * (for example if you have a camera child node you can make it active in this function).
     */
    virtual void onChildNodesSpawned() override;

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
     * Called after some child node was detached from this node.
     *
     * @param pDetachedDirectChild Direct child node that was detached (child of this node, not a child of
     * some child node).
     */
    virtual void onAfterDirectChildDetached(Node* pDetachedDirectChild) override;

    /**
     * Called after some child node was attached to this node.
     *
     * @remark Called after @ref onAfterAttachedToNewParent is called on child nodes.
     *
     * @param pNewDirectChild New direct child node (child of this node, not a child of some child node).
     */
    virtual void onAfterNewDirectChildAttached(Node* pNewDirectChild) override;

private:
    /** Called by child CollisionNode after it changed its shape. */
    void onChildCollisionChangedShape();

    /** Creates @ref pBody. */
    void createPhysicsBody();

    /** Destroys @ref pBody. */
    void destroyPhysicsBody();

    /** Not `nullptr` when spawned. */
    JPH::Body* pBody = nullptr;

#if defined(DEBUG)
    /** Number of times @ref onChildCollisionChangedShape was called. */
    size_t iRecreateCompoundCount = 0;
#endif
};
