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
}

void Transform::set_parent(Transform* new_parent) {
    if(parent_ == new_parent) {
        signal_change_attempted();
        return;
    }

    /* When we set the parent, we want to keep our existing position so
         * we update our translation and rotation to be relative to the parent */

    parent_ = new_parent;

    if(parent_) {
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
