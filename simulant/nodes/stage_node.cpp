#include "../stage.h"
#include "camera.h"

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

void StageNode::on_transformation_changed() {
    update_transformation_from_parent();
}

void StageNode::update_transformation_from_parent() {
    StageNode* parent = static_cast<StageNode*>(this->parent());

    if(!parent || parent == stage_) {
        absolute_rotation_ = rotation();
        absolute_position_ = position();
        absolute_scale_ = scale();
    } else {
        auto parent_pos = parent->absolute_position();
        auto parent_rot = parent->absolute_rotation();
        auto parent_scale = parent->absolute_scaling();

        absolute_rotation_ = parent_rot * rotation();
        absolute_position_ = parent_pos + parent_rot.rotate_vector(position());
        absolute_scale_ = parent_scale * scale();
    }

    recalc_bounds();

    each_child([](uint32_t, TreeNode* child) {
        StageNode* node = static_cast<StageNode*>(child);
        node->update_transformation_from_parent();
    });
}

void StageNode::on_parent_set(TreeNode* oldp, TreeNode* newp) {
    update_transformation_from_parent();
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
    if(newb.min() != transformed_aabb_.min() || newb.max() != transformed_aabb_.max()) {
        transformed_aabb_ = newb;
        signal_bounds_updated_(transformed_aabb_);
    }
}


void StageNode::update(float dt) {
    update_behaviours(dt);
}

void StageNode::late_update(float dt) {
    late_update_behaviours(dt);
}

void StageNode::fixed_update(float step) {
    fixed_update_behaviours(step);
}

}
