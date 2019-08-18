#pragma once

#include "body.h"

namespace smlt {
namespace behaviours {

/*
 * Almost the same as a rigid body, but has no mass, and doesn't take part in the simulation
 * aside from acting as a collider */
class StaticBody:
    public impl::Body,
    public RefCounted<StaticBody> {

public:
    StaticBody(RigidBodySimulation *simulation);
    virtual ~StaticBody();

    using impl::Body::init;
    using impl::Body::clean_up;

    const std::string name() const { return "Static Body"; }

    void add_mesh_collider(
        const MeshID& mesh,
        const PhysicsMaterial& properties,
        const Vec3& offset=Vec3(), const Quaternion& rotation=Quaternion()
    );

private:
    bool is_dynamic() const override { return false; }

    class b3MeshGenerator {
    private:
        std::vector<b3Vec3> vertices_;
        std::vector<b3Triangle> triangles_;

        std::shared_ptr<b3Mesh> mesh_;

    public:
        b3MeshGenerator();

        template<typename InputIterator>
        void insert_vertices(InputIterator first, InputIterator last) {
            for(auto it = first; it != last; ++it) {
                append_vertex((*it));
            }
        }

        template<typename InputIterator>
        void insert_triangles(InputIterator first, InputIterator last) {
            for(auto it = first; it != last; ++it) {
                append_triangle((*it));
            }
        }

        void append_vertex(const Vec3& v);
        void append_triangle(const utils::Triangle& tri);
        b3Mesh* get_mesh() const { return mesh_.get(); }
    };

    static std::unordered_map<MeshID, std::shared_ptr<b3MeshGenerator>> mesh_cache;
};

}
}
