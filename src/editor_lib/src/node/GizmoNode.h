#pragma once

// Standard.
#include <optional>

// Custom.
#include "game/node/SpatialNode.h"
#include "node/GizmoMode.hpp"

class MeshNode;

/** Gizmo element that controls a specific axis. */
enum class GizmoAxis {
    X,
    Y,
    Z,
};

/** Spawn to create a 3D gizmo (move, rotate or scale) to control a spatial node's transform. */
class GizmoNode : public SpatialNode {
public:
    GizmoNode() = delete;

    /**
     * Creates a new gizmo.
     *
     * @param mode            Type of a gizmo to create.
     * @param pControlledNode Node that will be transformed using the gizmo.
     */
    GizmoNode(GizmoMode mode, SpatialNode* pControlledNode);

    virtual ~GizmoNode() override = default;

    /**
     * Returns node ID of the mesh node that controls X axis of the gizmo.
     *
     * @param axis Axis.
     *
     * @return Node ID.
     */
    size_t getAxisNodeId(GizmoAxis axis);

    /**
     * Tracks mouse movement to change controlled node transform.
     *
     * @param axis Axis to enable.
     */
    void trackMouseMovement(GizmoAxis axis);

    /** Stops mouse tracking (see @ref trackMouseMovement) if it was enabled. */
    void stopTrackingMouseMovement();

    /**
     * Returns node which the gizmo controls.
     *
     * @return Node.
     */
    SpatialNode* getControlledNode() const { return pControlledNode; }

    /**
     * Returns gizmo mode.
     *
     * @return Mode.
     */
    GizmoMode getMode() const { return mode; }

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
     * Called when the window received mouse movement.
     *
     * @remark This function will not be called if @ref setIsReceivingInput was not enabled.
     * @remark This function will only be called while this node is spawned.
     *
     * @param xOffset  Mouse X movement delta in pixels (plus if moved to the right,
     * minus if moved to the left).
     * @param yOffset  Mouse Y movement delta in pixels (plus if moved up,
     * minus if moved down).
     */
    virtual void onMouseMove(double xOffset, double yOffset) override;

private:
    /** Groups info used while tracking mouse cursor movement. */
    struct TrackingInfo {
        /** Axis along which to apply changes. */
        GizmoAxis axis = GizmoAxis::X;

        /**
         * Relative (to parent node) position/rotation/scale of the controlled node when the tracking was
         * started.
         */
        glm::vec3 originalRelativeTransform = glm::vec3(0.0f);

        /** World position of the controlled node when the tracking was started. */
        glm::vec3 originalWorldPos = glm::vec3(0.0f);

        /** Offset (along @ref axis) at the moment when we started tracking mouse cursor movement. */
        float offsetToGizmoPivot = 0.0f;
    };

    /**
     * Calculates intersection between cursor direction in world space and gizmo and returns offset from gizmo
     * world location to the point where intersection occurs.
     *
     * @param gizmoOriginalLocation Location of the gizmo while tracking was started.
     * @param axis Returned offset will be along this axis.
     *
     * @return Empty if unable to calculate.
     */
    std::optional<float>
    calculateOffsetFromGizmoToCursorRay(const glm::vec3& gizmoOriginalLocation, GizmoAxis axis);

    /** Type of the gizmo. */
    const GizmoMode mode = GizmoMode::MOVE;

    /** Not empty while tracking mouse cursor movement. */
    std::optional<TrackingInfo> optTrackingInfo;

    /** Node that will be transformed using the gizmo. */
    SpatialNode* const pControlledNode = nullptr;

    /** Node that controls X axis transform. */
    MeshNode* pXAxisGizmoNode = nullptr;

    /** Node that controls Y axis transform. */
    MeshNode* pYAxisGizmoNode = nullptr;

    /** Node that controls Z axis transform. */
    MeshNode* pZAxisGizmoNode = nullptr;
};
