#include "../../object.h"
#include "../../stage.h"
#include "../../scene.h"

#include "ode_collidable.h"
#include "ode_body.h"
#include "ode_engine.h"

namespace kglt {
namespace physics {

dSpaceID ODECollidable::get_space() {
    ODEEngine* e = dynamic_cast<ODEEngine*>(engine());
    return e->space_;
}

bool ODECollidable::init() {

    return true;
}

void ODECollidable::cleanup() {
    for(auto p: shapes_) {
        dGeomDestroy(p.second);
    }
    shapes_.clear();
}

void ODECollidable::attach_to_responsive_body(ResponsiveBody& reponsive) {
    ODEBody& body = dynamic_cast<ODEBody&>(reponsive);

    for(auto p: shapes_) {
        dGeomSetBody(p.second, body.body_);
    }
}

ShapeID ODECollidable::add_plane(float a, float b, float c, float d) {
    dGeomID new_geom = dCreatePlane(get_space(), a, b, c, d);
    dGeomSetData(new_geom, this);
    ShapeID new_id = get_next_shape_id();

    shapes_.insert(std::make_pair(new_id, new_geom));
    return new_id;
}

ShapeID ODECollidable::add_sphere(float radius) {
    dGeomID new_geom = dCreateSphere(get_space(), radius);
    dGeomSetData(new_geom, this);

    ShapeID new_id = get_next_shape_id();

    shapes_.insert(std::make_pair(new_id, new_geom));
    return new_id;
}

ShapeID ODECollidable::add_box(float width, float height, float depth) {
    dGeomID new_geom = dCreateBox(get_space(), width, height, depth);
    dGeomSetData(new_geom, this);

    ShapeID new_id = get_next_shape_id();

    shapes_.insert(std::make_pair(new_id, new_geom));

    if(owner() && owner()->is_responsive()) {
        attach_to_responsive_body(owner()->body());
    }
    return new_id;
}

ShapeID ODECollidable::add_capsule(float radius, float length) {
    dGeomID new_geom = dCreateCapsule(get_space(), radius, length);
    dGeomSetData(new_geom, this);

    ShapeID new_id = get_next_shape_id();

    shapes_.insert(std::make_pair(new_id, new_geom));
    return new_id;
}

void ODECollidable::set_relative_position(ShapeID shape, const Vec3& pos) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void ODECollidable::set_absolute_position(ShapeID shape, const Vec3& pos) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void ODECollidable::set_relative_rotation(ShapeID shape, const Quaternion& quat) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void ODECollidable::set_absolute_rotation(ShapeID shape, const Quaternion& quat) {
    throw NotImplementedError(__FILE__, __LINE__);
}

void ODECollidable::set_bounciness(float value) {
    bounciness_ = value;
}

void ODECollidable::set_friction(int32_t friction) {
    friction_ = friction;
}

float ODECollidable::bounciness() const {
    return bounciness_;
}

int32_t ODECollidable::friction() const {
    return friction_;
}

}
}
