#include "transform.h"


namespace smlt {

void Transform::set_position(const Vec3& position) {
    if(has_parent()) {
        Vec3 ppos = parent_->position();
        set_translation(position - ppos);
    } else {
        set_translation(position);
    }
}

void Transform::set_orientation(const Quaternion& orientation) {
    if(has_parent()) {
        auto prot = parent_->orientation();
        prot.inverse();

        rotate((prot * orientation).normalized());
    } else {
        rotate(orientation);
    }
}

void Transform::set_translation(const Vec3 &translation) {
    translation_ = translation;
    signal_change();
}

void Transform::set_rotation(const Quaternion &rotation) {
    rotation_ = rotation;
    signal_change();
}

void Transform::rotate(const Quaternion &q) {
    assert(
        !std::isnan(q.x) &&
        !std::isnan(q.y) &&
        !std::isnan(q.z) &&
        !std::isnan(q.w)
    );

    if(!q.equals(rotation_)) {
        rotation_ = q;
        signal_change();
    }
}

void Transform::rotate(const Vec3& axis, const Degrees& amount) {
    auto q = Quaternion(axis, amount);
    rotate(q);
}

void Transform::rotate(const Degrees& x, const Degrees& y, const Degrees& z) {
    auto q = Quaternion(x, y, z);
    rotate(q);
}

Mat4 Transform::world_space_matrix() const {
    if(!absolute_transformation_is_dirty_) {
        return absolute_transformation_;
    }

    auto c0 = smlt::Vec4(orientation_ * smlt::Vec3(scale_factor_.x, 0, 0), 0);
    auto c1 = smlt::Vec4(orientation_ * smlt::Vec3(0, scale_factor_.y, 0), 0);
    auto c2 = smlt::Vec4(orientation_ * smlt::Vec3(0, 0, scale_factor_.z), 0);

    absolute_transformation_[0] = c0.x;
    absolute_transformation_[1] = c0.y;
    absolute_transformation_[2] = c0.z;
    absolute_transformation_[3] = 0.0f;

    absolute_transformation_[4] = c1.x;
    absolute_transformation_[5] = c1.y;
    absolute_transformation_[6] = c1.z;
    absolute_transformation_[7] = 0.0f;

    absolute_transformation_[8] = c2.x;
    absolute_transformation_[9] = c2.y;
    absolute_transformation_[10] = c2.z;
    absolute_transformation_[11] = 0.0f;

    absolute_transformation_[12] = position_.x;
    absolute_transformation_[13] = position_.y;
    absolute_transformation_[14] = position_.z;
    absolute_transformation_[15] = 1.0f;

    absolute_transformation_is_dirty_ = false;
    return absolute_transformation_;
}

void Transform::set_position_2d(const Vec2 &pos) {
    set_position(Vec3(pos, 0));
}

void Transform::set_translation_2d(const Vec2 &trans) {
    set_translation(Vec3(trans, 0));
}

bool Transform::add_listener(TransformListener* listener) {
    auto it = std::find(listeners_.begin(), listeners_.end(), listener);
    if(it != listeners_.end()) {
        return false;
    }

    listeners_.push_back(listener);
    return true;
}

bool Transform::remove_listener(TransformListener* listener) {
    auto it = std::remove(listeners_.begin(), listeners_.end(), listener);
    if(it == listeners_.end()) {
        return false;
    }

    listeners_.erase(it, listeners_.end());

    return true;
}

void Transform::update_transformation_from_parent() {
    Transform* parent = parent_;

    if(!parent) {
        orientation_ = rotation_;
        position_ = translation_;
    } else {
        auto parent_pos = parent->position();
        auto parent_rot = parent->orientation();

        orientation_ = parent_rot * rotation_;
        position_ = parent_pos + parent_rot * translation_;
    }

    absolute_transformation_is_dirty_ = true;
}

void Transform::sync(const Transform *other) {
    set_position(other->position());
    set_orientation(other->orientation());
    set_scale_factor(other->scale_factor());
}

void Transform::look_at(const Vec3& target, const Vec3& up) {
    set_orientation(
        Quaternion::look_rotation(
            (target - position_).normalized(),
            up
        )
    );
}

void Transform::set_parent(Transform* new_parent, TransformRetainMode retain_mode) {
    if(parent_ == new_parent) {
        signal_change_attempted();
        return;
    }

    /* When we set the parent, we want to keep our existing position so
         * we update our translation and rotation to be relative to the parent */

    parent_ = new_parent;

    if(parent_ && retain_mode == TRANSFORM_RETAIN_MODE_KEEP) {
        translation_ = (position_ - new_parent->position_);
        rotation_ = (orientation_ * new_parent->orientation_.inversed());
    }

    signal_change();
}

void Transform::signal_change_attempted() {
    for(auto& listener: listeners_) {
        listener->on_transformation_change_attempted();
    }
}

void Transform::signal_change() {
    update_transformation_from_parent();
    for(auto& listener: listeners_) {
        listener->on_transformation_changed();
    }
}


}
