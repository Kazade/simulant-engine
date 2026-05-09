#include "transform.h"

namespace smlt {

void Transform::set_translation_if_necessary(const Vec3& trans) {
    if(!translation_.equals(trans)) {
        translation_ = trans;
        signal_change();
    }
}

void Transform::set_rotation_if_necessary(const Quaternion& rot) {
    if(!rotation_.equals(rot)) {
        rotation_ = rot;
        signal_change();
    }
}

void Transform::set_scale_factor_if_necessary(const Vec3& scale) {
    if(!scale_factor_.equals(scale)) {
        scale_factor_ = scale;
        signal_change();
    }
}

void Transform::set_position(const Vec3& position) {
    signal_change_attempted();
    if(has_parent()) {
        Vec3 ppos = parent_->position();
        set_translation_if_necessary(position - ppos);
    } else {
        set_translation_if_necessary(position);
    }
}

void Transform::set_orientation(const Quaternion& orientation) {
    signal_change_attempted();
    if(has_parent()) {
        auto prot = parent_->orientation();
        prot.inverse();

        set_rotation_if_necessary((prot * orientation).normalized());
    } else {
        set_rotation_if_necessary(orientation);
    }
}

void Transform::set_scale(const Vec3& scale) {
    signal_change_attempted();
    if(has_parent()) {
        /* World scale = parent_scale * scale_factor
         * So scale_factor = world_scale / parent_scale */
        auto parent_scale = parent_->scale();
        /* Protect against division by zero */
        Vec3 divisor = parent_scale;
        if(divisor.x == 0.0f) divisor.x = 1.0f;
        if(divisor.y == 0.0f) divisor.y = 1.0f;
        if(divisor.z == 0.0f) divisor.z = 1.0f;
        set_scale_factor_if_necessary(scale / divisor);
    } else {
        set_scale_factor_if_necessary(scale);
    }
}

void Transform::set_translation(const Vec3& translation) {
    signal_change_attempted();
    set_translation_if_necessary(translation);
}

void Transform::set_rotation(const Quaternion& rotation) {
    signal_change_attempted();
    set_rotation_if_necessary(rotation);
}

void Transform::set_scale_factor(const Vec3& scale) {
    signal_change_attempted();
    set_scale_factor_if_necessary(scale);
}

void Transform::rotate(const Quaternion& q) {
    signal_change_attempted();

    assert(!std::isnan(q.x) && !std::isnan(q.y) && !std::isnan(q.z) &&
           !std::isnan(q.w));

    set_rotation_if_necessary(q * rotation_);
}

void Transform::rotate(const Vec3& axis, const Degrees& amount) {
    auto q = Quaternion(axis, amount);
    rotate(q);
}

void Transform::rotate(const Degrees& x, const Degrees& y, const Degrees& z) {
    auto q = Quaternion(x, y, z);
    rotate(q);
}

const Mat4& Transform::world_space_matrix() const {
    _ensure_clean();
    if(absolute_transformation_is_dirty_) {
        absolute_transformation_ =
            smlt::Mat4::as_transform(position_, orientation_, scale_);
        absolute_transformation_is_dirty_ = false;
    }
    return absolute_transformation_;
}

void Transform::set_position_2d(const Vec2& pos) {
    set_position(Vec3(pos, 0));
}

void Transform::set_translation_2d(const Vec2& trans) {
    set_translation(Vec3(trans, 0));
}

Vec2 Transform::position_2d() const {
    _ensure_clean();
    return position_.xy();
}

Vec2 Transform::translation_2d() const {
    return translation_.xy();
}

smlt::Degrees Transform::orientation_2d() const {
    _ensure_clean();
    // In 2D (X-Y plane) the orientation is the roll — rotation around Z.
    return orientation_.roll();
}

smlt::Degrees Transform::rotation_2d() const {
    return rotation_.roll();
}

void Transform::set_orientation_2d(const Degrees& orientation) {
    set_orientation(Quaternion(Vec3(0.0f, 0.0f, 1.0f), orientation));
}

void Transform::set_rotation_2d(const Degrees& rotation) {
    set_rotation(Quaternion(Vec3(0.0f, 0.0f, 1.0f), rotation));
}

void Transform::set_scale_factor_2d(const Vec2& xy) {
    set_scale_factor(Vec3(xy.x, xy.y, scale_factor_.z));
}

void Transform::translate_2d(const Vec2& xy) {
    translate(Vec3(xy, 0.0f));
}

void Transform::rotate_2d(const smlt::Degrees& rot) {
    rotate(Vec3(0.0f, 0.0f, 1.0f), rot);
}

void Transform::scale_by(const Vec3& v) {
    signal_change_attempted();
    set_scale_factor_if_necessary(scale_factor_ * v);
}

void Transform::scale_by(float xyz) {
    scale_by(Vec3(xyz));
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

void Transform::update_transformation_from_parent() const {
    Transform* parent = parent_;

    if(!parent) {
        orientation_ = rotation_;
        position_ = translation_;
        scale_ = scale_factor_;
        last_parent_gen_ = 0;
    } else {
        /* _ensure_clean() already resolved the parent before calling us,
         * so reading parent->position_ etc. directly avoids a redundant
         * recursive _ensure_clean() call through the accessor. */
        const Vec3& parent_pos = parent->position_;
        const Quaternion& parent_rot = parent->orientation_;
        const Vec3& parent_scale = parent->scale_;

        orientation_ = parent_rot * rotation_;
        scale_ = parent_scale * scale_factor_;
        position_ = parent_pos + (parent_rot * (translation_ * parent_scale));
        last_parent_gen_ = parent->generation_;
    }

    generation_++;
    absolute_transformation_is_dirty_ = true;
}

void Transform::_ensure_clean() const {
    if(!parent_) return;
    if(parent_->generation_ == last_parent_gen_) return;
    parent_->_ensure_clean();
    update_transformation_from_parent();
}

void Transform::sync(const Transform* other) {
    set_position(other->position());
    set_orientation(other->orientation());
    set_scale_factor(other->scale_factor());
}

void Transform::look_at(const Vec3& target, const Vec3& up) {
    set_orientation(
        Quaternion::look_rotation((target - position()).normalized(), up));
}

void Transform::set_parent(Transform* new_parent,
                           TransformRetainMode retain_mode) {
    signal_change_attempted();

    if(parent_ == new_parent) {
        return;
    }

    /* When we set the parent, we want to keep our existing position so
     * we update our translation and rotation to be relative to the parent */

    parent_ = new_parent;

    if(parent_ && retain_mode == TRANSFORM_RETAIN_MODE_KEEP) {
        /* Use lazy accessors so stale parent/self transforms are resolved
         * before we compute the new local offsets. */
        translation_ = (position() - new_parent->position());
        rotation_ = (orientation() * new_parent->orientation().inversed());
        /* To keep our world scale after parent change, compute new scale_factor_
         * such that: scale_ = parent_scale * scale_factor_
         * therefore:  scale_factor_ = scale_ / parent_scale */
        Vec3 parent_scale = new_parent->scale();
        /* Protect against division by zero which causes inf */
        if(parent_scale.x == 0.0f) parent_scale.x = 1.0f;
        if(parent_scale.y == 0.0f) parent_scale.y = 1.0f;
        if(parent_scale.z == 0.0f) parent_scale.z = 1.0f;
        scale_factor_ = scale() / parent_scale;
    }

    signal_change();
}

void Transform::signal_change_attempted() {
    for(auto& listener: listeners_) {
        listener->on_transformation_change_attempted();
    }
}

void Transform::signal_change() {
    /* Ensure ancestor chain is resolved before computing our own world values,
     * in case an ancestor was also recently moved without being propagated. */
    if(parent_) parent_->_ensure_clean();
    update_transformation_from_parent();
    for(auto& listener: listeners_) {
        listener->on_transformation_changed();
    }
}

} // namespace smlt
