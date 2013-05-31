#include <algorithm>
#include <functional>

#include "kazmath/mat3.h"

#include "scene.h"
#include "object.h"
#include "object_visitor.h"

namespace kglt {

uint64_t Object::object_counter = 0;

Object::Object(Stage *subscene):
    uuid_(++object_counter),
    subscene_(subscene),
    is_visible_(true),
    rotation_locked_(false),
    position_locked_(false) {

    kmVec3Fill(&position_, 0.0, 0.0, 0.0);
    kmQuaternionIdentity(&rotation_);
    kmVec3Fill(&absolute_position_, 0.0, 0.0, 0.0);
    kmQuaternionIdentity(&absolute_orientation_);

    update_from_parent();

    //When the parent changes, update the position/orientation
    parent_changed_connection_ = signal_parent_changed().connect(sigc::mem_fun(this, &Object::parent_changed_callback));
}

Object::~Object() {
    parent_changed_connection_.disconnect();
}

void Object::lock_rotation(float angle, float x, float y, float z) {
    kmVec3 axis;
    kmVec3Fill(&axis, x, y, z);
    kmQuaternionRotationAxisAngle(&rotation_, &axis, kmDegreesToRadians(angle));
    kmQuaternionAssign(&absolute_orientation_, &rotation_);
    transformation_changed();
    rotation_locked_ = true;
}

void Object::unlock_rotation() {
    rotation_locked_ = false;
    update_from_parent();
}

void Object::lock_position(float x, float y, float z) {
    move_to(x, y, z);
    position_locked_ = true;
}

void Object::unlock_position() {
    position_locked_ = false;
    update_from_parent();
}

void Object::move_to(float x, float y, float z) {
    if(position_locked_) return;

    position_.x = x;
    position_.y = y;
    position_.z = z;

    update_from_parent();
}

void Object::move_forward(float amount) {
    if(position_locked_) return;

    kmVec3 forward;
    kmQuaternionGetForwardVec3RH(&forward, &absolute_rotation());
    kmVec3Scale(&forward, &forward, amount);
    kmVec3Add(&position_, &position_, &forward);

    update_from_parent();
}

void Object::rotate_to(const kmQuaternion& quat) {
    if(rotation_locked_) return;

    kmQuaternionAssign(&rotation_, &quat);
    update_from_parent();
}

void Object::rotate_to(float angle, float x, float y, float z) {
    if(rotation_locked_) return;

    kmVec3 axis;
    kmVec3Fill(&axis, x, y, z);
    kmQuaternionRotationAxisAngle(&rotation_, &axis, kmDegreesToRadians(angle));
    update_from_parent();
}

void Object::rotate_x(float amount) {
    if(rotation_locked_) return;
    if(fabs(amount) < kmEpsilon) return;

    kmQuaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, 1, 0, 0);
    kmQuaternionRotationAxisAngle(&rot, &axis, kmDegreesToRadians(amount));
    kmQuaternionMultiply(&rotation_, &rotation_, &rot);
    kmQuaternionNormalize(&rotation_, &rotation_);

    update_from_parent();
}

void Object::rotate_z(float amount) {
    if(rotation_locked_) return;
    if(fabs(amount) < kmEpsilon) return;

    kmQuaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, 0, 0, 1);
    kmQuaternionRotationAxisAngle(&rot, &axis, kmDegreesToRadians(amount));
    kmQuaternionMultiply(&rotation_, &rotation_, &rot);
    kmQuaternionNormalize(&rotation_, &rotation_);

    update_from_parent();
}

void Object::rotate_y(float amount) {
    if(rotation_locked_) return;
    if(fabs(amount) < kmEpsilon) return;

    kmQuaternion rot;
    float rads = kmDegreesToRadians(amount);
    kmQuaternionRotationAxisAngle(&rot, &KM_VEC3_POS_Y, rads);
    kmQuaternionMultiply(&rotation_, &rotation_, &rot);
    kmQuaternionNormalize(&rotation_, &rotation_);

    update_from_parent();
}

kmMat4 Object::absolute_transformation() {
    kmMat4 rot_matrix, trans_matrix, final;

    kmMat4RotationQuaternion(&rot_matrix, &absolute_orientation_);
    kmMat4Translation(&trans_matrix, absolute_position().x, absolute_position().y, absolute_position().z);

    kmMat4Multiply(&final, &trans_matrix, &rot_matrix);
    return final;
}

void Object::set_position(const kmVec3& pos) {
    kmVec3Assign(&position_, &pos);
}

void Object::update_from_parent() {
    if(!has_parent()) {
        kmVec3Assign(&absolute_position_, &position_);
        kmQuaternionAssign(&absolute_orientation_, &rotation_);
        transformation_changed();
    } else {
        if(!position_locked_) {
            kmVec3Add(&absolute_position_, &parent().absolute_position_, &position_);
        }
        if(!rotation_locked_) {
            kmQuaternionMultiply(&absolute_orientation_, &rotation_, &parent().absolute_orientation_);
            kmQuaternionNormalize(&absolute_orientation_, &absolute_orientation_);
        }
    }

    transformation_changed(); //FIXME: Did it though?

    std::for_each(children().begin(), children().end(), [](Object* x) { x->update_from_parent(); });    
}

void Object::destroy_children() {
    //If this looks weird, it's because when you destroy
    //children the index changes so you need to gather them
    //up first and then destroy them
    std::vector<Object*> to_destroy;
    for(uint32_t i = 0; i < child_count(); ++i) {
        to_destroy.push_back(&child(i));
    }
    for(Object* o: to_destroy) {
        o->destroy();
    }
}

}
