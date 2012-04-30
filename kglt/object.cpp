#include "kazmath/mat3.h"

#include "object.h"
#include "object_visitor.h"

namespace kglt {

uint64_t Object::object_counter = 0;

Object::~Object() {
    if(has_parent()) {
        parent().detach_child(this);
    }
}

void Object::move_to(float x, float y, float z) {
    position_.x = x;
    position_.y = y;
    position_.z = z;
}

void Object::move_forward(float amount) {
    kmMat3 rot_matrix;
    kmVec3 forward;
    kmMat3RotationQuaternion(&rot_matrix, &rotation_);
    kmMat3GetForwardVec3(&forward, &rot_matrix);
    kmVec3Normalize(&forward, &forward);
    kmVec3Scale(&forward, &forward, amount);
    kmVec3Add(&position_, &position_, &forward);
}

void Object::rotate_y(float amount) {
    kmQuaternion rot;
    kmVec3 v;
    v.x = 0.0f;
    v.y = 1.0f;
    v.z = 0.0f;
    kmQuaternionRotationAxis(&rot, &v, kmDegreesToRadians(amount));
    kmQuaternionMultiply(&rotation_, &rot, &rotation_);
    kmQuaternionNormalize(&rotation_, &rotation_);
}

}
