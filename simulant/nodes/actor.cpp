//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "actor.h"

#include "../debug.h"
#include "../stage.h"
#include "../animation.h"
#include "../renderers/renderer.h"
#include "../assets/meshes/rig.h"
#include "../scenes/scene.h"

#define DEBUG_ANIMATION 0  /* If enabled, will show debug animation overlay */

namespace smlt {

Actor::Actor(Scene* owner, SoundDriver *sound_driver, MeshPtr mesh):
    StageNode(owner, STAGE_NODE_TYPE_ACTOR),
    AudioSource(owner, this, sound_driver) {

    set_mesh(mesh);
}

Actor::~Actor() {
    mesh_skeleton_added_.disconnect();
    submesh_created_connection_.disconnect();
    submesh_destroyed_connection_.disconnect();
}

bool Actor::has_multiple_meshes() const {
    /* Returns true if there are any meshes beyond DETAIL_LEVEL_NEAREST */

    if(meshes_[DETAIL_LEVEL_NEAR]) return true;
    if(meshes_[DETAIL_LEVEL_MID]) return true;
    if(meshes_[DETAIL_LEVEL_FAR]) return true;
    if(meshes_[DETAIL_LEVEL_FARTHEST]) return true;

    return false;
}

void Actor::set_mesh(const MeshPtr& mesh, DetailLevel detail_level) {
    /* Do nothing if we don't have a base mesh. You need at least a base mesh at all times */
    if(detail_level != DETAIL_LEVEL_NEAREST && !has_any_mesh()) {
        S_ERROR(
            "Attempted to set a mesh detail level before setting DETAIL_LEVEL_NEAREST"
        );
        return;
    }

    if(submesh_created_connection_) {
        submesh_created_connection_.disconnect();
    }

    if(submesh_destroyed_connection_) {
        submesh_destroyed_connection_.disconnect();
    }

    // Do nothing if already set
    if(meshes_[detail_level] && meshes_[detail_level] == mesh) {
        return;
    }

    if(!mesh) {
        if(detail_level == DETAIL_LEVEL_NEAREST && has_multiple_meshes()) {
            S_ERROR(
                "Attempted to clear the mesh at DETAIL_LEVEL_NEAREST while there were multiple meshes"
            );
            return;
        }

        meshes_[detail_level].reset();
        interpolated_vertex_data_.reset();
        recalc_effective_meshes();

        // FIXME: Delete vertex buffer!
        return;
    }

    //Increment the ref-count on this mesh
    meshes_[detail_level] = mesh;

    /* Only the nearest detail level is animated */
    has_animated_mesh_ = meshes_[DETAIL_LEVEL_NEAREST]->is_animated();

    /* FIXME: This logic should also happen if the associated Mesh has set_animation_enabled called */
    if(detail_level == DETAIL_LEVEL_NEAREST && has_animated_mesh_) {
        using namespace std::placeholders;

        interpolated_vertex_data_ = std::make_shared<VertexData>(meshes_[DETAIL_LEVEL_NEAREST]->vertex_data->vertex_specification());
        animation_state_ = std::make_shared<KeyFrameAnimationState>(
            meshes_[detail_level].get(),
            std::bind(&Actor::refresh_animation_state, this, _1, _2, _3)
        );

        animation_state_->play_first_animation();

        /* Make sure we update the vertex data immediately */
        refresh_animation_state(animation_state_->current_frame(), animation_state_->next_frame(), 0);
    }

    /* Update the rig if a skeleton is added to the mesh, or, if it
     * has one at the time of setting */
    if(detail_level == DETAIL_LEVEL_NEAREST) {
        mesh_skeleton_added_.disconnect();

        mesh_skeleton_added_ = meshes_[DETAIL_LEVEL_NEAREST]->signal_skeleton_added().connect(
            std::bind(&Actor::add_rig, this, std::placeholders::_1)
        );

        if(meshes_[DETAIL_LEVEL_NEAREST]->has_skeleton()) {
            add_rig(meshes_[DETAIL_LEVEL_NEAREST]->skeleton);
        } else {
            /* No skeleton on the mesh we just set, so delete the rig */
            rig_.reset();
        }
    }

    recalc_effective_meshes();

    /* Recalculate the AABB if necessary */
    mark_transformed_aabb_dirty();

    signal_mesh_changed_(id());
}

void Actor::on_update(float dt) {
    if(animation_state_) {
        animation_state_->update(dt);
    }
}

void Actor::refresh_animation_state(uint32_t current_frame, uint32_t next_frame, float interp) {
    MeshPtr base_mesh = meshes_[DETAIL_LEVEL_NEAREST];

    assert(base_mesh && base_mesh->is_animated());

#ifdef DEBUG_ANIMATION
    auto debug = find_descendent_with_name("Debug");
    if(!debug) {
        debug = scene->create_node<Debug>();
        debug->set_name("Debug");
        debug->set_parent(this);
    }

    debug->set_transform(absolute_transformation());
#endif

    base_mesh->animated_frame_data_->prepare_unpack(
        current_frame, next_frame, interp, rig_.get()
#if DEBUG_ANIMATION
        , debug
#endif
    );

#ifdef DEBUG_ANIMATION
    debug->set_transform(Mat4());
#endif
}

void Actor::add_rig(const Skeleton* skeleton) {
    assert(!rig_);

    rig_.reset(new Rig(skeleton));
}


const AABB &Actor::aabb() const {
    /*
        FIXME: Should return the superset of all mesh
        AABBs *not* only the base mesh.
     */

    static AABB aabb;

    if(has_mesh(DETAIL_LEVEL_NEAREST)) {
        return mesh(DETAIL_LEVEL_NEAREST)->aabb();
    }

    return aabb;
}

AssetID Actor::mesh_id(DetailLevel detail_level) const {
    auto& mesh = meshes_[detail_level];
    return (mesh) ? mesh->id() : AssetID(0);
}

const MeshPtr &Actor::best_mesh(DetailLevel detail_level) const {
    return find_mesh(detail_level);
}

const MeshPtr& Actor::base_mesh() const {
    return mesh(DETAIL_LEVEL_NEAREST);
}

const MeshPtr& Actor::mesh(DetailLevel detail_level) const {
    return meshes_[detail_level];
}

bool Actor::has_any_mesh() const {
    return bool(find_mesh(DETAIL_LEVEL_FARTHEST));
}

bool Actor::has_mesh(DetailLevel detail_level) const {
    /* Returns True if the Actor has a mesh at this detail level */
    return bool(meshes_[detail_level].get());
}

void Actor::do_generate_renderables(batcher::RenderQueue* render_queue, const CameraPtr& camera, const DetailLevel detail_level) {
    _S_UNUSED(camera);

    auto mesh = find_mesh(detail_level);
    if(!mesh) {
        return;
    }

    if(mesh->is_animated()) {
#ifdef DEBUG_ANIMATION
        auto debug = find_descendent_with_name("Debug");
        if(!debug) {
            debug = scene->create_node<Debug>();
            debug->set_name("Debug");
            debug->set_parent(this);
        }
#endif

        /*
         * Update the vertices for the animated base mesh if that's the
         * detail level we're using - if this is a skeletal animation then
         * the current rig will be used
         */
        mesh->animated_frame_data_->unpack_frame(
            animation_state->current_frame(),
            animation_state->next_frame(),
            animation_state->interp(),
            rig_.get(),
            interpolated_vertex_data_.get()
    #if DEBUG_ANIMATION
            , debug
    #endif
        );
    }

    auto vdata = (has_animated_mesh()) ?
        interpolated_vertex_data_.get() :
        mesh->vertex_data.get();

    for(auto submesh: mesh->each_submesh()) {
        Renderable new_renderable;
        new_renderable.final_transformation = absolute_transformation();
        new_renderable.render_priority = render_priority();
        new_renderable.is_visible = is_visible();
        new_renderable.arrangement = submesh->arrangement();
        new_renderable.vertex_data = vdata;
        new_renderable.index_data = submesh->index_data.get();
        new_renderable.index_element_count = (new_renderable.index_data) ? new_renderable.index_data->count() : 0;
        new_renderable.vertex_ranges = submesh->vertex_ranges();
        new_renderable.vertex_range_count = submesh->vertex_range_count();
        new_renderable.material = submesh->material_at_slot(material_slot_, true).get();
        new_renderable.centre = transformed_aabb().centre();

        render_queue->insert_renderable(std::move(new_renderable));
    }
}

void Actor::recalc_effective_meshes() {
    MeshPtr current = meshes_[0];
    for(auto i = 0; i < DETAIL_LEVEL_MAX; ++i) {
        effective_meshes_[i] = current;

        if(i < DETAIL_LEVEL_MAX - 1) {
            if(meshes_[i + 1]) {
                current = meshes_[i + 1];
            }
        }
    }
}

}
