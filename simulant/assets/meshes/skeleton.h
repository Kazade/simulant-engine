#pragma once

#include <cstdint>

#include "../../math/quaternion.h"
#include "../../math/vec3.h"

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

private:
    Skeleton* skeleton_ = nullptr;

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

}
