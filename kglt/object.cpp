#include <algorithm>
#include <functional>

#include "kazmath/mat3.h"
#include "stage.h"
#include "object.h"
#include "camera.h"
#include "utils/ownable.h"
#include "physics/physics_engine.h"

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
    parent_changed_connection_ = signal_parent_changed().connect(std::bind(&Object::parent_changed_callback, this, std::placeholders::_1, std::placeholders::_2));
}

Object::~Object() {
    parent_changed_connection_.disconnect();
}

void Object::_update_constraint() {
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

void Object::constrain_to(const Vec3 &min, const Vec3 &max) {
    constraint_.reset(new std::pair<Vec3, Vec3>(min, max));
}

void Object::constrain_to(const AABB& box) {
    constrain_to(box.min, box.max);
}

std::pair<Vec3, Vec3> Object::constraint() const {
    if(!is_constrained()) {
        throw LogicError("Tried to get constraint on unconstrained camera");
    }

    return *constraint_;
}

bool Object::is_constrained() const {
    return bool(constraint_);
}

void Object::disable_constraint() {
    constraint_.reset();
}


void Object::parent_changed_callback(GenericTreeNode *old_parent, GenericTreeNode *new_parent) {
    Object* new_p = dynamic_cast<Object*>(new_parent);

    if(!new_p) {
        return;
    }

    if(new_p->is_responsive() != is_responsive()) {
        throw std::logic_error("Tried to connect a responsive object to a non-responsive object");
    }

    if(is_responsive()) {
        if(responsive_parental_constraint_) {
            body().destroy_constraint(responsive_parental_constraint_);
            responsive_parental_constraint_ = ConstraintID();
        }

        body().set_position(new_p->body().position());
        body().set_rotation(new_p->body().rotation());

        responsive_parental_constraint_ = body().create_fixed_constraint(new_p->body());
    }

    update_from_parent();
}

void Object::make_responsive() {
    if(!stage()->window().has_physics_engine()) {
        throw std::logic_error("Tried to make an object responsive when no physics engine is enabled");
    }

    auto engine = stage()->window().physics();

    responsive_body_ = engine->new_responsive_body(this);

    signal_made_responsive_();
}

void Object::make_collidable() {
    if(!stage()->window().has_physics_engine()) {
        throw std::logic_error("Tried to make an object collidable when no physics engine is enabled");
    }

    auto engine = stage()->window().physics();

    collidable_ = engine->new_collidable(this);

    signal_made_collidable_();
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

    if(is_responsive()) {
        if(responsive_parental_constraint_) {
            //Destroy the existing constraint
            body().destroy_constraint(responsive_parental_constraint_);
        }

        //Set the new absolute position
        body().set_position(kglt::Vec3(x, y, z));

        if(has_parent() && !parent_is_root()) {
            //Recreate the constraint with the parent (as long as the parent isn't the stage itself)
            responsive_parental_constraint_ = body().create_fixed_constraint(parent()->as<Object>()->body());
        }
    } else {
        kglt::Vec3 parent_pos;
        if(has_parent()) {
            parent_pos = parent()->as<SceneNode>()->position();
        }

        set_relative_position(kglt::Vec3(x, y, z) - parent_pos);
    }
}

void Object::set_relative_position(float x, float y, float z) {
    //Always store the relative_position_ even for responsive bodies
    //as they only deal with absolute
    relative_position_ = Vec3(x, y, z);

    if(is_responsive()) {
        if(responsive_parental_constraint_) {
            //Destroy the existing constraint
            body().destroy_constraint(responsive_parental_constraint_);
        }

        //Set the new absolute position
        body().set_position(parent()->as<SceneNode>()->position() + Vec3(x, y, z));

        if(has_parent() && !parent_is_root()) {
            //Recreate the constraint with the parent (as long as the parent isn't the stage itself)
            responsive_parental_constraint_ = body().create_fixed_constraint(parent()->as<Object>()->body());
        }
    } else {
        update_from_parent();
    }
}

kglt::Vec3 Object::absolute_position() const {
    if(is_responsive()) {
        return body().position();
    }

    return absolute_position_;
}

kglt::Vec3 Object::relative_position() const {
    if(is_responsive()) {
        return absolute_position() - parent()->as<SceneNode>()->position();
    }

    return relative_position_;
}

kglt::Quaternion Object::absolute_rotation() const {
    if(is_responsive()) {
        return body().rotation();
    } else {
        return absolute_rotation_;
    }
}

kglt::Quaternion Object::relative_rotation() const {
    if(is_responsive()) {
        Quaternion parent_rot = parent()->as<SceneNode>()->rotation();
        parent_rot.inverse();
        return parent_rot * body().rotation();
    }

    return relative_rotation_;
}


void Object::set_absolute_rotation(const Quaternion& quat) {
    if(rotation_locked_) {
        return;
    }

    if(is_responsive()) {
        if(responsive_parental_constraint_) {
            //Destroy the existing constraint
            body().destroy_constraint(responsive_parental_constraint_);
            responsive_parental_constraint_ = ConstraintID();
        }

        //Set the new rotation
        body().set_rotation(quat);

        if(has_parent() && !parent_is_root()) {
            //Recreate the constraint with the parent (as long as the parent isn't the stage itself)
            responsive_parental_constraint_ = body().create_fixed_constraint(parent()->as<Object>()->body());
        }
    } else {
        kglt::Quaternion parent_rot;
        kmQuaternionIdentity(&parent_rot);

        if(has_parent()) {
            parent_rot = parent()->as<SceneNode>()->rotation();
        }

        parent_rot.inverse();
        set_relative_rotation(parent_rot * quat);
    }

    _update_constraint();
}

void Object::set_relative_rotation(const Quaternion &quaternion) {
    //Always store the relative rotation, even for responsive bodies
    //as they only deal with absolute
    relative_rotation_ = quaternion;
    relative_rotation_.normalize();

    if(is_responsive()) {
        if(responsive_parental_constraint_) {
            //Destroy the existing constraint
            body().destroy_constraint(responsive_parental_constraint_);
        }

        //Set the new rotation
        body().set_rotation(parent()->as<SceneNode>()->rotation() * quaternion);

        if(has_parent() && !parent_is_root()) {
            //Recreate the constraint with the parent (as long as the parent isn't the stage itself)
            responsive_parental_constraint_ = body().create_fixed_constraint(parent()->as<Object>()->body());
        }
    } else {
        update_from_parent();
    }
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

void Object::look_at(const Vec3& position) {
    Vec3 forward = (position - absolute_position()).normalized();

    float dot = Vec3(0, 0, -1).dot(forward);

    if(fabs(dot - (-1.0)) < 0.00000001f) {
        //Rotate 180 degrees around up
        rotate_absolute_y(180.0);
        return;
    }

    if(fabs(dot - 1.0) < 0.00000001f) {
        return;
    }

    float rot_angle = acosf(dot);
    Vec3 rot_axis = Vec3(0, 0, -1).cross(forward).normalized();

    Quaternion q;
    kmQuaternionRotationAxisAngle(&q, &rot_axis, rot_angle);

    set_absolute_rotation(q);
}


Mat4 Object::absolute_transformation() const {
    Mat4 rot_matrix, trans_matrix, final;

    Quaternion abs_rot = absolute_rotation();

    kmMat4RotationQuaternion(&rot_matrix, &abs_rot);
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
    }

    apply_recursively([](GenericTreeNode* x) {
        x->as<Object>()->update_from_parent();
    }, false);
}

void Object::destroy_children() {
    auto childs = children();

    detach_children();

    for(auto child: childs) {
        child->apply_recursively(&ownable_tree_node_destroy);
    }
}

}
