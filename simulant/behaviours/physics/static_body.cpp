#include "static_body.h"
#include "simulation.h"

#include "bounce/bounce.h"
#include "bounce/collision/shapes/mesh_shape.h"

#include "../../utils/mesh/triangulate.h"

namespace smlt {
namespace behaviours {

static inline void to_b3vec3(const Vec3& rhs, b3Vec3& ret) {
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
}

StaticBody::StaticBody(RigidBodySimulation* simulation):
    Body(simulation) {

}

StaticBody::~StaticBody() {

}

StaticBody::b3MeshGenerator::b3MeshGenerator():
    mesh_(new b3Mesh()) {

}

void StaticBody::b3MeshGenerator::insert_triangles(
    const std::vector<utils::Triangle>::iterator first,
    const std::vector<utils::Triangle>::iterator last) {
    b3MeshTriangle btri;

    for(auto it = first; it != last; ++it) {
        utils::Triangle& tri = (*it);

        btri.v1 = tri.idx[0];
        btri.v2 = tri.idx[1];
        btri.v3 = tri.idx[2];

        triangles_.push_back(btri);
        mesh_->triangles = &triangles_[0];
        mesh_->triangleCount = triangles_.size();
    }
}

void StaticBody::b3MeshGenerator::append_vertex(const Vec3 &v) {
    b3Vec3 bv;
    to_b3vec3(v, bv);
    vertices_.push_back(bv);

    mesh_->vertices = &vertices_[0];
    mesh_->vertexCount = vertices_.size();
}

void StaticBody::add_mesh_collider(const MeshID &mesh_id, const PhysicsMaterial &properties, const Vec3 &offset, const Quaternion &rotation) {
    assert(simulation_);

    auto& mesh_cache = get_mesh_cache();

    // If we haven't already seen this mesh, then create a new b3Mesh for it
    if(mesh_cache.count(mesh_id) == 0) {
        auto bmesh = std::make_shared<b3MeshGenerator>();
        auto mesh = mesh_id.fetch();

        std::vector<Vec3> vertices;
        std::vector<utils::Triangle> triangles;

        // Turn the mesh into a list of vertices + triangle indexes
        triangulate(mesh, vertices, triangles);

        // Add them to our b3Mesh generator
        bmesh->insert_vertices(vertices.begin(), vertices.end());
        bmesh->insert_triangles(triangles.begin(), triangles.end());

        // Build mesh AABB tree and mesh adjacency
        bmesh->get_mesh()->BuildTree();
        bmesh->get_mesh()->BuildAdjacency();

        // Store the generator for later
        mesh_cache.insert(std::make_pair(mesh_id, bmesh));
    }

    // Grab the b3Mesh from the generator
    b3Mesh* genMesh = mesh_cache.at(mesh_id)->get_mesh();

    b3MeshShape shape;
    shape.m_mesh = genMesh;

    b3FixtureDef sdef;
    sdef.shape = &shape;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    store_collider(simulation_->bodies_.at(this)->CreateFixture(sdef), properties);
}

StaticBody::MeshCache& StaticBody::get_mesh_cache() {
    static MeshCache mesh_cache;
    return mesh_cache;
}


}
}
