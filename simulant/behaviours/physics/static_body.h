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

    const char* name() const override { return "Static Body"; }

    void add_mesh_collider(
        const MeshID& mesh,
        const PhysicsMaterial& properties,
        uint16_t kind=0,
        const Vec3& offset=Vec3(), const Quaternion& rotation=Quaternion()
    );

private:
    bool is_dynamic() const override { return false; }

    class b3MeshGenerator {
    private:
        std::vector<b3Vec3> vertices_;
        std::vector<b3MeshTriangle> triangles_;
        std::shared_ptr<b3Mesh> mesh_;

    public:
        b3MeshGenerator();

        template<typename InputIterator>
        void insert_vertices(InputIterator first, InputIterator last) {
            for(auto it = first; it != last; ++it) {
                append_vertex((*it));
            }
        }

        void insert_triangles(
            const std::vector<utils::Triangle>::iterator first,
            const std::vector<utils::Triangle>::iterator last
        );

        void append_vertex(const Vec3& v);

        b3Mesh* get_mesh() const { return mesh_.get(); }
    };

    typedef std::unordered_map<MeshID, std::shared_ptr<b3MeshGenerator>> MeshCache;
    static MeshCache& get_mesh_cache();
};

}
}
