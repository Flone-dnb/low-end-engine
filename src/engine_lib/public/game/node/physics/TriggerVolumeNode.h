#pragma once

// Standard.
#include <memory>

// Custom.
#include "game/node/SpatialNode.h"

namespace JPH {
    class Body;
}

class CollisionShape;

/** Reports contacts with other physics bodies (but does not block them).  */
class TriggerVolumeNode : public SpatialNode {
public:
    // Manages internal physics body.
    friend class PhysicsManager;

public:
    TriggerVolumeNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    TriggerVolumeNode(const std::string& sNodeName);

    virtual ~TriggerVolumeNode() override = default;

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
     * Used to temporary disable the trigger (even when spawned).
     *
     * @param bEnable New state.
     */
    void setIsTriggerEnabled(bool bEnable);

    /**
     * Returns used collision shape.
     *
     * @return Shape.
     */
    CollisionShape& getShape() const;

    /**
     * Tells if the trigger is temporary disabled (can be used while spawned).
     *
     * @return Is active.
     */
    bool isTriggerEnabled() const { return bIsTriggerEnabled; }

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
     * Called after the physics update to notify that there was a contact with another physics body.
     *
     * @param pNode            Node that has contact with the character.
     * @param hitWorldPosition World position of the hit.
     * @param hitNormal        Normal of the hit.
     */
    virtual void onContactAdded(Node* pNode, const glm::vec3& hitWorldPosition, const glm::vec3& hitNormal) {}

    /**
     * Called after the physics update to notify that a contact with another physics body was lost.
     *
     * @param pNode Node that had contact with the character.
     */
    virtual void onContactRemoved(Node* pNode) {}

private:
    /** Sets `onChanged` callback to @ref pShape. */
    void setOnShapeChangedCallback();

    /** Collision shape. */
    std::unique_ptr<CollisionShape> pShape;

    /** Not `nullptr` when spawned. */
    JPH::Body* pBody = nullptr;

    /** Used to temporary disable the trigger while spawned. */
    bool bIsTriggerEnabled = true;
};