#pragma once

#include <cstdint>

#include "../../math/quaternion.h"
#include "../../math/vec3.h"
#include "../../meshes/mesh.h"

#define MAX_BONES_PER_VERTEX 4
#define MAX_BONES_PER_MESH 64

namespace smlt {

struct Bone;
class Skeleton;

class Joint {
    friend class Skeleton;

public:
    Joint(Skeleton* skeleton):
        skeleton_(skeleton) {}

    void rotate_to(const Quaternion& q);
    void move_to(const smlt::Vec3& v);
    void set_name(const std::string& name);

    Bone* link_to(Joint* other);

    Joint* parent() const;
    std::size_t id() const;
    void set_id(std::size_t id) {
        id_ = id;
    }
private:
    Skeleton* skeleton_ = nullptr;
    std::size_t id_ = 0;  // 0 == root

    Joint* parent_ = nullptr;

    char name_[32];
    Quaternion rotation_;
    Vec3 position_;
};

struct Bone {
    Joint* joints[2];
};

struct SkeletonVertex {
    smlt::Vec3 position;
    Bone* bones[MAX_BONES_PER_VERTEX];
    float weights[MAX_BONES_PER_VERTEX];
};

class Mesh;

class Skeleton {
    friend class Joint;
public:
    Skeleton(Mesh* mesh, uint32_t num_vertices, uint32_t num_joints):
        mesh_(mesh) {
        joints_.resize(num_joints, Joint(this));
        vertices_.resize(num_vertices);
    }

    SkeletonVertex* vertex(uint32_t idx) {
        return &vertices_[idx];
    }

    Joint* joint(uint32_t idx) {
        return &joints_[idx];
    }

    Joint* find_joint(const std::string& name) {
        for(auto& joint: joints_) {
            if(joint.name_ == name) {
                return &joint;
            }
        }

        return nullptr;
    }

    std::size_t vertex_count() const {
        return vertices_.size();
    }

    std::size_t joint_count() const {
        return joints_.size();
    }
private:
    Mesh* mesh_ = nullptr;
    std::vector<Joint> joints_;
    std::vector<SkeletonVertex> vertices_;

    Bone bones_[MAX_BONES_PER_MESH];
    uint8_t bone_count_ = 0;
};

struct JointState {
    /* The translation and rotation from the parent joint */
    smlt::Quaternion rotation;
    smlt::Vec3 translation;

    /* The absolute rotation/translation taking into account all
     * parents */
    smlt::Quaternion absolute_rotation;
    smlt::Vec3 absolute_translation;
};

struct SkeletonFrame {
    std::vector<JointState> joints;
};

/* This processes the skeleton and updates the vertex
 * data with the new vertex positions and normals */
class SkeletalFrameUnpacker : public MeshFrameData {
public:
    SkeletalFrameUnpacker(Mesh* mesh, std::size_t num_frames);

    virtual void unpack_frame(uint32_t current_frame, uint32_t next_frame, float t, VertexData* out);

    void set_joint_state_at_frame(std::size_t frame, std::size_t joint, JointState state) {
        skeleton_frames_[frame].joints[joint] = state;
    }

    const JointState& joint_state_at_frame(std::size_t frame, std::size_t joint) const {
        return skeleton_frames_[frame].joints[joint];
    }

    /* Recursively visit joints from the root (0) and
     * recalculate the absolute_rotation and translation
     * for each. */
    void rebuild_key_frame_absolute_transforms();

private:
    Mesh* mesh_ = nullptr;

    /* Key frames for skeletal animation */
    std::vector<SkeletonFrame> skeleton_frames_;
};

}
