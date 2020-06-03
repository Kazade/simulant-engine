#include <cstring>

#include "skeleton.h"

namespace smlt {

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

    Bone* new_bone = &skeleton_->bones_[skeleton_->bone_count_++];
    new_bone->joints[0] = this;
    new_bone->joints[1] = other;
    return new_bone;
}

void SkeletalFrameUnpacker::unpack_frame(uint32_t current_frame, uint32_t next_frame, float t, VertexData* out) {
    /* Initialise the interpolated vertex data with all the mesh data (so UV etc. are populated) */
    mesh_->vertex_data->clone_into(*out);

    /* Now, update the positions and normals based on the skeleton */
}

}
