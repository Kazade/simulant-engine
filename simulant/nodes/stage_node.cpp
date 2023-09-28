#include "../stage.h"
#include "camera.h"
#include "../window.h"
#include "../application.h"

namespace smlt {

std::vector<StageNode *> StageNode::find_descendents_by_types(std::initializer_list<StageNodeType> type_list) const {
    std::set<StageNodeType> types(type_list);
    std::vector<StageNode*> nodes;
    for(auto& node: each_descendent()) {
        if(types.count(node.node_type())) {
            nodes.push_back(&node);
        }
    }

    return nodes;
}

const StageNode* StageNode::child_at(std::size_t i) const {
    std::size_t j = 0;

    auto it = first_child_;
    while(it) {
        if(j++ == i) {
            return it;
        }

        it = it->next_;
    }

    return nullptr;
}

std::size_t StageNode::child_count() const {
    std::size_t i = 0;
    auto it = first_child_;
    while(it) {
        ++i;
        it = it->next_;
    }

    return i;
}

void StageNode::remove_from_parent() {
    if(!parent_) {
        return;
    }

    /* Remove from sibling list */
    if(parent_->first_child_ == this) {
        parent_->first_child_ = next_;
        if(parent_->first_child_) {
            parent_->first_child_->prev_ = nullptr;
        }
    }

    if(parent_->last_child_ == this) {
        parent_->last_child_ = prev_;
        if(parent_->last_child_) {
            parent_->last_child_->next_ = nullptr;
        }
    }

    if(next_) next_->prev_ = prev_;
    if(prev_) prev_->next_ = next_;

    parent_ = nullptr;
    next_ = prev_ = nullptr;

    assert(first_child_ != this);
    assert(last_child_ != this);
    assert(next_ != this);
    assert(prev_ != this);

    on_parent_set(parent_, nullptr);
}

void StageNode::set_parent(StageNode* new_parent) {
    if(new_parent == parent_ || new_parent == this) {
        return;
    }

    auto old_parent = parent_;

    remove_from_parent();
    parent_ = new_parent;

    // FIXME: DELETE THIS! It ensures that nodes are always attached to the
    // scene, but we want to move away from that
    if(!parent_) {
        parent_ = scene.get();
    }

    if(parent_->first_child_) {
        /* New parent already had children, so find the last
         * one and attach there */
        auto it = parent_->last_child();
        assert(it);
        assert(!it->next_);

        it->next_ = this;
        prev_ = it;
        next_ = nullptr;
        parent_->last_child_ = this;
    } else {
        assert(!parent_->last_child_);

        parent_->first_child_ = this;
        parent_->last_child_ = this;
        next_ = nullptr;
        prev_ = nullptr;
    }

    assert(first_child_ != this);
    assert(last_child_ != this);
    assert(next_ != this);
    assert(prev_ != this);

    on_parent_set(old_parent, parent_);
}

void StageNode::generate_renderables(batcher::RenderQueue* render_queue, const smlt::Camera* camera, const smlt::Viewport *viewport, const DetailLevel detail_level) {
    do_generate_renderables(render_queue, camera, viewport, detail_level);
}

/* Right this is a bit confusing.
 *
 * StageNode inherits DestroyableObject which provides
 * destroy() which calls on_destroy() and then if that
 * returns true, it calls _destroy().
 *
 * In the case that someone calls destroy(),
 * we want to essentially delay actual destruction until after
 * late_update. So we override _destroy here to tell the scene
 * that this node needs proper destroying
 */
void StageNode::finalize_destroy() {
    if(owner_) {
        owner_->queue_clean_up(this);

        // Go through the subnodes and ask each for destruction in-turn
        for(auto& stage_node: each_child()) {
            stage_node.destroy();
        };
    }
}

StageNode::StageNode(smlt::Scene* owner, smlt::StageNodeType node_type):
    Identifiable(new_stage_node_id(node_type)),
    AudioSource(owner, this, get_app()->sound_driver.get()),
    owner_(owner),
    node_type_(node_type) {

}

StageNode::~StageNode() {

}

StageNodeType StageNode::node_type() const {
    return node_type_;
}

void StageNode::on_clean_up() {
    signal_cleaned_up_(); // Tell everyone we're going
    remove_from_parent(); // Make sure we're not connected to anything
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
    if(!absolute_transformation_is_dirty_) {
        return absolute_transformation_;
    }

    auto c0 = smlt::Vec4(absolute_rotation_ * smlt::Vec3(absolute_scale_.x, 0, 0), 0);
    auto c1 = smlt::Vec4(absolute_rotation_ * smlt::Vec3(0, absolute_scale_.y, 0), 0);
    auto c2 = smlt::Vec4(absolute_rotation_ * smlt::Vec3(0, 0, absolute_scale_.z), 0);

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

    absolute_transformation_[12] = absolute_position_.x;
    absolute_transformation_[13] = absolute_position_.y;
    absolute_transformation_[14] = absolute_position_.z;
    absolute_transformation_[15] = 1.0f;

    absolute_transformation_is_dirty_ = false;
    return absolute_transformation_;
}

Vec3 StageNode::absolute_forward() const {
    return absolute_rotation_.forward();
}

Vec3 StageNode::absolute_right() const {
    return absolute_rotation_.right();
}

Vec3 StageNode::absolute_up() const {
    return absolute_rotation_.up();
}

void StageNode::recalc_visibility() {
    bool previously_visible = self_and_parents_visible_;

    // If this isn't visible, then fast-out, else return whether the parent is visible until
    // there are no more parents
    self_and_parents_visible_ = is_visible_ && ((parent_) ? parent_->is_visible() : true);

    if(previously_visible != self_and_parents_visible_) {
        /* Recurse through children to update */
        for(auto& node: each_child()) {
            node.recalc_visibility();
        }
    }
}

void StageNode::finalize_destroy_immediately() {
    if(owner_) {
        std::vector<StageNode*> to_destroy;

        for(auto& child: each_child()) {
            to_destroy.push_back(&child);
        }

        owner_->clean_up_node(this);

        std::for_each(to_destroy.begin(), to_destroy.end(), [](StageNode* n) { n->destroy_immediately(); });
    }
}

bool StageNode::is_visible() const {
    return self_and_parents_visible_;
}

void StageNode::set_visible(bool visible) {
    is_visible_ = visible;
    recalc_visibility();
}

smlt::Promise<void> StageNode::destroy_after(const Seconds& seconds) {
    std::weak_ptr<bool> alive = alive_marker_;

    auto conn = std::make_shared<sig::connection>();
    auto counter = std::make_shared<float>(0.0f);

    auto p = Promise<void>::create();

    *conn = smlt::get_app()->signal_update().connect([=] (float dt) mutable {
        *counter += dt;

        if(*counter < seconds.to_float()) {
            return;
        }

        /* Detect whether the object was already destroyed. This avoids a weird
         * bug where the wrong object can be destroyed if the original was replaced
         * in the stage node pool before this code runs */
        if(alive.lock()) {
            if(!is_destroyed()) {
                destroy();
            }
        }

        conn->disconnect();
        p.fulfill();
    });

    return p;
}

void StageNode::move_to_absolute(const Vec3& position) {
    if(!has_parent()) {
        move_to(position);
    } else {
        assert(parent_);

        // The stage itself is immovable so, we only bother with this if this isn't he stage
        // could also use if(!parent()->is_root())
        Vec3 ppos = parent_->absolute_position();
        move_to(position - ppos);
    }
}

void StageNode::move_to_absolute(float x, float y, float z) {
    move_to_absolute(Vec3(x, y, z));
}

void StageNode::rotate_to_absolute(const Quaternion& rotation) {
    if(has_parent()) {
        auto prot = parent_->absolute_rotation();
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
    StageNode* parent = parent_;

    if(!parent || parent_is_scene()) {
        absolute_rotation_ = rotation_;
        absolute_position_ = position_;
        absolute_scale_ = scaling_;
    } else {
        auto parent_pos = parent->absolute_position();
        auto parent_rot = parent->absolute_rotation();
        auto parent_scale = parent->absolute_scaling();

        absolute_rotation_ = parent_rot * rotation_;
        absolute_position_ = parent_pos + parent_rot * position_;
        absolute_scale_ = parent_scale * scaling_;
    }

    mark_transformed_aabb_dirty();
    mark_absolute_transformation_dirty();

    for(auto& node: each_child()) {
        node.update_transformation_from_parent();
    }
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

StageNode *StageNode::find_descendent_with_name(const std::string &name) {
    for(auto& stage_node: each_descendent()) {
        if(stage_node.name() == name) {
            return &stage_node;
        }
    }

    return nullptr;
}

void StageNode::set_cullable(bool v) {
    cullable_ = v;
}

bool StageNode::is_cullable() const {
    return cullable_;
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

void StageNode::mark_absolute_transformation_dirty() {
    absolute_transformation_is_dirty_ = true;
}

void StageNode::update(float dt) {
    on_update(dt);

    for(auto& child: each_child()) {
        child.update(dt);
    }
}

void StageNode::late_update(float dt) {
    on_late_update(dt);

    for(auto& child: each_child()) {
        child.late_update(dt);
    }
}

void StageNode::fixed_update(float step) {
    on_fixed_update(step);

    for(auto& child: each_child()) {
        child.fixed_update(step);
    }
}

bool StageNode::parent_is_scene() const {
    return parent_ == scene.get();
}

}
