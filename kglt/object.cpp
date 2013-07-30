#include <algorithm>
#include <functional>

#include "kazmath/mat3.h"
#include "stage.h"
#include "scene.h"
#include "object.h"
#include "object_visitor.h"
#include "camera.h"

namespace kglt {

uint64_t Object::object_counter = 0;

Object::Object(Stage *stage):
    uuid_(++object_counter),
    stage_(stage),
    is_visible_(true),
    rotation_locked_(false),
    position_locked_(false) {

    update_from_parent();

    //When the parent changes, update the position/orientation
    parent_changed_connection_ = signal_parent_changed().connect(sigc::mem_fun(this, &Object::parent_changed_callback));
}

Object::~Object() {
    parent_changed_connection_.disconnect();
}

void Object::attach_to_camera(CameraID cam) {
    set_parent(stage_->scene().camera(cam));
}

void Object::lock_rotation() {
    rotation_locked_ = true;
}

void Object::unlock_rotation() {
    rotation_locked_ = false;
    update_from_parent();
}

void Object::lock_position() {
    position_locked_ = true;
}

void Object::unlock_position() {
    position_locked_ = false;
    update_from_parent();
}

/**
 * @brief Object::set_absolute_position
 *
 * Sets the absolute position of this object, if the object has a parent
 * then set the relative position required to absolutely position this object
 *
 * @param x
 * @param y
 * @param z
 */
void Object::set_absolute_position(float x, float y, float z) {
    if(position_locked_) {
        return;
    }

    kglt::Vec3 parent_pos;
    if(has_parent()) {
        parent_pos = parent().absolute_position();

    }

    set_relative_position(kglt::Vec3(x, y, z) - parent_pos);
}

void Object::set_absolute_rotation(const Quaternion& quat) {
    if(rotation_locked_) {
        return;
    }

    kglt::Quaternion parent_rot;
    kmQuaternionIdentity(&parent_rot);

    if(has_parent()) {
        parent_rot = parent().absolute_rotation();
    }

    parent_rot.inverse();
    set_relative_rotation(parent_rot * quat);
}

void Object::set_relative_position(float x, float y, float z) {
    relative_position_ = Vec3(x, y, z);
    update_from_parent();
}

void Object::set_relative_rotation(const Quaternion &quaternion) {
    relative_rotation_ = quaternion;
    relative_rotation_.normalize();

    update_from_parent();
}

void Object::set_absolute_rotation(const Degrees &angle, float x, float y, float z) {
    if(rotation_locked_) {
        return;
    }

    kglt::Quaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, x, y, z);    
    kmQuaternionRotationAxisAngle(&rot, &axis, kmDegreesToRadians(angle.value_));

    set_absolute_rotation(rot);
}

void Object::move_forward(float amount) {
    set_absolute_position(
        absolute_position() + (forward() * amount)
    );
}

void Object::rotate_absolute_x(float amount) {
    if(rotation_locked_) {
        return;
    }

    if(fabs(amount) < kmEpsilon) {
        return;
    }

    Quaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, 1, 0, 0);
    kmQuaternionRotationAxisAngle(&rot, &axis, kmDegreesToRadians(amount));

    set_absolute_rotation(absolute_rotation() * rot);

    update_from_parent();
}

void Object::rotate_absolute_z(float amount) {
    if(rotation_locked_) {
        return;
    }

    if(fabs(amount) < kmEpsilon) {
        return;
    }

    Quaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, 0, 0, 1);
    kmQuaternionRotationAxisAngle(&rot, &axis, kmDegreesToRadians(amount));

    set_absolute_rotation(absolute_rotation() * rot);
    absolute_rotation_.normalize();

    update_from_parent();
}

void Object::rotate_absolute_y(float amount) {
    if(rotation_locked_) {
        return;
    }

    if(fabs(amount) < kmEpsilon) {
        return;
    }

    Quaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, 0, 1, 0);
    kmQuaternionRotationAxisAngle(&rot, &axis, kmDegreesToRadians(amount));

    set_absolute_rotation(absolute_rotation() * rot);
    absolute_rotation_.normalize();

    update_from_parent();
}

Mat4 Object::absolute_transformation() const {
    Mat4 rot_matrix, trans_matrix, final;

    kmMat4RotationQuaternion(&rot_matrix, &absolute_rotation_);
    kmMat4Translation(&trans_matrix, absolute_position().x, absolute_position().y, absolute_position().z);

    kmMat4Multiply(&final, &trans_matrix, &rot_matrix);
    return final;
}

void Object::update_from_parent() {
    Vec3 orig_pos = absolute_position();
    Quaternion orig_rot = absolute_rotation();

    if(!has_parent()) {
        absolute_position_ = relative_position();
        absolute_rotation_ = relative_rotation();
    } else {
        if(!position_locked_) {
            absolute_position_ = parent().absolute_position() + relative_position();
        }
        if(!rotation_locked_) {
            absolute_rotation_ = relative_rotation() * parent().absolute_rotation();
            absolute_rotation_.normalize();
        }
    }

    //Only signal that the transformation changed if it did
    if(orig_pos != absolute_position() || orig_rot != absolute_rotation()) {
        transformation_changed();
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
