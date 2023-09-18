#include "physics_body.h"

namespace smlt {

// FIXME: Calculate these values from real-world values!
const PhysicsMaterial PhysicsMaterial::WOOD(0.005, 0.4, 0.2);
const PhysicsMaterial PhysicsMaterial::RUBBER(0.001, 0.3, 0.8);
const PhysicsMaterial PhysicsMaterial::IRON(0.1, 0.2, 0.00001);
const PhysicsMaterial PhysicsMaterial::STONE(0.1, 0.8, 0.00001);

const static auto& w = PhysicsMaterial::WOOD;
const static auto& r = PhysicsMaterial::RUBBER;
const static auto& i = PhysicsMaterial::IRON;
const static auto& s = PhysicsMaterial::STONE;

const PhysicsMaterial PhysicsMaterial::WOOD_25(w.density * 0.25f, w.friction, w.bounciness);
const PhysicsMaterial PhysicsMaterial::WOOD_50(w.density * 0.50f, w.friction, w.bounciness);
const PhysicsMaterial PhysicsMaterial::WOOD_75(w.density * 0.75f, w.friction, w.bounciness);

const PhysicsMaterial PhysicsMaterial::RUBBER_25(r.density * 0.25f, r.friction, r.bounciness);
const PhysicsMaterial PhysicsMaterial::RUBBER_50(r.density * 0.50f, r.friction, r.bounciness);
const PhysicsMaterial PhysicsMaterial::RUBBER_75(r.density * 0.75f, r.friction, r.bounciness);

const PhysicsMaterial PhysicsMaterial::IRON_25(i.density * 0.25f, i.friction, i.bounciness);
const PhysicsMaterial PhysicsMaterial::IRON_50(i.density * 0.50f, i.friction, i.bounciness);
const PhysicsMaterial PhysicsMaterial::IRON_75(i.density * 0.75f, i.friction, i.bounciness);

const PhysicsMaterial PhysicsMaterial::STONE_25(s.density * 0.25f, s.friction, s.bounciness);
const PhysicsMaterial PhysicsMaterial::STONE_50(s.density * 0.50f, s.friction, s.bounciness);
const PhysicsMaterial PhysicsMaterial::STONE_75(s.density * 0.75f, s.friction, s.bounciness);

void PhysicsBody::add_box_collider(
    const Vec3 &size, const PhysicsMaterial &properties, uint16_t kind, const Vec3 &offset, const Quaternion &rotation
) {
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

void PhysicsBody::add_sphere_collider(const float diameter, const PhysicsMaterial& properties, uint16_t kind, const Vec3& offset) {
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

void PhysicsBody::add_triangle_collider(
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

void PhysicsBody::add_capsule_collider(float height, const float diameter, const PhysicsMaterial& properties, uint16_t kind) {
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

void PhysicsBody::register_collision_listener(CollisionListener *listener) {
    listeners_.insert(listener);
    listener->watching_.insert(this);
}

void PhysicsBody::unregister_collision_listener(CollisionListener *listener) {
    listener->watching_.erase(this);
    listeners_.erase(listener);
}

}
