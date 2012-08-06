#include <algorithm>
#include <functional>

#include "kazmath/mat3.h"

#include "scene.h"
#include "object.h"
#include "object_visitor.h"

namespace kglt {

uint64_t Object::object_counter = 0;

Object::Object(Scene *parent_scene):
    id_(++object_counter),
    scene_(parent_scene),
    is_visible_(true) {

    kmVec3Fill(&position_, 0.0, 0.0, 0.0);
    kmQuaternionIdentity(&rotation_);

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
}

void Object::rotate_x(float amount) {
    kmQuaternion rot;
    rot.x = 1.0f;
    rot.y = 0.0f;
    rot.z = 0.0f;
    rot.w = amount * kmPIOver180;
    kmQuaternionMultiply(&rotation(), &rotation(), &rot);

    update_from_parent();
}

void Object::rotate_z(float amount) {
    kmQuaternion rot;
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 1.0f;
    rot.w = amount * kmPIOver180;
    kmQuaternionMultiply(&rotation(), &rot, &rotation());

    update_from_parent();
}

void Object::rotate_y(float amount) {
    kmQuaternion rot;

	rot.w = cos(amount / 2);
	rot.x = 0.0;
	rot.y = sin(amount / 2);
	rot.z = 0.0;
	
	kmQuaternionNormalize(&rot, &rot);
	
    kmQuaternionMultiply(&rotation_, &rot, &rotation_);
    kmQuaternionNormalize(&rotation_, &rotation_);

    update_from_parent();
}

}
