#include <cstring>

#include "skeleton.h"
#include "../../debug.h"

namespace smlt {

void Joint::rotate_to(const Quaternion& q) {
    rotation_ = q;
}

void Joint::move_to(const Vec3& v) {
    position_ = v;
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
        L_ERROR("Attempted to change the parent of a joint");
        return nullptr;
    }

    Bone* new_bone = &skeleton_->bones_[skeleton_->bone_count_++];
    new_bone->joints[0] = this;
    new_bone->joints[1] = other;

    // Set the parent
    other->parent_ = this;
    return new_bone;
}

Joint* Joint::parent() const {
    return parent_;
}

std::size_t Joint::id() const {
    return id_;
}

SkeletalFrameUnpacker::SkeletalFrameUnpacker(Mesh* mesh, std::size_t num_frames):
    mesh_(mesh) {

    assert(mesh->skeleton.get());

    /* Allocate the memory for all the frames */
    skeleton_frames_.resize(num_frames);
    for(auto& frame: skeleton_frames_) {
        frame.joints.resize(mesh->skeleton->joint_count());
    }
}

void SkeletalFrameUnpacker::unpack_frame(
    uint32_t current_frame, uint32_t next_frame, float t, VertexData* out, Debug* debug
) {

    /* Initialise the interpolated vertex data with all the mesh data (so UV etc. are populated) */
    mesh_->vertex_data->clone_into(*out);
    auto skeleton = mesh_->skeleton.get();

    for(std::size_t i = 0; i < skeleton->joint_count(); ++i) {
        auto& state0 = joint_state_at_frame(current_frame, i);
        auto& state1 = joint_state_at_frame(next_frame, i);

        /* Debug draw the joints */
        if(debug) {
            auto parent_joint = skeleton->joint(i)->parent();
            if(parent_joint) {
                debug->draw_line(
                    joint_state_at_frame(current_frame, parent_joint->id()).absolute_translation,
                    state0.absolute_translation,
                    Colour::YELLOW, 0.1f, false
                );
            }
        }

        // FIXME: Transform bone vertices and update positions
    }
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
                    parent_rot.rotate_vector(joint->translation() + frame.joints[i].translation)
                );
            }
        }
    }
}

}
