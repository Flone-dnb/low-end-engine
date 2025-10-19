#pragma once

// Standard.
#include <mutex>

// Custom.
#include "game/node/SpatialNode.h"

class SkeletonNode;

/** Should be child node of a skeleton node, attaches to a specific bone of the skeleton. */
class SkeletonBoneAttachmentNode : public SpatialNode {
public:
    SkeletonBoneAttachmentNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    SkeletonBoneAttachmentNode(const std::string& sNodeName);

    virtual ~SkeletonBoneAttachmentNode() override = default;

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
     * Sets skeleton bone index to attach to.
     *
     * @param iNewBoneIndex Bone index.
     */
    void setBoneIndex(unsigned int iNewBoneIndex);

    /**
     * Returns index of the skeleton bone to attach to.
     *
     * @return Bone index.
     */
    unsigned int getBoneIndex() const { return iBoneIndex; }

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     * @remark This function is called before any of the child nodes are spawned. If you
     * need to do some logic after child nodes are spawned use @ref onChildNodesSpawned.
     */
    virtual void onSpawning() override;

    /**
     * Called before a new frame is rendered.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic (if there is any).
     *
     * @remark This function is disabled by default, use @ref setIsCalledEveryFrame to enable it.
     * @remark This function will only be called while this node is spawned.
     *
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    virtual void onBeforeNewFrame(float timeSincePrevFrameInSec) override;

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
    /** `nullptr` if no parent node or if not a skeleton node, otherwise skeleton parent node. */
    std::pair<std::mutex, SkeletonNode*> mtxCachedSkeletonParent;

    /** Index of the skeleton bone to attach to. */
    unsigned int iBoneIndex = 0;
};
