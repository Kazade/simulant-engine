#include "mesh_instancer.h"
#include "../stage.h"
#include "../meshes/mesh.h"

namespace smlt {

uint32_t MeshInstancer::id_counter_ = 0;

MeshInstancer::MeshInstancer(Stage *stage, SoundDriver *sound_driver, MeshPtr mesh):
    TypedDestroyableObject<MeshInstancer, Stage>(stage),
    StageNode(stage, STAGE_NODE_TYPE_MESH_INSTANCER),
    AudioSource(stage, this, sound_driver) {

    set_mesh(mesh);
}

MeshInstancer::~MeshInstancer() {

}

const AABB &MeshInstancer::aabb() const {
    return aabb_;
}

void MeshInstancer::set_mesh(MeshPtr mesh) {
    mesh_ = mesh;

    /* Recalc the local AABBs for the new mesh */
    for(auto& i: instances_) {
        i.second.recalc_aabb(mesh_);
    }

    /* recalc as a whole */
    recalc_aabb();
}

MeshPtr MeshInstancer::mesh() const {
    return mesh_;
}

MeshInstanceID MeshInstancer::new_mesh_instance(const Vec3 &position, const Quaternion &rotation) {
    MeshInstance i;
    i.id = ++MeshInstancer::id_counter_;
    i.transformation = Mat4::from_pos_rot_scale(position, rotation, Vec3(1));
    i.abs_transformation = absolute_transformation() * i.transformation;
    i.recalc_aabb(mesh_);

    instances_.insert(std::make_pair(i.id, i));

    /* Recalculate the aabb for the instancer as a whole */
    recalc_aabb();

    return i.id;
}

bool MeshInstancer::destroy_mesh_instance(MeshInstanceID mid) {
    auto it = instances_.find(mid);
    if(it != instances_.end()) {
        instances_.erase(it);
        recalc_aabb();

        return true;
    }

    return false;
}

bool MeshInstancer::show_mesh_instance(MeshInstanceID mid) {
    auto it = instances_.find(mid);
    if(it != instances_.end()) {
        it->second.is_visible = true;
        return true;
    }

    return false;
}

bool MeshInstancer::hide_mesh_instance(MeshInstanceID mid) {
    auto it = instances_.find(mid);
    if(it != instances_.end()) {
        it->second.is_visible = false;
        return true;
    }

    return false;
}

void MeshInstancer::recalc_aabb() {
    AABB new_aabb;

    if(mesh_) {
        for(auto& mi_pair: instances_) {
            new_aabb.encapsulate(mi_pair.second.aabb);
        }
    }

    std::swap(aabb_, new_aabb);
}

void MeshInstancer::on_transformation_changed() {
    StageNode::on_transformation_changed();

    /* When the transformation changes, we need
         * to update all instances */

    for(auto& instance: instances_) {
        instance.second.abs_transformation = (
            absolute_transformation() * instance.second.transformation
        );
    }
}

void MeshInstancer::_get_renderables(
        batcher::RenderQueue* render_queue,
        const CameraPtr camera,
        const DetailLevel detail_level) {

    /* No instances or mesh, no renderables */
    if(instances_.empty() || !mesh_) {
        return;
    }

    _S_UNUSED(camera);
    _S_UNUSED(detail_level);  // FIXME: Support detail levels like actors?

    for(auto submesh: mesh_->each_submesh()) {
        Renderable new_renderable;

        new_renderable.render_priority = render_priority();
        new_renderable.arrangement = submesh->arrangement();
        new_renderable.vertex_data = mesh_->vertex_data.get();
        new_renderable.index_data = submesh->index_data.get();
        new_renderable.index_element_count = (new_renderable.index_data) ? new_renderable.index_data->count() : 0;
        new_renderable.vertex_ranges = submesh->vertex_ranges();
        new_renderable.vertex_range_count = submesh->vertex_range_count();

        // FIXME: Support material slots like actors?
        new_renderable.material = submesh->material_at_slot(MATERIAL_SLOT0, true).get();

        for(auto& mesh_instance: instances_) {
            auto to_insert = new_renderable;  // Create a copy
            to_insert.final_transformation = mesh_instance.second.abs_transformation;
            to_insert.is_visible = mesh_instance.second.is_visible;
            to_insert.centre = mesh_instance.second.aabb.centre();
            render_queue->insert_renderable(std::move(to_insert));
        }
    }
}

void MeshInstancer::MeshInstance::recalc_aabb(smlt::MeshPtr mesh) {    
    /* Calculate the AABB for this instance for performance later */
    if(!mesh) {
        aabb = AABB();
        return;
    }

    auto corners = mesh->aabb().corners();
    auto transform = transformation;

    for(auto& corner: corners) {
        corner = corner.transformed_by(transform);
    }

    aabb = AABB(corners.data(), corners.size());
}

}

