#include "../../object.h"
#include "../../stage.h"
#include "../../scene.h"

#include "ode_collidable.h"
#include "ode_body.h"
#include "ode_engine.h"

namespace kglt {
namespace physics {

dSpaceID ODECollidable::get_space() {
    auto e = engine().as<ODEEngine>();
    return e->space_;
}

bool ODECollidable::init() {

    return true;
}

void ODECollidable::cleanup() {
    for(auto p: shapes_) {
        std::lock_guard<std::recursive_mutex> lock(engine()->mutex());
        dGeomDestroy(p.second);
    }
    shapes_.clear();
}

void ODECollidable::attach_to_responsive_body(ResponsiveBody& reponsive) {
    ODEBody& body = dynamic_cast<ODEBody&>(reponsive);

    for(auto p: shapes_) {
        std::lock_guard<std::recursive_mutex> lock(engine()->mutex());
        dGeomSetBody(p.second, body.body_);
    }
}

ShapeID ODECollidable::add_plane(float a, float b, float c, float d) {
    std::lock_guard<std::recursive_mutex> lock(engine()->mutex());

    dGeomID new_geom = dCreatePlane(get_space(), a, b, c, d);
    dGeomSetData(new_geom, this);
    ShapeID new_id = get_next_shape_id();

    shapes_.insert(std::make_pair(new_id, new_geom));

    if(owner() && owner()->is_responsive()) {
        attach_to_responsive_body(owner()->body());
    }

    return new_id;
}

ShapeID ODECollidable::add_sphere(float radius) {
    std::lock_guard<std::recursive_mutex> lock(engine()->mutex());

    dGeomID new_geom = dCreateSphere(get_space(), radius);
    dGeomSetData(new_geom, this);

    ShapeID new_id = get_next_shape_id();

    shapes_.insert(std::make_pair(new_id, new_geom));

    if(owner() && owner()->is_responsive()) {
        attach_to_responsive_body(owner()->body());
    }

    return new_id;
}

ShapeID ODECollidable::add_box(float width, float height, float depth) {
    std::lock_guard<std::recursive_mutex> lock(engine()->mutex());

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
    std::lock_guard<std::recursive_mutex> lock(engine()->mutex());

    dGeomID new_geom = dCreateCapsule(get_space(), radius, length);
    dGeomSetData(new_geom, this);

    ShapeID new_id = get_next_shape_id();

    shapes_.insert(std::make_pair(new_id, new_geom));

    if(owner() && owner()->is_responsive()) {
        attach_to_responsive_body(owner()->body());
    }
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
