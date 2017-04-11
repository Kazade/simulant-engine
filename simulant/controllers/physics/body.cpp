
#include "body.h"
#include "simulation.h"

#include "../../nodes/stage_node.h"
#include "../../nodes/actor.h"
#include "../../time_keeper.h"
#include "../../deps/bounce/bounce.h"
#include "../../stage.h"

#include "../../utils/mesh/triangulate.h"

namespace smlt {
namespace controllers {
namespace impl {

Body::Body(Controllable* object, RigidBodySimulation* simulation):
    Controller(),
    simulation_(simulation->shared_from_this()) {

    object_ = dynamic_cast<StageNode*>(object);
    if(!object_) {
        throw std::runtime_error("Tried to attach a rigid body controller to something that isn't moveable");
    }

    // Keep each body's last_state in sync when the simulation is stepped
    simulation_stepped_connection_ = simulation->signal_simulation_pre_step().connect([this]() {
        if(auto sim = simulation_.lock()) {
            last_state_ = sim->body_transform(this);
        }
    });
}

Body::~Body() {
    simulation_stepped_connection_.disconnect();
}

bool Body::init() {
    auto sim = simulation_.lock();
    if(!sim) {
        return false;
    }

    body_ = sim->acquire_body(this);

    return true;
}

void Body::cleanup() {
    auto sim = simulation_.lock();
    if(sim) {
        sim->release_body(this);
    }
}

void Body::move_to(const Vec3& position) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    auto xform = sim->body_transform(this);
    sim->set_body_transform(
        this,
        position,
        xform.second
    );

    object_->move_to_absolute(position);
}

void Body::update(float dt) {
    const bool INTERPOLATION_ENABLED = true;

    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    if(INTERPOLATION_ENABLED) {
        auto prev_state = last_state_; // This is set by the signal connected in Body::Body()
        auto next_state = sim->body_transform(this);

        float t = sim->time_keeper_->fixed_step_remainder() / dt;

        auto new_pos = prev_state.first.lerp(next_state.first, t);
        auto new_rot = prev_state.second.slerp(next_state.second, t);

        object_->move_to_absolute(new_pos);
        object_->rotate_to_absolute(new_rot);
    } else {
        auto state = sim->body_transform(this);
        object_->move_to_absolute(state.first);
        object_->rotate_to_absolute(state.second);
    }
}

void Body::add_box_collider(const Vec3 &size, const PhysicsMaterial &properties, const Vec3 &offset, const Quaternion &rotation) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3Vec3 p;
    b3Quat q;
    to_b3vec3(offset, p);
    to_b3quat(rotation, q);
    b3Transform tx(p, q);

    // Apply scaling
    tx.rotation[0][0] = size.x * 0.5;
    tx.rotation[1][1] = size.y * 0.5;
    tx.rotation[2][2] = size.z * 0.5;

    auto def = std::make_shared<b3BoxHull>();
    def->Set(size.x * 0.5, size.y * 0.5, size.z * 0.5);
    def->SetTransform(tx);
    hulls_.push_back(def);

    b3HullShape hsdef;
    hsdef.m_hull = def.get();

    b3ShapeDef sdef;
    sdef.shape = &hsdef;
    sdef.userData = this;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    sim->bodies_.at(this)->CreateShape(sdef);
}

void Body::add_sphere_collider(const float diameter, const PhysicsMaterial& properties, const Vec3& offset) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    b3SphereShape sphere;
    to_b3vec3(offset, sphere.m_center);
    sphere.m_radius = diameter * 0.5;

    b3ShapeDef sdef;
    sdef.shape = &sphere;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    sim->bodies_.at(this)->CreateShape(sdef);
}

void Body::add_mesh_collider(const MeshID &mesh_id, const PhysicsMaterial &properties, const Vec3 &offset, const Quaternion &rotation) {
    auto sim = simulation_.lock();
    if(!sim) {
        return;
    }

    // If we haven't already seen this mesh, then create a new b3Mesh for it
    if(meshes_.count(mesh_id) == 0) {
        auto bmesh = std::make_shared<b3MeshGenerator>();
        auto mesh = mesh_id.fetch();

        std::vector<Vec3> vertices;
        std::vector<utils::Triangle> triangles;

        // Turn the mesh into a list of vertices + triangle indexes
        triangulate(mesh, vertices, triangles);

        // Add them to our b3Mesh generator
        bmesh->insert_vertices(vertices.begin(), vertices.end());
        bmesh->insert_triangles(triangles.begin(), triangles.end());

        // Store the generator for later
        meshes_.insert(std::make_pair(mesh_id, bmesh));
    }

    b3MeshShape shape;
    // Grab the b3Mesh from the generator
    shape.m_mesh = meshes_.at(mesh_id)->get_mesh();

    b3ShapeDef sdef;
    sdef.shape = &shape;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    sim->bodies_.at(this)->CreateShape(sdef);
}

Body::b3MeshGenerator::b3MeshGenerator():
    mesh_(new b3Mesh()) {

}

void Body::b3MeshGenerator::append_vertex(const Vec3 &v) {
    b3Vec3 bv;
    to_b3vec3(v, bv);
    vertices_.push_back(bv);

    mesh_->vertices = &vertices_[0];
    mesh_->vertexCount = vertices_.size();
}

void Body::b3MeshGenerator::append_triangle(const utils::Triangle& src) {
    b3Triangle tri;
    tri.v1 = src.idx[0];
    tri.v2 = src.idx[1];
    tri.v3 = src.idx[2];

    triangles_.push_back(tri);
    mesh_->triangles = &triangles_[0];
    mesh_->triangleCount = triangles_.size();
    mesh_->BuildTree(); // Rebuild the tree
}

}
}

}
