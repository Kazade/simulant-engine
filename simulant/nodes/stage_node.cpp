#include "../stage.h"
#include "camera_proxy.h"

namespace smlt {

StageNode::StageNode(Stage *stage):
    stage_(stage) {

}

void StageNode::cleanup() {
    remove_from_parent(); // Make sure we're detached from the scene

    // Go through the subnodes and ask each for destruction in-turn
    each_descendent_lf([](uint32_t, TreeNode* node) {
        StageNode* stage_node = dynamic_cast<StageNode*>(node);
        assert(stage_node);

        stage_node->detach();
        stage_node->ask_owner_for_destruction();        
    });

    detach(); // Make sure we're not connected to anything
}

void StageNode::set_parent(CameraID id) {
    TreeNode::set_parent(stage_->camera(id));
}

Vec3 StageNode::absolute_position() const {
    return absolute_position_;
}

Quaternion StageNode::absolute_rotation() const {
    return absolute_rotation_;
}

Vec3 StageNode::absolute_scaling() const {
    return absolute_scale_;
}

Mat4 StageNode::absolute_transformation() const {
    Mat4 final(absolute_rotation_);

    final[0] *= absolute_scale_.x;
    final[5] *= absolute_scale_.y;
    final[10] *= absolute_scale_.z;
    final[15] = 1.0;

    final[12] = absolute_position_.x;
    final[13] = absolute_position_.y;
    final[14] = absolute_position_.z;

    return final;
}

void StageNode::move_to_absolute(const Vec3& position) {
    if(!parent_is_stage()) {
        // The stage itself is immovable so, we only bother with this if this isn't he stage
        // could also use if(!parent()->is_root())
        Vec3 ppos = static_cast<StageNode*>(parent())->absolute_position();
        move_to(position - ppos);
    } else {
        move_to(position);
    }
}

void StageNode::move_to_absolute(float x, float y, float z) {
    move_to_absolute(Vec3(x, y, z));
}

void StageNode::rotate_to_absolute(const Quaternion& rotation) {
    if(!parent_is_stage()) {
        auto prot = static_cast<StageNode*>(parent())->absolute_rotation();
        prot.inverse();

        rotate_to((prot * rotation).normalized());
    } else {
        rotate_to(rotation);
    }
}

void StageNode::rotate_to_absolute(const Degrees& degrees, float x, float y, float z) {
    rotate_to_absolute(Quaternion(Vec3(x, y, z), degrees));
}

void StageNode::on_position_set(const Vec3& oldp, const Vec3& newp) {
    update_position_from_parent();
}

void StageNode::on_rotation_set(const Quaternion& oldr, const Quaternion& newr) {
    update_rotation_from_parent();
}

void StageNode::on_scaling_set(const Vec3& olds, const Vec3& news) {
    update_scaling_from_parent();
}

void StageNode::update_rotation_from_parent() {
    if(rotation_locked()) {
        return;
    }

    StageNode* parent = static_cast<StageNode*>(this->parent());

    Quaternion prot;
    if(parent) {
        prot = parent->absolute_rotation();
        update_position_from_parent();
    }

    absolute_rotation_ = prot * rotation();
    absolute_rotation_.normalize();

    recalc_bounds();

    each_child([](uint32_t, TreeNode* child) {
        StageNode* node = static_cast<StageNode*>(child);
        node->update_rotation_from_parent();
    });
}

void StageNode::update_position_from_parent() {
    StageNode* parent = static_cast<StageNode*>(this->parent());

    Vec3 ppos;
    Vec3 rel = position();

    if(parent) {
        ppos = parent->absolute_position();
        rel = parent->absolute_rotation().rotate_vector(rel);
    }

    absolute_position_ = ppos + rel;

    recalc_bounds();

    each_child([](uint32_t, TreeNode* child) {
        StageNode* node = static_cast<StageNode*>(child);
        node->update_position_from_parent();
    });
}

void StageNode::update_scaling_from_parent() {
    StageNode* parent = static_cast<StageNode*>(this->parent());

    Vec3 pscale(1, 1, 1);
    if(parent) {
        pscale = parent->scale();
    }

    absolute_scale_ = pscale * scale();

    recalc_bounds();

    each_child([](uint32_t, TreeNode* child) {
        StageNode* node = static_cast<StageNode*>(child);
        node->update_scaling_from_parent();
    });
}

void StageNode::on_parent_set(TreeNode* oldp, TreeNode* newp) {
    update_position_from_parent();
    update_rotation_from_parent();
    update_scaling_from_parent();
}

const AABB StageNode::transformed_aabb() const {
    auto corners = aabb().corners();
    auto transform = absolute_transformation();

    for(auto& corner: corners) {
        corner = corner.transformed_by(transform);
    }

    return AABB(corners.data(), corners.size());
}

void StageNode::recalc_bounds() {
    auto newb = transformed_aabb();
    if(newb.min != bounds_.min || newb.max != bounds_.max) {
        bounds_ = newb;
        signal_bounds_updated_(bounds_);
    }
}


void StageNode::update(float dt) {
    update_controllers(dt);
}

void StageNode::late_update(float dt) {
    late_update_controllers(dt);
}

void StageNode::fixed_update(float step) {
    fixed_update_controllers(step);
}

}
