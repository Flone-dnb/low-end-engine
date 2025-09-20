#pragma once

// Standard.
#include <memory>

// Custom.
#include "game/node/SpatialNode.h"

namespace JPH {
    class Body;
}

class CollisionShape;

/** Physically simulated body that is moved by forces. */
class DynamicBodyNode : public SpatialNode {
    // Manages internal physics body.
    friend class PhysicsManager;

public:
    DynamicBodyNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    DynamicBodyNode(const std::string& sNodeName);

    virtual ~DynamicBodyNode() override = default;

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
     * Sets a new shape.
     *
     * @param pNewShape New shape.
     */
    void setShape(std::unique_ptr<CollisionShape> pNewShape);

    /**
     * Specify `true` if this body should be simulated, `false` if the simulation should be paused for this
     * body.
     *
     * @remark Note that the body will not be simulated while despawned.
     *
     * @param bActivate New state.
     */
    void setIsSimulated(bool bActivate);

    /**
     * Returns used collision shape.
     *
     * @return `nullptr` if not set.
     */
    CollisionShape* getShape() const { return pShape.get(); }

    /**
     * `true` if this body is simulated, `false` if the simulation is paused for this body.
     *
     * @return State.
     */
    bool isSimulated() const { return bIsSimulated; }

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

private:
    /** Sets `onChanged` callback to @ref pShape. */
    void setOnShapeChangedCallback();

    /** Collision shape. */
    std::unique_ptr<CollisionShape> pShape;

    /** Not `nullptr` when spawned. */
    JPH::Body* pBody = nullptr;

    /** `false` to pause simulation for this body. */
    bool bIsSimulated = true;

#if defined(DEBUG)
    /** `true` if we produced a warning in case the body fell out of the world. */
    bool bWarnedAboutFallingOutOfWorld = false;
#endif
};
