#include <algorithm>
#include <functional>

#include "kazmath/mat3.h"
#include "stage.h"
#include "object.h"
#include "camera.h"
#include "utils/ownable.h"

namespace kglt {

uint64_t Object::object_counter = 0;

MoveableObject::MoveableObject(Stage *stage):
    Object(stage),
    is_visible_(true),
    rotation_locked_(false),
    position_locked_(false) {

    update_from_parent();

    //When the parent changes, update the position/orientation
    parent_changed_connection_ = signal_parent_changed().connect(std::bind(&MoveableObject::parent_changed_callback, this, std::placeholders::_1, std::placeholders::_2));
}

MoveableObject::~MoveableObject() {
    parent_changed_connection_.disconnect();
}

void MoveableObject::_update_constraint() {
    if(!is_constrained()) {
        return;
    }

    Vec3 new_position = absolute_position();

    if(new_position.x < constraint_->first.x) new_position.x = constraint_->first.x;
    if(new_position.y < constraint_->first.y) new_position.y = constraint_->first.y;
    if(new_position.z < constraint_->first.z) new_position.z = constraint_->first.z;

    if(new_position.x > constraint_->second.x) new_position.x = constraint_->second.x;
    if(new_position.y > constraint_->second.y) new_position.y = constraint_->second.y;
    if(new_position.z > constraint_->second.z) new_position.z = constraint_->second.z;

    if(new_position != absolute_position()) {
        move_to(new_position);
    }
}

void MoveableObject::constrain_to(const Vec3 &min, const Vec3 &max) {
    constraint_.reset(new std::pair<Vec3, Vec3>(min, max));
}

void MoveableObject::constrain_to(const AABB& box) {
    constrain_to(box.min, box.max);
}

std::pair<Vec3, Vec3> MoveableObject::constraint() const {
    if(!is_constrained()) {
        throw LogicError("Tried to get constraint on unconstrained camera");
    }

    return *constraint_;
}

bool MoveableObject::is_constrained() const {
    return bool(constraint_);
}

void MoveableObject::disable_constraint() {
    constraint_.reset();
}


void MoveableObject::parent_changed_callback(GenericTreeNode *old_parent, GenericTreeNode *new_parent) {
    MoveableObject* new_p = dynamic_cast<MoveableObject*>(new_parent);

    if(!new_p) {
        return;
    }

    update_from_parent();
}

void MoveableObject::lock_rotation() {
    rotation_locked_ = true;
}

void MoveableObject::unlock_rotation() {
    rotation_locked_ = false;
    update_from_parent();
}

void MoveableObject::lock_position() {
    position_locked_ = true;
}

void MoveableObject::unlock_position() {
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
void MoveableObject::set_absolute_position(float x, float y, float z) {
    if(position_locked_) {
        return;
    }

    kglt::Vec3 parent_pos;
    if(has_parent()) {
        parent_pos = parent()->as<SceneNode>()->position();
    }

    set_relative_position(kglt::Vec3(x, y, z) - parent_pos);
}

void MoveableObject::set_relative_position(float x, float y, float z) {
    //Always store the relative_position_ even for responsive bodies
    //as they only deal with absolute
    relative_position_ = Vec3(x, y, z);
    update_from_parent();
}

void MoveableObject::set_absolute_rotation(const Quaternion& quat) {
    if(rotation_locked_) {
        return;
    }

    kglt::Quaternion parent_rot;
    kmQuaternionIdentity(&parent_rot);

    if(has_parent()) {
        parent_rot = parent()->as<SceneNode>()->rotation();
    }

    parent_rot.inverse();
    set_relative_rotation(parent_rot * quat);
    _update_constraint();
}

void MoveableObject::set_relative_rotation(const Quaternion &quaternion) {
    //Always store the relative rotation, even for responsive bodies
    //as they only deal with absolute
    relative_rotation_ = quaternion;
    relative_rotation_.normalize();
    update_from_parent();
}

void MoveableObject::set_absolute_rotation(const Degrees &angle, float x, float y, float z) {
    if(rotation_locked_) {
        return;
    }

    kglt::Quaternion rot;
    kmVec3 axis;
    kmVec3Fill(&axis, x, y, z);
    kmQuaternionRotationAxisAngle(&rot, &axis, kmDegreesToRadians(angle.value_));

    set_absolute_rotation(rot);
}

void MoveableObject::move_forward(float amount) {
    set_absolute_position(
        absolute_position() + (forward() * amount)
    );
}

void MoveableObject::rotate_around(const kglt::Vec3& axis, const kglt::Degrees& degrees) {
    if(rotation_locked_) {
        return;
    }

    float amount = degrees.value_;
    if(fabs(amount) < kmEpsilon) {
        return;
    }

    Quaternion rot;
    kmQuaternionRotationAxisAngle(&rot, &axis, kmDegreesToRadians(amount));

    set_absolute_rotation(rot * absolute_rotation());
    absolute_rotation_.normalize();
    update_from_parent();
}

void MoveableObject::look_at(const Vec3& position) {
    Vec3 forward = (position - absolute_position()).normalized();

    float dot = Vec3(0, 0, -1).dot(forward);

    if(fabs(dot - (-1.0)) < kmEpsilon) {
        //Rotate 180 degrees around up
        rotate_global_y(Degrees(180.0));
        return;
    }

    if(fabs(dot - 1.0) < kmEpsilon) {
        return;
    }

    float rot_angle = acosf(dot);
    Vec3 rot_axis = Vec3(0, 0, -1).cross(forward).normalized();

    Quaternion q;
    kmQuaternionRotationAxisAngle(&q, &rot_axis, rot_angle);

    set_absolute_rotation(q);
}


Mat4 MoveableObject::absolute_transformation() const {
    Mat4 rot_matrix, trans_matrix, final;

    Quaternion abs_rot = absolute_rotation();

    kmMat4RotationQuaternion(&rot_matrix, &abs_rot);
    kmMat4Translation(&trans_matrix, absolute_position().x, absolute_position().y, absolute_position().z);

    kmMat4Multiply(&final, &trans_matrix, &rot_matrix);
    return final;
}

void MoveableObject::update_from_parent() {
    Vec3 orig_pos = absolute_position();
    Quaternion orig_rot = absolute_rotation();

    if(!has_parent()) {
        absolute_position_ = relative_position();
        absolute_rotation_ = relative_rotation();
    } else {
        if(!position_locked_) {
            absolute_position_ = parent()->as<SceneNode>()->position() + relative_position();
        }
        if(!rotation_locked_) {
            absolute_rotation_ = relative_rotation() * parent()->as<SceneNode>()->rotation();
            absolute_rotation_.normalize();
        }
    }

    //Only signal that the transformation changed if it did
    if(orig_pos != absolute_position() || orig_rot != absolute_rotation()) {
        transformation_changed();
        signal_transformation_changed_(absolute_position(), absolute_rotation());
    }

    assert(!isnan(absolute_position_.x));
    assert(!isnan(absolute_position_.y));
    assert(!isnan(absolute_position_.z));

    apply_recursively([](GenericTreeNode* x) {
        x->as<MoveableObject>()->update_from_parent();
    }, false);
}

void MoveableObject::destroy_children() {
    auto childs = children();

    detach_children();

    for(auto child: childs) {
        child->apply_recursively(&ownable_tree_node_destroy);
    }
}

}
