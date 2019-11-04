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

#include "../stage.h"
#include "../animation.h"
#include "../renderers/renderer.h"
#include "../renderers/batching/renderable_store.h"

namespace smlt {

Actor::Actor(ActorID id, Stage* stage, SoundDriver* sound_driver):
    TypedDestroyableObject<Actor, Stage>(stage),
    StageNode(stage),
    generic::Identifiable<ActorID>(id),
    Source(stage, sound_driver) {

}

Actor::Actor(ActorID id, Stage* stage, SoundDriver *sound_driver, MeshID mesh):
    TypedDestroyableObject<Actor, Stage>(stage),
    StageNode(stage),
    generic::Identifiable<ActorID>(id),
    Source(stage, sound_driver) {

    set_mesh(mesh);
}

Actor::~Actor() {

}

VertexSpecification SubActor::vertex_specification() const {
    auto* vertex_data = get_vertex_data();
    if(vertex_data) {
        return vertex_data->vertex_specification();
    }

    return VertexSpecification();
}

VertexData* SubActor::vertex_data() const {
    if(parent_.has_animated_mesh()) {
        return parent_.interpolated_vertex_data_.get();
    }

    return submesh_->vertex_data.get();
}

IndexData* SubActor::index_data() const {
    return submesh_->index_data.get();
}

std::size_t SubActor::index_element_count() const {
    return submesh_->index_data->count();
}

IndexType SubActor::index_type() const {
    return submesh_->index_data->index_type();
}

bool Actor::has_multiple_meshes() const {
    /* Returns true if there are any meshes beyond DETAIL_LEVEL_NEAREST */

    if(meshes_[DETAIL_LEVEL_NEAR]) return true;
    if(meshes_[DETAIL_LEVEL_MID]) return true;
    if(meshes_[DETAIL_LEVEL_FAR]) return true;
    if(meshes_[DETAIL_LEVEL_FARTHEST]) return true;

    return false;
}

void Actor::set_mesh(MeshID mesh, DetailLevel detail_level) {
    /* Do nothing if we don't have a base mesh. You need at least a base mesh at all times */
    if(detail_level != DETAIL_LEVEL_NEAREST && !has_any_mesh()) {
        L_ERROR(
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
    if(meshes_[detail_level] && meshes_[detail_level]->id() == mesh) {
        return;
    }

    if(!mesh) {
        if(detail_level == DETAIL_LEVEL_NEAREST && has_multiple_meshes()) {
            L_ERROR(
                "Attempted to clear the mesh at DETAIL_LEVEL_NEAREST while there were multiple meshes"
            );
            return;
        }

        meshes_[detail_level].reset();
        interpolated_vertex_data_.reset();

        // FIXME: Delete vertex buffer!
        return;
    }

    auto meshptr = stage->assets->mesh(mesh);

    if(!meshptr) {
        L_ERROR(_F("Unable to locate mesh with the ID: {0}").format(mesh));
        return;
    }

    //Increment the ref-count on this mesh
    meshes_[detail_level] = meshptr;
    meshptr.reset();

    /* Only the nearest detail level is animated */
    /* FIXME: This logic should also happen if the associated Mesh has set_animation_enabled called */
    if(detail_level == DETAIL_LEVEL_NEAREST && has_animated_mesh(detail_level)) {
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

    /* Recalculate the AABB if necessary */
    mark_transformed_aabb_dirty();

    signal_mesh_changed_(id());
}

void Actor::update(float dt) {
    StageNode::update(dt);

    update_source(dt);
    if(animation_state_) {
        animation_state_->update(dt);
    }
}

void Actor::refresh_animation_state(uint32_t current_frame, uint32_t next_frame, float interp) {
    assert(meshes_[DETAIL_LEVEL_NEAREST] && meshes_[DETAIL_LEVEL_NEAREST]->is_animated());

    meshes_[DETAIL_LEVEL_NEAREST]->animated_frame_data_->unpack_frame(
        current_frame, next_frame, interp, interpolated_vertex_data_.get()
    );
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

MeshID Actor::mesh_id(DetailLevel detail_level) const {
    auto& mesh = meshes_[detail_level];
    return (mesh) ? mesh->id() : MeshID(0);
}

SubActor::SubActor(const Actor &parent, std::shared_ptr<SubMesh> submesh):
    parent_(parent),
    submesh_(submesh) {

    material_ = submesh->material();
}

SubActor::~SubActor() {

}

const MaterialID SubActor::material_id() const {
    if(material_override_) {
        return material_override_->id();
    }

    /* Return the submeshes material */
    return material_->id();
}

void SubActor::override_material_id(MaterialID material) {
    if(material == material_override_->id()) {
        return;
    }

    if(material) {
        //Store the pointer to maintain the ref-count
        material_override_ = parent_.stage->assets->material(material);
    } else {
        // If we passed a zero material ID, then remove the
        // material pointer
        material_override_.reset();
    }
}

void SubActor::remove_material_id_override() {
    override_material_id(MaterialID());
}

MeshPtr Actor::best_mesh(DetailLevel detail_level) const {
    return find_mesh(detail_level);
}

MeshPtr Actor::base_mesh() const {
    return mesh(DETAIL_LEVEL_NEAREST);
}

MeshPtr Actor::mesh(DetailLevel detail_level) const {
    return meshes_[detail_level];
}

bool Actor::has_any_mesh() const {
    return bool(find_mesh(DETAIL_LEVEL_FARTHEST));
}

bool Actor::has_mesh(DetailLevel detail_level) const {
    /* Returns True if the Actor has a mesh at this detail level */
    return bool(meshes_[detail_level]);
}

SubMesh* SubActor::submesh() {
    if(!submesh_) {
        throw std::logic_error("Submesh was not initialized");
    }

    return submesh_.get();
}

const SubMesh *SubActor::submesh() const {
    if(!submesh_) {
        throw std::logic_error("Submesh was not initialized");
    }

    return submesh_.get();
}

void Actor::_get_renderables(RenderableFactory* factory, CameraPtr camera, DetailLevel level) {
    auto mesh = find_mesh(level);
    if(!mesh) {
        return;
    }

    mesh->each([&](const std::string& name, SubMesh* submesh) {
        /*
         * It's important that subactors hold a reference to their counterpart
         * submesh. Otherwise if a thread destroys a submesh while rendering is happening
         * we get random crashes. We called shared_from_this to keep the submesh around
         */
        Renderable new_renderable;
        new_renderable.final_transformation = absolute_transformation();
        new_renderable.render_priority = render_priority();
        new_renderable.is_visible = is_visible();

        auto vdata = (has_animated_mesh()) ? interpolated_vertex_data_.get() : mesh->vertex_data.get();

        new_renderable.arrangement = submesh->arrangement();
        new_renderable.vertex_data = vdata;
        new_renderable.index_data = submesh->index_data.get();
        new_renderable.index_element_count = new_renderable.index_data->count();
        new_renderable.material_id = submesh->material_at_slot(material_slot_, true);
        new_renderable.centre = transformed_aabb().centre();

        factory->push_renderable(new_renderable);
    });
}


VertexData* SubActor::get_vertex_data() const {
    auto sm = submesh();
    assert(sm);

    auto ret = sm->vertex_data.get();
    assert(ret);
    return ret;
}

IndexData* SubActor::get_index_data() const {
    auto sm = submesh();
    assert(sm);

    auto ret = sm->index_data.get();
    assert(ret);
    return ret;
}

}
