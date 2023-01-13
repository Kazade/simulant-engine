
#include "body.h"
#include "simulation.h"
#include "collision_listener.h"

#include "bounce/bounce.h"

#include "../../nodes/stage_node.h"
#include "../../nodes/actor.h"
#include "../../time_keeper.h"
#include "../../stage.h"

namespace smlt {
namespace behaviours {
namespace impl {

static inline void to_vec3(const b3Vec3& rhs, Vec3& ret) {
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
}

static inline void to_quat(const b3Quat& rhs, Quaternion& out) {
    out = Quaternion(
        rhs.v.x,
        rhs.v.y,
        rhs.v.z,
        rhs.s
    );
}

static inline void to_b3vec3(const Vec3& rhs, b3Vec3& ret) {
    ret.x = rhs.x;
    ret.y = rhs.y;
    ret.z = rhs.z;
}

static inline void to_b3quat(const Quaternion& q, b3Quat& ret) {
    ret.v.x = q.x;
    ret.v.y = q.y;
    ret.v.z = q.z;
    ret.s = q.w;
}

Body::Body(RigidBodySimulation* simulation):
    simulation_(simulation) {

    // Keep each body's last_state in sync when the simulation is stepped
    simulation_stepped_connection_ = simulation->signal_simulation_pre_step().connect([this]() {
        last_state_ = simulation_->body_transform(this);
    });
}

Body::~Body() {
    simulation_stepped_connection_.disconnect();
}

bool Body::init() {
    if(!simulation_) {
        return false;
    }

    return true;
}

void Body::clean_up() {
    for(auto listener: listeners_) {
        unregister_collision_listener(listener);
    }

    if(simulation_) {
        simulation_->release_body(this);
    }
}

void Body::move_to(const Vec3& position) {
    assert(simulation_);

    auto xform = simulation_->body_transform(this);
    simulation_->set_body_transform(
        this,
        position,
        xform.second
    );

    stage_node->move_to_absolute(position);
}

void Body::rotate_to(const Quaternion& rotation) {
    assert(simulation_);

    auto xform = simulation_->body_transform(this);
    simulation_->set_body_transform(
        this,
        xform.first,
        rotation
    );

    stage_node->rotate_to_absolute(rotation);
}

Quaternion Body::rotation() const {
    auto p = body_->GetOrientation();
    Quaternion r;
    to_quat(p, r);
    return r;
}

Vec3 Body::position() const {
    auto p = body_->GetPosition();
    Vec3 r;
    to_vec3(p, r);
    return r;
}

void Body::update(float dt) {
    static const bool INTERPOLATION_ENABLED = true;

    assert(simulation_);

    if(INTERPOLATION_ENABLED) {
        auto prev_state = last_state_; // This is set by the signal connected in Body::Body()
        auto next_state = simulation_->body_transform(this);

        // Prevent a divide by zero.
        float t = (dt == 0.0f) ? 0.0f : smlt::fast_divide(simulation_->time_keeper_->fixed_step_remainder(), dt);

        auto new_pos = prev_state.first.lerp(next_state.first, t);
        auto new_rot = prev_state.second.slerp(next_state.second, t);

        stage_node->move_to_absolute(new_pos);
        stage_node->rotate_to_absolute(new_rot);
    } else {
        auto state = simulation_->body_transform(this);
        stage_node->move_to_absolute(state.first);
        stage_node->rotate_to_absolute(state.second);
    }
}

void Body::store_collider(b3Fixture *fixture, const PhysicsMaterial &material, uint16_t kind) {
    // Store details about the collider so that when contacts
    // arise we can provide more detailed information to the user
    ColliderDetails details;
    details.material = material;
    details.kind = kind;

    // Make sure the b3Shape has this body as its userData!
    fixture->SetUserData(this);

    collider_details_.insert(std::make_pair(fixture, details));
}

void Body::contact_started(const Collision &collision) {
    for(auto listener: listeners_) {
        listener->on_collision_enter(collision);
    }
}

void Body::contact_finished(const Collision& collision) {
    for(auto listener: listeners_) {
        listener->on_collision_exit(collision);
    }
}

void Body::on_behaviour_added(Organism *organism) {
    StageNodeBehaviour::on_behaviour_added(organism); // Set the stage_node property from `organism`

    // We aquire the body at this point so we can set the initial transform from the stage node
    assert(simulation_);
    body_ = simulation_->acquire_body(this);
}

void Body::add_box_collider(
    const Vec3 &size, const PhysicsMaterial &properties, uint16_t kind, const Vec3 &offset, const Quaternion &rotation) {
    assert(simulation_);

    b3Vec3 p;
    b3Quat q;
    to_b3vec3(offset, p);
    to_b3quat(rotation, q);
    b3Transform tx(p, q);

    // Apply scaling
    b3Vec3 s;
    s.x = s.y = s.z = 1.0f;

    auto def = std::make_shared<b3BoxHull>(
        size.x * 0.5f, size.y * 0.5f, size.z * 0.5f
    );

    def->Transform(tx, s);
    hulls_.push_back(def);

    b3HullShape hsdef;
    hsdef.m_hull = def.get();

    b3FixtureDef sdef;
    sdef.shape = &hsdef;
    sdef.userData = this;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    store_collider(simulation_->bodies_.at(this)->CreateFixture(sdef), properties, kind);
}

void Body::add_sphere_collider(const float diameter, const PhysicsMaterial& properties, uint16_t kind, const Vec3& offset) {
    assert(simulation_);

    b3SphereShape sphere;
    to_b3vec3(offset, sphere.m_center);
    sphere.m_radius = diameter * 0.5f;

    b3FixtureDef sdef;
    sdef.shape = &sphere;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    store_collider(simulation_->bodies_.at(this)->CreateFixture(sdef), properties, kind);
}

void Body::add_triangle_collider(
    const smlt::Vec3& v1, const smlt::Vec3& v2, const smlt::Vec3& v3,
    const PhysicsMaterial& properties, uint16_t kind
) {
    assert(simulation_);

    b3Vec3 bv1, bv2, bv3;
    to_b3vec3(v1, bv1);
    to_b3vec3(v2, bv2);
    to_b3vec3(v3, bv3);

    b3TriangleShape tri;
    tri.Set(bv1, bv2, bv3);

    b3FixtureDef sdef;
    sdef.shape = &tri;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    store_collider(simulation_->bodies_.at(this)->CreateFixture(sdef), properties, kind);
}

void Body::add_capsule_collider(float height, const float diameter, const PhysicsMaterial& properties, uint16_t kind) {
    assert(simulation_);

    float off = (height - (diameter * 0.5f)) * 0.5f;
    b3Vec3 v1(0.0f, off, 0.0f);
    b3Vec3 v2(0.0f, -off, 0.0f);

    b3CapsuleShape capsule;
    capsule.m_vertex1 = v1;
    capsule.m_vertex2 = v2;
    capsule.m_radius = diameter * 0.5f;

    b3FixtureDef sdef;
    sdef.shape = &capsule;
    sdef.density = properties.density;
    sdef.friction = properties.friction;
    sdef.restitution = properties.bounciness;

    store_collider(simulation_->bodies_.at(this)->CreateFixture(sdef), properties, kind);
}

void Body::register_collision_listener(CollisionListener *listener) {
    listeners_.insert(listener);
    listener->watching_.insert(this);
}

void Body::unregister_collision_listener(CollisionListener *listener) {
    listener->watching_.erase(this);
    listeners_.erase(listener);
}



}
}

}
