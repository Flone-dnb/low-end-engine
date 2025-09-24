#pragma once

// Custom.
#include "game/node/physics/DynamicBodyNode.h"

/** Physically simulated body that is moved by velocities. */
class KinematicBodyNode : public DynamicBodyNode {
    // Manages internal physics body.
    friend class PhysicsManager;

public:
    KinematicBodyNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    KinematicBodyNode(const std::string& sNodeName);

    virtual ~KinematicBodyNode() override;

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
     * Sets linear velocity of the body.
     * 
     * @remark Does nothing if the node is not spawned.
     * 
     * @param velocity Velocity.
     */
    void setLinearVelocity(const glm::vec3& velocity);

    /**
     * Sets angular velocity of the body.
     *
     * @remark Does nothing if the node is not spawned.
     *
     * @param velocity Velocity.
     */
    void setAngularVelocity(const glm::vec3& velocity);

    /**
     * Returns linear velocity of the body.
     *
     * @return Velocity.
     */
    glm::vec3 getLinearVelocity();

    /**
     * Returns angular velocity of the body.
     *
     * @return Velocity.
     */
    glm::vec3 getAngularVelocity();
};