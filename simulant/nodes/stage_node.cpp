#include "../stage.h"
#include "camera.h"

namespace smlt {

StageNode::StageNode(Stage *stage):
    TreeNode(),
    stage_(stage) {

}

StageNode::~StageNode() {

}

void StageNode::clean_up() {
    signal_cleaned_up_(); // Tell everyone we're going

    remove_from_parent(); // Make sure we're not connected to anything

    // Go through the subnodes and ask each for destruction in-turn
    std::vector<StageNode*> to_destroy;
    for(auto stage_node: each_child()) {
        assert(stage_node);
        to_destroy.push_back(stage_node);
    };

    for(auto stage_node: to_destroy) {
        // Don't wait for the next frame, just destroy now
        stage_node->destroy_immediately();
    }

    TwoPhaseConstructed::clean_up();
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
    Mat4 scale;
    Mat4 trans;
    Mat4 rot(absolute_rotation_);

    scale[0] = absolute_scale_.x;
    scale[5] = absolute_scale_.y;
    scale[10] = absolute_scale_.z;

    trans[12] = absolute_position_.x;
    trans[13] = absolute_position_.y;
    trans[14] = absolute_position_.z;

    return trans * rot * scale;
}

bool StageNode::is_visible() const {
    // Debug check to make sure the parent is always a stage node. If we have a parent
    // the dynamic cast should return something truthy, if there's no parent just return true
    assert(parent() ? (bool) dynamic_cast<StageNode*>(parent()) : true);

    // If this isn't visible, then fast-out, else return whether the parent is visible until
    // there are no more parents
    return is_visible_ && ((parent()) ? ((StageNode*) parent())->is_visible() : true);
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

    mark_transformed_aabb_dirty();

    for(auto node: each_child()) {
        assert(node);
        node->update_transformation_from_parent();
    }
}

void StageNode::on_parent_set(TreeNode* oldp, TreeNode* newp) {
    update_transformation_from_parent();
}

AABB StageNode::calculate_transformed_aabb() const {
    auto corners = aabb().corners();
    auto transform = absolute_transformation();

    for(auto& corner: corners) {
        corner = corner.transformed_by(transform);
    }

    return AABB(corners.data(), corners.size());
}

const AABB StageNode::transformed_aabb() const {
    recalc_bounds_if_necessary();
    return transformed_aabb_;
}

StageNode *StageNode::find_child_with_name(const std::string &name) {
    for(auto stage_node: each_descendent()) {
        if(stage_node && stage_node->name() == name) {
            return stage_node;
        }
    }

    return nullptr;
}

void StageNode::recalc_bounds_if_necessary() const {
    if(!transformed_aabb_dirty_) {
        return;
    }

    auto newb = calculate_transformed_aabb();
    if(newb.min() != transformed_aabb_.min() || newb.max() != transformed_aabb_.max()) {
        transformed_aabb_ = newb;
        signal_bounds_updated_(transformed_aabb_);
    }

    transformed_aabb_dirty_ = false;
}

void StageNode::mark_transformed_aabb_dirty() {
    transformed_aabb_dirty_ = true;
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

StageNodeIterator& StageNodeIterator::operator++() {
    /* We essentially have to find the next node, this varies depending
         * on whether we're doing leaf-first or not... */
    if(!current_ || !start_) {
        return *this;
    }

    if(itype_ == ITERATION_TYPE_SIBLINGS || itype_ == ITERATION_TYPE_CHILDREN) {
        // Easy! Just go right!
        // If current_ ends up as null then we're at the end
        // Look at how we construct the StageNodeIterator to see how we set
        // the start node for both these types.
        current_ = (StageNode*) current_->right_;

        // Skip past the root if we're not including it
        if(current_ == start_ && !include_root_) {
            current_ = (StageNode*) current_->right_;
        }
    } else if(itype_ == ITERATION_TYPE_ANCESTORS) {
        // Again, easy! We just go up!
        current_ = (StageNode*) current_->parent_;

        // Skip past the root if we're not including it
        // remember root_ is the starting point, not the root
        // of the tree
        if(current_ == start_ && !include_root_) {
            current_ = (StageNode*) current_->parent_;
        }
    } else {
        /* Some kind of descendent iteration */
        if(!leaf_first_) {
            if(current_->first_child_) {
                current_ = (StageNode*) current_->first_child_;
            } else if(current_->right_) {
                current_ = (StageNode*) current_->right_;
            } else {
                // No children, and we're at the last one so we
                // work back up, looking for opportunities to go
                // right
                while(current_ != start_) {
                    assert(current_->parent_);
                    current_ = (StageNode*) current_->parent_;

                    if(current_ == start_) {
                        // If our start_ node wasn't the root of the tree
                        // then we might accidentally shift right to a sibling
                        // at this point without this check
                        break;
                    }

                    if(current_->right_) {
                        // Shift to the sibling and begin descending that branch
                        current_ = (StageNode*) current_->right_;
                        break;
                    }
                }

                if(current_ == start_) {
                    /* We are done, we're back at the root! */
                    start_ = nullptr;
                    current_ = nullptr;
                }
            }
        } else {
            /* Leaf-first is tricksy!

                            o 9
                           / \
                        5 o   o 8
                         / \   \
                      3 o 4 o   o 7
                       / \       \
                    1 o   o 2     o 6
            */

            if(current_ == start_) {
                // This is for the case that we've started
                // at a node which isn't the true root node
                // and we've found our way back up to it
                current_ = nullptr;
            } else if(current_->right_) {
                // Go along any siblings
                current_ = (StageNode*) current_->right_;

                // If we've moved to a sibling with children
                // go right down to the first child
                while(current_->first_child_) {
                    history_.push(current_);
                    current_ = (StageNode*) current_->first_child_;
                }
            } else if(!history_.empty()) {
                current_ = history_.top();
                history_.pop();

                /* If we've worked our way back up to the start
                 * but we're not including the root, then just end now */
                if(current_ == start_ && !include_root_) {
                    current_ = nullptr;
                }
            } else {
                // History is empty and we can't go
                // right, we must be back at the true root
                // we shouldn't really get here a the
                // first if should stop us, but still!
                current_ = nullptr;
                assert(!current_->parent());
            }
        }
    }

    return *this;
}

StageNodeIterator::StageNodeIterator(StageNode* root, StageNodeIterator::IterationType itype, bool include_root, bool leaf_first):
    start_(root),
    itype_(itype),
    include_root_(include_root),
    leaf_first_(leaf_first) {

    current_ = start_;

    /* To iterate all siblings we need to start
         * at the parent's first child and go right from
         * there */
    if(itype == ITERATION_TYPE_SIBLINGS && start_->parent_) {
        current_ = (StageNode*) start_->parent_->first_child();
        if(current_ == start_ && !include_root) {
            current_ = (StageNode*) current_->right_;
        }
    } else if(itype == ITERATION_TYPE_ANCESTORS && !include_root) {
        current_ = (StageNode*) current_->parent();
    } else if(itype == ITERATION_TYPE_CHILDREN) {
        // Children is similar, we start at the first child
        // and just go right
        current_ = (StageNode*) start_->first_child();
    } else if(itype == ITERATION_TYPE_DESCENDENTS && leaf_first) {
        // If we're doing a leaf-first iteration, we start at the first
        // leaf and push a stack of nodes that we skipped
        while(current_->first_child_) {
            history_.push(current_);
            current_ = (StageNode*) current_->first_child_;
        }
    } else if(itype == ITERATION_TYPE_DESCENDENTS && !include_root) {
        // We're not doing a leaf first iteration, but we don't want
        // to include the root
        current_ = (StageNode*) current_->first_child_;
    }
}

}
