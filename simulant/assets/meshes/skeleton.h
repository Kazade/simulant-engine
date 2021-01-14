#pragma once

#include <cstdint>

#include "../../math/quaternion.h"
#include "../../math/vec3.h"
#include "../../meshes/mesh.h"

#define MAX_JOINTS_PER_VERTEX 4
#define MAX_JOINTS_PER_MESH 64

namespace smlt {

struct Bone;
class Skeleton;

/* This is an index into the vertex data array
 * that and the weight that this joint affects that
 * vertex */
struct JointVertex {
    std::size_t vertex_id = 0;
    float weight = 0.0f;
};

class Joint {
    friend class Skeleton;

public:
    Joint(Skeleton* skeleton):
        skeleton_(skeleton) {

        // Just give a joint a bit of memory for
        // vertices to start with
        vertices_.reserve(32);
    }

    void rotate_to(const Quaternion& q);
    void move_to(const smlt::Vec3& v);

    std::string name() const;
    void set_name(const std::string& name);

    Bone* link_to(Joint* other);

    Joint* parent() const;
    std::size_t id() const;
    void set_id(std::size_t id) {
        id_ = id;
    }

    const smlt::Quaternion& rotation() const {
        return rotation_;
    }

    const smlt::Vec3& translation() const {
        return translation_;
    }

    const smlt::Quaternion& absolute_rotation() const {
        return absolute_rotation_;
    }

    const smlt::Vec3 absolute_translation() const {
        return absolute_translation_;
    }

private:
    Skeleton* skeleton_ = nullptr;
    std::size_t id_ = 0;  // 0 == root

    Joint* parent_ = nullptr;

    char name_[33]; // 32 + \0

    Quaternion rotation_;
    Quaternion absolute_rotation_;

    Vec3 translation_;
    Vec3 absolute_translation_;

    std::vector<JointVertex> vertices_;

    void recalc_absolute_transformation();
};

struct Bone {
    Joint* joints[2] = {nullptr, nullptr};
};

class Mesh;

class Skeleton {
    friend class Joint;
public:
    Skeleton(Mesh* mesh, std::size_t num_joints):
        mesh_(mesh) {
        joints_.resize(num_joints, Joint(this));
    }

    Joint* joint(std::size_t idx) {
        return &joints_[idx];
    }

    const Joint* joint(std::size_t idx) const {
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

    std::size_t joint_count() const {
        return joints_.size();
    }

    void attach_vertex_to_joint(std::size_t j, std::size_t vertex_index, float weight) {
        JointVertex v;
        v.vertex_id = vertex_index;
        v.weight = weight;
        joint(j)->vertices_.push_back(v);
    }

private:
    Mesh* mesh_ = nullptr;
    std::vector<Joint> joints_;

    Bone bones_[MAX_JOINTS_PER_MESH];
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

struct SkeletonVertex {
    int32_t joints[MAX_JOINTS_PER_VERTEX] = {-1, -1, -1, -1};
    float weights[MAX_JOINTS_PER_VERTEX] = {0, 0, 0, 0};
};

/* This processes the skeleton and updates the vertex
 * data with the new vertex positions and normals */
class SkeletalFrameUnpacker : public FrameUnpacker {
public:
    SkeletalFrameUnpacker(Mesh* mesh, std::size_t num_frames, std::size_t num_vertices);

    void prepare_unpack(
        uint32_t current_frame,
        uint32_t next_frame,
        float t, Rig* const rig,
        Debug* const debug=nullptr
    ) override;

    virtual void unpack_frame(
        const uint32_t current_frame,
        const uint32_t next_frame,
        const float t,
        Rig* const rig,
        VertexData* const out,
        Debug* const debug=nullptr
    ) override;

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

    const SkeletonVertex* vertex_at(std::size_t i);

    bool link_vertex_to_joint(std::size_t vertex, std::size_t j, float weight) {
        auto vert = &vertices_[vertex];
        for(uint8_t i = 0; i < MAX_JOINTS_PER_VERTEX; ++i) {
            if(vert->joints[i] < 0) {
                vert->joints[i] = j;
                vert->weights[i] = weight;
                return true;
            }
        }

        return false;
    }

    const std::vector<SkeletonVertex>& vertices() const {
        return vertices_;
    }
private:
    Mesh* mesh_ = nullptr;

    /* Key frames for skeletal animation */
    std::vector<SkeletonFrame> skeleton_frames_;
    std::vector<SkeletonVertex> vertices_;
};

}
