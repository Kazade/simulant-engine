#pragma once

#include <unordered_map>

#include "../generic/identifiable.h"

#include "stage_node.h"
#include "../interfaces.h"
#include "../sound.h"
#include "../generic/manual_object.h"
#include "../generic/containers/contiguous_map.h"

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
    public TypedDestroyableObject<MeshInstancer, Stage>,
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

    void _get_renderables(
        batcher::RenderQueue* render_queue,
        const CameraPtr camera,
        const DetailLevel detail_level
    ) override;

private:
    UniqueIDKey make_key() const override {
        return make_unique_id_key(id());
    }

    MeshPtr mesh_;

    /* The axis-aligned box containing all mesh instances */
    AABB aabb_;

    void recalc_aabb();    

    void on_transformation_changed() override;

    struct MeshInstance {
        uint32_t id = 0;
        bool is_visible = true;
        Mat4 transformation;
        Mat4 abs_transformation;
        AABB aabb;

        /* Recalc the aabb from the transformation */
        void recalc_aabb(MeshPtr mesh);
    };

    static uint32_t id_counter_;

    /* FIXME: Convert to ContiguousMap when it has erase... */
    std::unordered_map<uint32_t, MeshInstance> instances_;
};


}
