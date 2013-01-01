#include <algorithm>
#include <functional>

#include "kazmath/mat3.h"

#include "scene.h"
#include "object.h"
#include "object_visitor.h"

namespace kglt {

uint64_t Object::object_counter = 0;

Object::Object(SubScene *subscene):
    uuid_(++object_counter),
    subscene_(subscene),
    is_visible_(true) {

    kmVec3Fill(&position_, 0.0, 0.0, 0.0);
    kmQuaternionIdentity(&rotation_);
    kmVec3Fill(&absolute_position_, 0.0, 0.0, 0.0);
    kmQuaternionIdentity(&absolute_orientation_);

    update_from_parent();

    //When the parent changes, update the position/orientation
    signal_parent_changed().connect(sigc::mem_fun(this, &Object::parent_changed_callback));
}

Object::~Object() {

}

void Object::move_to(float x, float y, float z) {
    position_.x = x;
    position_.y = y;
    position_.z = z;

    update_from_parent();
    transformation_changed();
}

void Object::move_forward(float amount) {
    kmMat3 rot_matrix;
    kmVec3 forward;
    kmMat3RotationQuaternion(&rot_matrix, &rotation_);
    kmMat3GetForwardVec3(&forward, &rot_matrix);
    kmVec3Normalize(&forward, &forward);
    kmVec3Scale(&forward, &forward, amount);
    kmVec3Add(&position_, &position_, &forward);

    update_from_parent();
    transformation_changed();
}

void Object::rotate_to(float angle, float x, float y, float z) {
    kmVec3 axis;
    kmVec3Fill(&axis, x, y, z);
    kmQuaternionRotationAxis(&rotation_, &axis, kmDegreesToRadians(angle));
    update_from_parent();
    transformation_changed();
}

void Object::rotate_x(float amount) {
    kmQuaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, 1, 0, 0);
    kmQuaternionRotationAxis(&rot, &axis, kmDegreesToRadians(amount));
    kmQuaternionMultiply(&rotation_, &rotation(), &rot);
    kmQuaternionNormalize(&rotation_, &rotation_);

    update_from_parent();
    transformation_changed();
}

void Object::rotate_z(float amount) {
    kmQuaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, 0, 0, 1);
    kmQuaternionRotationAxis(&rot, &axis, kmDegreesToRadians(amount));
    kmQuaternionMultiply(&rotation_, &rot, &rotation());
    kmQuaternionNormalize(&rotation_, &rotation_);

    update_from_parent();
    transformation_changed();
}

void Object::rotate_y(float amount) {
    kmQuaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, 0, 1, 0);
    kmQuaternionRotationAxis(&rot, &axis, kmDegreesToRadians(amount));
    kmQuaternionMultiply(&rotation_, &rot, &rotation_);
    kmQuaternionNormalize(&rotation_, &rotation_);

    update_from_parent();
    transformation_changed();
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
        kmQuaternionNormalize(&absolute_orientation_, &absolute_orientation_);
    } else {
        kmVec3Add(&absolute_position_, &parent().absolute_position_, &position_);
        kmQuaternionMultiply(&absolute_orientation_, &parent().absolute_orientation_, &rotation_);
        kmQuaternionNormalize(&absolute_orientation_, &absolute_orientation_);
    }

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
