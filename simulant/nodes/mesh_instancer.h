#pragma once

#include "stage_node.h"
#include "../interfaces.h"
#include "../sound.h"
#include "../generic/manual_object.h"

namespace smlt {

typedef std::size_t MeshInstanceID;

/**
 * @brief The MeshInstancer class
 *
 * A MeshInstancer is the most basic way to include a mesh in a scene. It's
 * designed as a lightweight way to create multiple instances of the same mesh
 * in different locations.
 *
 * Instantiated meshes do not form part of the stage
 * hierarchy, they are not a StageNode, and are not directly accessible
 * but they do behave as if they are a child of the MeshInstancer in that
 * they are rotated and transformed relative to the MeshInstancer.
 *
 * You can swap the Mesh associated with a MeshInstancer, and all instances
 * will change immediately. You can also toggle the visibility of an individual
 * instance, or remove an instance.
 *
 * The bounds of a MeshInstancer are the sum bounds of all its instances.
 *
 * Spawning animated meshes is currently unsupported.
 */
class MeshInstancer:
    public TypedDestroyableObject<Actor, Stage>,
    public StageNode,
    public virtual Boundable,
    public generic::Identifiable<MeshInstancerID>,
    public AudioSource,
    public HasMutableRenderPriority,
    public ChainNameable<MeshInstancer> {

public:
    MeshInstancer(Stage* stage, SoundDriver* sound_driver, MeshPtr mesh);
    virtual ~MeshInstancer();

    const AABB& aabb() const override;

    void set_mesh(MeshPtr mesh);
    MeshPtr mesh() const;

    /**
     *  Create a new mesh_instance at the specified location.
     *  Returns a new non-zero MeshInstanceID on success. Returns
     *  0 on failure.
     */
    MeshInstanceID new_mesh_instance(
        const smlt::Vec3& position,
        const smlt::Quaternion& rotation=smlt::Quaternion()
    );

    /**
     * @brief destroy_mesh_instance
     * @param mid The MeshInstanceID to destroy
     * @return true if the instance existed and was destroyed,
     * false otherwise
     */
    bool destroy_mesh_instance(MeshInstanceID mid);

    /**
     * @brief show_mesh_instance
     * @param mid - The MeshInstanceID to show
     * @return true if the instance existed, false otherwise
     */
    bool show_mesh_instance(MeshInstanceID mid);

    /**
     * @brief hide_mesh_instance
     * @param mid - The MeshInstanceID to hide
     * @return true if the instance existed, false otherwise
     */
    bool hide_mesh_instance(MeshInstanceID mid);

};


}
