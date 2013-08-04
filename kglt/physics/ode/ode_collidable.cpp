#include "../../object.h"
#include "../../stage.h"
#include "../../scene.h"

#include "ode_collidable.h"
#include "ode_body.h"
#include "ode_engine.h"

namespace kglt {
namespace physics {

static uint32_t shape_counter = 0;

dSpaceID ODECollidable::get_space() {
    ODEEngine* engine = dynamic_cast<ODEEngine*>(owner()->stage().scene().physics_engine());
    return engine->space_;
}

bool ODECollidable::init() {

    return true;
}

void ODECollidable::cleanup() {

}

void ODECollidable::attach_to_responsive_body(ResponsiveBody& reponsive) {
    ODEBody& body = dynamic_cast<ODEBody&>(reponsive);

    for(auto p: shapes_) {
        dGeomSetBody(p.second, body.body_);
    }
}

ShapeID ODECollidable::add_sphere(float radius) {
    dGeomID new_geom = dCreateSphere(get_space(), radius);
    ShapeID new_id(shape_counter++);

    shapes_.insert(std::make_pair(new_id, new_geom));
    return new_id;
}

ShapeID ODECollidable::add_box(float width, float height, float depth) {
    dGeomID new_geom = dCreateBox(get_space(), width, height, depth);
    ShapeID new_id(shape_counter++);

    shapes_.insert(std::make_pair(new_id, new_geom));
    return new_id;
}

ShapeID ODECollidable::add_capsule(float radius, float length) {

}

void ODECollidable::set_relative_position(ShapeID shape, const Vec3& pos) {

}

void ODECollidable::set_absolute_position(ShapeID shape, const Vec3& pos) {

}

void ODECollidable::set_relative_rotation(ShapeID shape, const Quaternion& quat) {

}

void ODECollidable::set_absolute_rotation(ShapeID shape, const Quaternion& quat) {

}

}
}
