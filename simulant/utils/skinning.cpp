#include "skinning.h"

#include <algorithm>

#include "../logging.h"
#include "../math/aabb.h"
#include "../vertex_data.h"

#include "../deps/sh4zam/shz_matrix.h"

namespace smlt {

namespace {

/* Reads vertex `i`'s joint indices/weights from `source` and leaves XMTRX
 * holding the blended skinning matrix. Returns false (XMTRX untouched) if
 * the vertex has no weight at all, in which case callers should fall back to
 * the bind pose. */
bool load_blended_matrix(const VertexData* source, uint32_t i,
                         const Mat4* joint_matrices, uint16_t joint_count,
                         bool use_byte_joints) {
    const Vec4& weights_acc = *source->weights_at<Vec4>(i);
    float weights[4] = {weights_acc.x, weights_acc.y, weights_acc.z,
                        weights_acc.w};

    uint16_t joints[4] = {0, 0, 0, 0};
    if(use_byte_joints) {
        const auto* acc = source->joints_at<uint8_t>(i);
        for(int j = 0; j < 4; ++j) {
            joints[j] = acc[j];
        }
    } else {
        const auto* acc = source->joints_at<uint16_t>(i);
        for(int j = 0; j < 4; ++j) {
            joints[j] = acc[j];
        }
    }

    float sum = weights[0] + weights[1] + weights[2] + weights[3];

    /* Not every format weights every vertex (MS3D in particular allows
     * vertices with no bone at all). */
    if(sum == 0.0f) {
        return false;
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
        if(joint_index >= joint_count) {
            continue;
        }

        /* Missing joints are pre-zeroed (see skin_vertices doc comment), so
         * they can be blended in unconditionally with no extra branch. */
        const Mat4& joint_matrix = joint_matrices[joint_index];
        shz_xmtrx_blend((const shz_mat4x4_t*)joint_matrix._native(), weight);
    }

    return true;
}

} // namespace

void skin_vertices(const VertexData* source, const Mat4* joint_matrices,
                   uint16_t joint_count, VertexData* dest) {
    const auto& source_spec = source->vertex_specification();

    if(!source_spec.has_joints() || !source_spec.has_weights()) {
        S_WARN_ONCE("Skinned mesh has no joint/weight vertex attributes and "
                    "can't be posed");
        return;
    }

    const bool has_positions = source_spec.has_positions();
    const bool has_normals = source_spec.has_normals();
    const bool use_byte_joints =
        source_spec.joint_attribute == VERTEX_ATTRIBUTE_4UB;

    dest->move_to_start();

    for(uint32_t i = 0; i < source->count(); ++i) {
        bool blended = load_blended_matrix(source, i, joint_matrices,
                                           joint_count, use_byte_joints);

        if(!blended) {
            /* Leave unweighted vertices at their bind-pose position/normal */
            if(has_positions) {
                dest->position(*source->position_at<Vec3>(i));
            }

            if(has_normals) {
                dest->normal(*source->normal_at<Vec3>(i));
            }

            dest->move_next();
            continue;
        }

        if(has_positions) {
            const Vec3& v = *source->position_at<Vec3>(i);
            shz_vec3_t out =
                shz_xmtrx_transform_point3(shz_vec3_init(v.x, v.y, v.z));
            dest->position({out.x, out.y, out.z});
        }

        if(has_normals) {
            const Vec3& n = *source->normal_at<Vec3>(i);
            shz_vec3_t out =
                shz_xmtrx_transform_vec3(shz_vec3_init(n.x, n.y, n.z));
            dest->normal(Vec3(out.x, out.y, out.z).normalized());
        }

        dest->move_next();
    }

    dest->done();
}

AABB skinned_aabb(const VertexData* source, const Mat4* joint_matrices,
                  uint16_t joint_count) {
    const auto& source_spec = source->vertex_specification();

    if(!source_spec.has_positions() || !source_spec.has_joints() ||
       !source_spec.has_weights()) {
        return AABB();
    }

    const bool use_byte_joints =
        source_spec.joint_attribute == VERTEX_ATTRIBUTE_4UB;

    Vec3 min_v, max_v;
    bool first = true;

    for(uint32_t i = 0; i < source->count(); ++i) {
        bool blended = load_blended_matrix(source, i, joint_matrices,
                                           joint_count, use_byte_joints);

        Vec3 pos;
        if(!blended) {
            pos = *source->position_at<Vec3>(i);
        } else {
            const Vec3& v = *source->position_at<Vec3>(i);
            shz_vec3_t out =
                shz_xmtrx_transform_point3(shz_vec3_init(v.x, v.y, v.z));
            pos = Vec3(out.x, out.y, out.z);
        }

        if(first) {
            min_v = max_v = pos;
            first = false;
        } else {
            min_v = Vec3(std::min(min_v.x, pos.x), std::min(min_v.y, pos.y),
                        std::min(min_v.z, pos.z));
            max_v = Vec3(std::max(max_v.x, pos.x), std::max(max_v.y, pos.y),
                        std::max(max_v.z, pos.z));
        }
    }

    AABB result;
    if(!first) {
        result.set_min_max(min_v, max_v);
    }
    return result;
}

} // namespace smlt
