#include "kazmath/mat3.h"

#include "scene.h"
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

void Object::rotate_x(float amount) {
    kmQuaternion rot;
    rot.x = 1.0f;
    rot.y = 0.0f;
    rot.z = 0.0f;
    rot.w = amount * kmPIOver180;
    kmQuaternionMultiply(&rotation(), &rotation(), &rot);
}

void Object::rotate_z(float amount) {
    kmQuaternion rot;
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 1.0f;
    rot.w = amount * kmPIOver180;
    kmQuaternionMultiply(&rotation(), &rot, &rotation());
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
}

Scene& Object::scene() { 
	Scene* scene = root_as<Scene>(); 
	assert(scene);
	return *scene;
}

}
