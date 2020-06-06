#include <cstring>

#include "skeleton.h"

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

SkeletalFrameUnpacker::SkeletalFrameUnpacker(Mesh* mesh, std::size_t num_frames):
    mesh_(mesh) {

    assert(mesh->skeleton.get());

    /* Allocate the memory for all the frames */
    skeleton_frames_.resize(num_frames);
    for(auto& frame: skeleton_frames_) {
        frame.joints.resize(mesh->skeleton->joint_count());
    }
}

void SkeletalFrameUnpacker::unpack_frame(uint32_t current_frame, uint32_t next_frame, float t, VertexData* out) {
    /* Initialise the interpolated vertex data with all the mesh data (so UV etc. are populated) */
    mesh_->vertex_data->clone_into(*out);
    auto skeleton = mesh_->skeleton.get();

    for(std::size_t i = 0; i < skeleton->joint_count(); ++i) {
        auto& state0 = joint_state_at_frame(current_frame, i);
        auto& state1 = joint_state_at_frame(next_frame, i);

        // FIXME: Transform bone vertices and update positions
    }
}

}
