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

#include <cstring>

#include "armature.h"
#include "joint.h"

#include "../asset_manager.h"
#include "../assets/material.h"
#include "../deps/sh4zam/shz_matrix.h"
#include "../renderers/batching/render_queue.h"
#include "../scenes/scene.h"

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

MeshPtr Armature::add_mesh(const MeshPtr& source) {
    if(!source) {
        return MeshPtr();
    }

    if(!source->is_skinned()) {
        S_WARN("Mesh '{0}' has no skin, so can't be posed by an armature",
               source->name());
        return MeshPtr();
    }

    auto output = build_output_mesh(source);
    if(!output) {
        return MeshPtr();
    }

    meshes_.push_back(SkinnedMesh{source, output});

    mark_skinning_dirty();
    rebuild_aabb();

    return output;
}

MeshPtr Armature::source_mesh(std::size_t i) const {
    return (i < meshes_.size()) ? meshes_[i].source : MeshPtr();
}

MeshPtr Armature::skinned_mesh(std::size_t i) const {
    return (i < meshes_.size()) ? meshes_[i].output : MeshPtr();
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

MeshPtr Armature::build_output_mesh(const MeshPtr& source) {
    auto source_data = source->vertex_data.get();

    VertexSpecification render_spec = source_data->vertex_specification();
    render_spec.joint_attribute = VERTEX_ATTRIBUTE_NONE;
    render_spec.weight_attribute = VERTEX_ATTRIBUTE_NONE;

    auto output = scene->assets->create_mesh(render_spec);
    if(!output) {
        return MeshPtr();
    }

    output->set_name(source->name());

    const uint32_t count = source_data->count();
    output->vertex_data->resize(count);

    /* Position/normal/uv/color/specular all come before joints/weights in
     * the layout (see VertexSpecification::recalc_stride_and_offsets), so
     * their offsets are identical between the two specs and we can copy
     * each vertex's shared prefix directly rather than walking it
     * attribute-by-attribute. Everything except position and normal is then
     * left untouched by posing. */
    const uint32_t copy_bytes =
        std::min(source_data->stride(), output->vertex_data->stride());
    for(uint32_t i = 0; i < count; ++i) {
        shz_memcpy(output->vertex_data->data() +
                        (std::size_t)i * output->vertex_data->stride(),
                   source_data->data() + (std::size_t)i * source_data->stride(),
                   copy_bytes);
    }
    output->vertex_data->done();

    for(auto& sm: source->each_submesh()) {
        SubMesh* new_sm = nullptr;
        if(sm->type() == SUBMESH_TYPE_INDEXED) {
            /* index_data_ is private, but Armature is a friend of Mesh (and
             * Mesh a friend of SubMesh) - grab the shared_ptr directly
             * rather than via the ::index_data property, which unwraps to
             * the pointee type rather than the shared_ptr itself. Topology
             * is invariant under deformation, so it can be shared. */
            new_sm = output->create_submesh(sm->name(), sm->material(),
                                            sm->index_data_, sm->arrangement());
        } else {
            new_sm = output->create_submesh(sm->name(), sm->material(),
                                            sm->arrangement());
            for(std::size_t r = 0; r < sm->vertex_range_count(); ++r) {
                new_sm->add_vertex_range(sm->vertex_ranges()[r].start,
                                         sm->vertex_ranges()[r].count);
            }
        }

        for(uint8_t slot = MATERIAL_SLOT1; slot < MATERIAL_SLOT_MAX; ++slot) {
            auto mat = sm->material_at_slot((MaterialSlot)slot, false);
            if(mat) {
                new_sm->set_material_at_slot((MaterialSlot)slot, mat);
            }
        }
    }

    return output;
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
    const std::size_t count =
        std::min(joints_.size(), skin.inverse_bind_matrices.size());

    joint_matrices_.clear();
    joint_matrices_.resize(count, Mat4());

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
            (shz_mat4x4_t*)joint_matrices_[h]._native(),
            (shz_mat4x4_t*)skin.inverse_bind_matrices[h]._native());
    }
}

void Armature::pose_mesh(const SkinnedMesh& entry) {
    auto source_data = entry.source->vertex_data.get();
    auto output_data = entry.output->vertex_data.get();

    const auto& source_spec = source_data->vertex_specification();

    if(!source_spec.has_joints() || !source_spec.has_weights()) {
        /* Bound to a skeleton, but with nothing saying how. The output mesh
         * already holds a copy of the bind pose, so leave it at that. */
        S_WARN_ONCE("Skinned mesh '{0}' has no joint/weight vertex "
                    "attributes and can't be posed",
                    entry.source->name());
        return;
    }

    const bool has_positions = source_spec.has_positions();
    const bool has_normals = source_spec.has_normals();
    const bool use_byte_joints = source_spec.joint_attribute == VERTEX_ATTRIBUTE_4UB;

    output_data->move_to_start();

    for(uint32_t i = 0; i < source_data->count(); ++i) {
        const Vec4& weights_acc = *source_data->weights_at<Vec4>(i);
        float weights[4] = {weights_acc.x, weights_acc.y, weights_acc.z,
                            weights_acc.w};

        uint16_t joints[4] = {0, 0, 0, 0};
        if(use_byte_joints) {
            const auto* acc = source_data->joints_at<uint8_t>(i);
            for(int j = 0; j < 4; ++j) {
                joints[j] = acc[j];
            }
        } else {
            const auto* acc = source_data->joints_at<uint16_t>(i);
            for(int j = 0; j < 4; ++j) {
                joints[j] = acc[j];
            }
        }

        float sum = weights[0] + weights[1] + weights[2] + weights[3];

        /* Not every format weights every vertex (MS3D in particular allows
         * vertices with no bone at all). Blending zero joint matrices would
         * collapse those vertices onto the origin, so leave them at their
         * bind-pose position/normal instead. */
        if(sum == 0.0f) {
            if(has_positions) {
                output_data->position(*source_data->position_at<Vec3>(i));
            }

            if(has_normals) {
                output_data->normal(*source_data->normal_at<Vec3>(i));
            }

            output_data->move_next();
            continue;
        }

        if(sum > 1.01f) {
            float inv_sum = shz_invf_fsrra(sum);
            for(float& weight: weights) {
                weight *= inv_sum;
            }
        }

        shz_xmtrx_init_zero();
        for(int j = 0; j < 4; ++j) {
            float weight = weights[j];
            if(weight == 0.0f) {
                continue;
            }

            auto joint_index = joints[j];
            if(joint_index >= joint_matrices_.size() || !joints_[joint_index]) {
                continue;
            }

            const Mat4& joint_matrix = joint_matrices_[joint_index];
            shz_xmtrx_blend((const shz_mat4x4_t*)joint_matrix._native(), weight);
        }

        if(has_positions) {
            const Vec3& v = *source_data->position_at<Vec3>(i);
            shz_vec3_t out = shz_xmtrx_transform_point3(shz_vec3_init(v.x, v.y, v.z));
            output_data->position({out.x, out.y, out.z});
        }

        if(has_normals) {
            const Vec3& n = *source_data->normal_at<Vec3>(i);
            shz_vec3_t out = shz_xmtrx_transform_vec3(shz_vec3_init(n.x, n.y, n.z));
            output_data->normal(Vec3(out.x, out.y, out.z).normalized());
        }

        output_data->move_next();
    }

    /* This also refreshes the output mesh's AABB */
    output_data->done();
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

    /* Meshes bound to the same skeleton share a Skin (and therefore a set
     * of joint matrices), so only recompute when it actually changes */
    const Mesh::Skin* current_skin = nullptr;

    for(auto& entry: meshes_) {
        auto& skin = entry.source->skin;
        if(!skin) {
            continue;
        }

        if(skin.get() != current_skin) {
            current_skin = skin.get();
            update_joint_matrices(armature_world_inverse, *skin);
        }

        pose_mesh(entry);
    }

    rebuild_aabb();
}

void Armature::rebuild_aabb() {
    if(meshes_.empty()) {
        aabb_ = AABB();
        mark_transformed_aabb_dirty();
        return;
    }

    aabb_ = meshes_[0].output->aabb();
    for(std::size_t i = 1; i < meshes_.size(); ++i) {
        auto& other = meshes_[i].output->aabb();
        aabb_.encapsulate(other.min());
        aabb_.encapsulate(other.max());
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
        int i = entry.output->submesh_count();

        for(auto& submesh: entry.output->each_submesh()) {
            Renderable new_renderable;
            new_renderable.final_transformation = mat;
            new_renderable.render_priority = rp;
            new_renderable.is_visible = visible;
            new_renderable.arrangement = submesh->arrangement();
            new_renderable.vertex_data = entry.output->vertex_data.get();
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
             * invariant under deformation, and only the per-frame normals
             * need refreshing. */
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
