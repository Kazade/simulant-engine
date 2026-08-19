//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published
//     by the Free Software Foundation, either version 3 of the License, or (at
//     your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <unordered_set>

#include "armature.h"
#include "joint.h"

#include "../asset_manager.h"
#include "../assets/material.h"
#include "../deps/sh4zam/shz_matrix.h"
#include "../renderers/batching/render_queue.h"
#include "../scenes/scene.h"
#include "../utils/skinning.h"

namespace smlt {

bool Armature::on_create(Params params) {
    /* Any mesh beyond the first is passed as "mesh.1", "mesh.2"... and has
     * to be picked up before clean_params drops everything undeclared */
    std::vector<MeshPtr> extras;
    const std::string prefix = extra_mesh_param_prefix();
    for(std::size_t i = 1;; ++i) {
        auto name = prefix + std::to_string(i);
        auto maybe = params.get<MeshRef>(name.c_str());
        if(!maybe) {
            break;
        }

        extras.push_back(maybe.value().lock());
    }

    if(!clean_params<Armature>(params)) {
        return false;
    }

    auto mesh = params.get<MeshRef>("mesh").value_or(MeshPtr()).lock();
    if(mesh) {
        add_mesh(mesh);
    }

    for(auto& extra: extras) {
        add_mesh(extra);
    }

    return StageNode::on_create(params);
}

bool Armature::add_mesh(const MeshPtr& source) {
    if(!source) {
        return false;
    }

    if(!source->is_skinned()) {
        S_WARN("Mesh '{0}' has no skin, so can't be posed by an armature",
               source->name());
        return false;
    }

    meshes_.push_back(SkinnedMesh{source});

    mark_skinning_dirty();
    rebuild_aabb();

    return true;
}

MeshPtr Armature::source_mesh(std::size_t i) const {
    return (i < meshes_.size()) ? meshes_[i].source : MeshPtr();
}

bool Armature::read_back_pose(std::size_t i, VertexData& out) const {
    if(i >= meshes_.size()) {
        return false;
    }

    auto& entry = meshes_[i];
    auto& skin = entry.source->skin;
    if(!skin) {
        return false;
    }

    auto it = skin_poses_.find(skin.get());
    if(it == skin_poses_.end()) {
        return false;
    }

    auto source_data = entry.source->vertex_data.get();
    out.reset(source_data->vertex_specification());
    out.resize(source_data->count());

    skin_vertices(source_data, it->second.info.joint_matrices,
                 it->second.info.joint_count, &out);

    return true;
}

Joint* Armature::joint(std::size_t index) const {
    ensure_joints();
    return (index < joints_.size()) ? joints_[index] : nullptr;
}

Joint* Armature::find_joint(const std::string& name) const {
    ensure_joints();

    for(auto joint: joints_) {
        if(joint && joint->name() == name) {
            return joint;
        }
    }

    return nullptr;
}

void Armature::ensure_joints() const {
    if(!joints_dirty_) {
        return;
    }

    joints_.clear();

    for(auto& node: each_descendent()) {
        if(node.node_type() != Joint::Meta::node_type) {
            continue;
        }

        auto joint = static_cast<Joint*>(&node);

        /* Armatures can be nested - don't steal a joint that belongs to one
         * of ours */
        if(joint->armature() != this) {
            continue;
        }

        auto index = joint->joint_index();
        if(index < 0) {
            S_WARN("Joint '{0}' has no index and will not be posed",
                   joint->name());
            continue;
        }

        if(joints_.size() <= (std::size_t)index) {
            joints_.resize((std::size_t)index + 1, nullptr);
        }

        joints_[index] = joint;
    }

    joints_dirty_ = false;
}

void Armature::update_joint_matrices(const Mat4& armature_world_inverse,
                                     const Mesh::Skin& skin) {
    auto& pose = skin_poses_[&skin];

    const std::size_t count =
        std::min(joints_.size(), skin.inverse_bind_matrices.size());

    /* Missing joints (a declared index with no matching Joint node) are left
     * as the zero matrix, so skin_vertices() can blend them in
     * unconditionally without needing to know which joints exist. */
    pose.joint_matrices.assign(count, Mat4::zero());

    /* Chain the three matrix multiplies through XMTRX directly, saving one
     * full XMTRX load+store round-trip per joint compared to calling
     * shz_mat4x4_mult twice. */
    for(std::size_t h = 0; h < count; ++h) {
        auto joint = joints_[h];
        if(!joint) {
            continue;
        }

        const Mat4& joint_matrix = joint->transform->world_space_matrix();

        // XMTRX = armature_world_inverse * joint_matrix
        shz_xmtrx_load_apply_4x4(
            (shz_mat4x4_t*)armature_world_inverse._native(),
            (shz_mat4x4_t*)joint_matrix._native());
        // XMTRX = (armature_world_inverse * joint_matrix) * inverse_bind[h]
        shz_xmtrx_apply_store_4x4(
            (shz_mat4x4_t*)pose.joint_matrices[h]._native(),
            (shz_mat4x4_t*)skin.inverse_bind_matrices[h]._native());
    }

    pose.info.joint_matrices = pose.joint_matrices.data();
    pose.info.joint_count = (uint16_t)count;
    ++pose.info.generation;
}

void Armature::update_skinning() {
    skinning_dirty_ = false;

    ensure_joints();

    if(meshes_.empty() || joints_.empty()) {
        return;
    }

    /* Use the block-triangular inverse - it's faster than the general
     * inverse and is always correct for TRS world matrices. */
    const Mat4 armature_world_inverse =
        transform->world_space_matrix().inversed_transform();

    /* Meshes bound to the same skeleton share a Skin (and therefore a set of
     * joint matrices) - glTF exporters commonly emit one skin per mesh part
     * over a shared skeleton - so only recompute each distinct skin once. */
    std::unordered_set<const Mesh::Skin*> processed;

    for(auto& entry: meshes_) {
        auto& skin = entry.source->skin;
        if(!skin) {
            continue;
        }

        if(processed.insert(skin.get()).second) {
            update_joint_matrices(armature_world_inverse, *skin);
        }
    }

    rebuild_aabb();
}

void Armature::rebuild_aabb() {
    bool first = true;

    for(auto& entry: meshes_) {
        auto& skin = entry.source->skin;
        if(!skin) {
            continue;
        }

        auto it = skin_poses_.find(skin.get());
        if(it == skin_poses_.end()) {
            continue;
        }

        AABB mesh_aabb =
            skinned_aabb(entry.source->vertex_data.get(),
                        it->second.info.joint_matrices,
                        it->second.info.joint_count);

        if(first) {
            aabb_ = mesh_aabb;
            first = false;
        } else {
            aabb_.encapsulate(mesh_aabb.min());
            aabb_.encapsulate(mesh_aabb.max());
        }
    }

    if(first) {
        aabb_ = AABB();
    }

    mark_transformed_aabb_dirty();
}

const AABB& Armature::aabb() const {
    return aabb_;
}

void Armature::on_late_update(float dt) {
    _S_UNUSED(dt);

    /* An armature that isn't being animated still needs posing once, and a
     * hand-posed skeleton needs re-posing whenever it's moved. Anything
     * driving the armature every frame (see AnimationController) will have
     * cleared this already. Doing it here rather than at render time keeps
     * the AABB used for culling in step with the pose. */
    if(skinning_dirty_) {
        update_skinning();
    }
}

void Armature::do_generate_renderables(batcher::RenderQueue* render_queue,
                                       const Camera* camera, const Viewport*,
                                       const DetailLevel detail_level,
                                       Light** lights,
                                       const std::size_t light_count,
                                       bool respect_visibility) {
    _S_UNUSED(camera);
    _S_UNUSED(detail_level);

    if(respect_visibility && !is_visible()) {
        return;
    }

    /* Catches joints posed after late_update ran (e.g. by a mixin further
     * down the tree) - otherwise we'd render a frame-old pose */
    if(skinning_dirty_) {
        update_skinning();
    }

    auto rp = render_priority();
    const Mat4* mat = &transform->world_space_matrix();
    auto center = transformed_aabb().center();
    const bool visible = is_visible();

    for(auto& entry: meshes_) {
        auto& skin = entry.source->skin;

        const SkinningInfo* skinning_info = nullptr;
        if(skin) {
            auto it = skin_poses_.find(skin.get());
            if(it != skin_poses_.end()) {
                skinning_info = &it->second.info;
            }
        }

        int i = entry.source->submesh_count();

        for(auto& submesh: entry.source->each_submesh()) {
            Renderable new_renderable;
            new_renderable.final_transformation = mat;
            new_renderable.render_priority = rp;
            new_renderable.is_visible = visible;
            new_renderable.arrangement = submesh->arrangement();

            /* The unposed, shared bind-pose vertex data (with joint/weight
             * attributes) plus the current joint matrices - actually
             * skinning this into something renderable is deferred to the
             * renderer, immediately before submission. */
            new_renderable.vertex_data = entry.source->vertex_data.get();
            new_renderable.skinning = skinning_info;

            new_renderable.index_data = submesh->index_data.get();
            new_renderable.index_element_count =
                (new_renderable.index_data)
                    ? new_renderable.index_data->count()
                    : 0;
            new_renderable.vertex_ranges = submesh->vertex_ranges();
            new_renderable.vertex_range_count = submesh->vertex_range_count();
            new_renderable.material =
                submesh->material_at_slot(material_slot_, true).get();
            new_renderable.center = center;

            /* Indexed submeshes have fixed connectivity, so expose a stable
             * key (the index data's uuid) to allow derived data to be
             * cached. This holds even while posing: the topology is
             * invariant under deformation, and skinning only changes
             * position/normal. */
            new_renderable.key = (new_renderable.index_data)
                                     ? (int64_t)new_renderable.index_data->uuid()
                                     : -1;

            if(shadow_receive() == SHADOW_RECEIVE_ALWAYS) {
                new_renderable.flags |= RENDERABLE_FLAG_RECEIVES_SHADOWS;
            }

            new_renderable.light_count = light_count;
            for(auto l = 0u; l < light_count; ++l) {
                new_renderable.lights_affecting_this_frame[l] = lights[l];
            }

            new_renderable.precedence = float(precedence()) + ((--i) * 0.0001f);

            render_queue->insert_renderable(std::move(new_renderable));
        }
    }
}

} // namespace smlt
