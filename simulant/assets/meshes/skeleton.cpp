#include <cstring>

#include "skeleton.h"
#include "rig.h"

#include "../../debug.h"

namespace smlt {

void Joint::rotate_to(const Quaternion& q) {
    bool recalc = rotation_ != q;
    rotation_ = q;
    if(recalc) {
        recalc_absolute_transformation();
    }
}

void Joint::move_to(const Vec3& v) {
    bool recalc = translation_ != v;
    translation_ = v;
    if(recalc) {
        recalc_absolute_transformation();
    }
}

std::string Joint::name() const {
    return std::string(name_);
}

void Joint::set_name(const std::string& name) {
    strncpy(name_, name.c_str(), 32);
    assert(name.length() < 32);
    name_[name.length()] = '\0';
}

Bone* Joint::link_to(Joint* other) {
    /* See if we're already linked first */
    for(auto& bone: skeleton_->bones_) {
        if(bone.joints[0] == this && bone.joints[1] == other) {
            return &bone;
        } else if(bone.joints[0] == other && bone.joints[1] == this) {
            return &bone;
        } else {
            continue;
        }
    }

    if(other->parent()) {
        S_ERROR("Attempted to change the parent of a joint");
        return nullptr;
    }

    Bone* new_bone = &skeleton_->bones_[skeleton_->bone_count_++];
    new_bone->joints[0] = this;
    new_bone->joints[1] = other;

    // Set the parent
    other->parent_ = this;
    other->recalc_absolute_transformation();

    return new_bone;
}

Joint* Joint::parent() const {
    return parent_;
}

std::size_t Joint::id() const {
    return id_;
}

void Joint::recalc_absolute_transformation() {
    if(parent_) {
        absolute_rotation_ = parent_->absolute_rotation_ * rotation_;
        absolute_translation_ = (
            parent_->absolute_translation_ +
            (parent_->absolute_rotation_ * translation_)
        );
    } else {
        absolute_rotation_ = rotation_;
        absolute_translation_ = translation_;
    }
}

SkeletalFrameUnpacker::SkeletalFrameUnpacker(Mesh* mesh, std::size_t num_frames, std::size_t num_vertices):
    mesh_(mesh) {

    assert(mesh->skeleton.get());

    /* Allocate the memory for all the frames */
    skeleton_frames_.resize(num_frames);
    for(auto& frame: skeleton_frames_) {
        frame.joints.resize(mesh->skeleton->joint_count());
    }

    vertices_.resize(num_vertices);
}

void SkeletalFrameUnpacker::prepare_unpack(uint32_t current_frame, uint32_t next_frame, float t, Rig* const rig, Debug* const debug) {
    _S_UNUSED(debug);

    const Skeleton* sk = mesh_->skeleton.get();

    if(!rig || !sk) {
        return;
    }

    /* Update the rig with the interpolated frame state */
    for(std::size_t j = 0; j < rig->joint_count(); ++j) {
        const JointState& state0 = joint_state_at_frame(current_frame, j);
        const JointState& state1 = joint_state_at_frame(next_frame, j);

        auto q = state0.rotation.slerp(state1.rotation, t);
        auto d = state0.translation.lerp(state1.translation, t);

        rig->joint(j)->rotate_to(q);
        rig->joint(j)->move_to(d);
    }
}

void SkeletalFrameUnpacker::unpack_frame(
    const uint32_t current_frame, const uint32_t next_frame,
    const float t, Rig* const rig, VertexData* const out, Debug* const debug
) {
    _S_UNUSED(current_frame);
    _S_UNUSED(next_frame);
    _S_UNUSED(t);

    if(!rig) {
        return;
    }

    /* Make sure everything is up-to-date */
    rig->recalc_absolute_transformations();

    /* Initialise the interpolated vertex data with all the mesh data (so UV etc. are populated) */
    mesh_->vertex_data->clone_into(*out);
    auto skeleton = mesh_->skeleton.get();

    /* Debug draw the joints */
    if(debug) {
        for(std::size_t i = 0; i < rig->joint_count(); ++i) {
            auto parent_joint = rig->joint(i)->parent();
            if(parent_joint) {
                debug->draw_line(
                    parent_joint->absolute_translation_,
                    rig->joint(i)->absolute_translation_,
                    Colour::YELLOW, 0.1f, false
                );
            }
        }
    }

    auto vdata = mesh_->vertex_data.get();

    /* Go through each skeleton vertex, and update the counterpart
     * actual vertex by manipulating its position to take into account the current
     * joint position vs the original joint position for each joint that has a
     * weighting */
    vdata->move_to_start();
    for(std::size_t i = 0; i < vertices_.size(); ++i) {
        Vec3 p = *vdata->position_at<Vec3>(i);
        Vec3 n = *vdata->normal_at<Vec3>(i);
        auto sv = vertices_[i];

        Vec3 po, no;
        for(auto k = 0; k < MAX_JOINTS_PER_VERTEX; ++k) {
            const auto j = sv.joints[k];
            if(j > -1) {
                const auto q = rig->joint(j)->absolute_rotation_;
                const auto d = rig->joint(j)->absolute_translation_;

                auto joint = skeleton->joint(j);

                /* FIXME: Optimise! Using matrices may be fewer instructions */
                Quaternion rot = q * joint->absolute_rotation().inversed();
                po += ((p - joint->absolute_translation()) * rot) + d * sv.weights[k];
                no += rot * n;
            }
        }

        out->position(po);
        out->normal(no.normalized());
        out->move_next();
    }

    out->done();
}

void SkeletalFrameUnpacker::rebuild_key_frame_absolute_transforms() {
    for(auto& frame: skeleton_frames_) {
        for(std::size_t i = 0; i < mesh_->skeleton->joint_count(); ++i) {
            Joint* joint = mesh_->skeleton->joint(i);
            Joint* parent = joint->parent();
            if(!parent) {
                frame.joints[i].absolute_rotation = joint->rotation() * frame.joints[i].rotation;
                frame.joints[i].absolute_translation = joint->translation() + frame.joints[i].translation;
            } else {
                auto& parent_rot = frame.joints[parent->id()].absolute_rotation;
                frame.joints[i].absolute_rotation = (
                    parent_rot * joint->rotation() * frame.joints[i].rotation
                );

                frame.joints[i].absolute_translation = (
                    frame.joints[parent->id()].absolute_translation +
                    parent_rot * (joint->translation() + frame.joints[i].translation)
                );
            }
        }
    }
}

}
