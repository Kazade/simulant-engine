#pragma once

#include "./tree_node.h"
#include "../interfaces/nameable.h"
#include "../interfaces/printable.h"
#include "../interfaces/transformable.h"
#include "../interfaces/updateable.h"
#include "../interfaces/boundable.h"
#include "../behaviours/behaviour.h"
#include "../interfaces/has_auto_id.h"
#include "../generic/data_carrier.h"
#include "../shadows.h"
#include "../generic/manual_object.h"

namespace smlt {

class RenderableFactory;

typedef sig::signal<void (AABB)> BoundsUpdatedSignal;
typedef sig::signal<void ()> CleanedUpSignal;

/* Used for multiple levels of detail when rendering stage nodes */

enum DetailLevel {
    DETAIL_LEVEL_NEAREST = 0,
    DETAIL_LEVEL_NEAR,
    DETAIL_LEVEL_MID,
    DETAIL_LEVEL_FAR,
    DETAIL_LEVEL_FARTHEST,
    DETAIL_LEVEL_MAX
};

class StageNode;

class StageNodeIterator:
    public std::iterator<std::forward_iterator_tag, StageNode*, uint32_t, const StageNode**, StageNode*> {

public:
    enum IterationType {
        ITERATION_TYPE_SIBLINGS,
        ITERATION_TYPE_ANCESTORS,
        ITERATION_TYPE_CHILDREN,
        ITERATION_TYPE_DESCENDENTS
    };

    StageNodeIterator():
        start_(nullptr),
        current_(nullptr) {}

    explicit StageNodeIterator(
        StageNode* root,
        IterationType itype, bool include_root, bool leaf_first
    );

    StageNodeIterator& operator++();

    StageNodeIterator operator++(int) {
        auto retval = *this;
        ++(*this);
        return retval;
    }

    bool operator==(StageNodeIterator& other) const {
        if(!current_ && !other.current_) {
            // Both "end" iterators
            return true;
        }

        // FIXME: compare everything?
        return start_ == other.start_ && current_ == other.current_ && itype_ == other.itype_;
    }

    bool operator!=(StageNodeIterator other) const {
        return !(*this == other);
    }

    reference operator*() const {
        return current_;
    }

private:
    StageNode* start_ = nullptr;
    StageNode* current_ = nullptr;
    IterationType itype_ = ITERATION_TYPE_SIBLINGS;
    bool include_root_ = false;
    bool leaf_first_ = false;

    /* For leaf-first iteration we need to store the parents
     * we haven't visited */
    std::vector<StageNode*> history_;
};


class StageNode:
    public virtual DestroyableObject,
    public TreeNode,
    public Nameable,
    public Printable,
    public Transformable,
    public Updateable,
    public virtual BoundableEntity,
    public Organism,
    public HasAutoID<StageNode>,
    public TwoPhaseConstructed {

    DEFINE_SIGNAL(BoundsUpdatedSignal, signal_bounds_updated);

    // Fired when the node is cleaned up later, following destroy
    DEFINE_SIGNAL(CleanedUpSignal, signal_cleaned_up);

    friend class StageNodeIterator;

public:
    class StageNodeIteratorPair {
    private:
        friend class StageNode;

        StageNodeIteratorPair(StageNode* root, StageNodeIterator::IterationType itype, bool include_root, bool leaf_first):
            root_(root),
            itype_(itype),
            include_root_(include_root),
            leaf_first_(leaf_first) {

        }

        StageNode* root_;
        StageNodeIterator::IterationType itype_;
        bool include_root_;
        bool leaf_first_;

    public:
        StageNodeIterator begin() {
            return StageNodeIterator(root_, itype_, include_root_, leaf_first_);
        }

        StageNodeIterator end() {
            return StageNodeIterator();
        }
    };

    StageNodeIteratorPair each_ancestor() {
        return StageNodeIteratorPair(this, StageNodeIterator::ITERATION_TYPE_ANCESTORS, false, false);
    }

    StageNodeIteratorPair each_descendent() {
        return StageNodeIteratorPair(this, StageNodeIterator::ITERATION_TYPE_DESCENDENTS, false, false);
    }

    StageNodeIteratorPair each_descendent_lf() {
        return StageNodeIteratorPair(this, StageNodeIterator::ITERATION_TYPE_DESCENDENTS, false, true);
    }

    StageNodeIteratorPair each_descendent_and_self() {
        return StageNodeIteratorPair(this, StageNodeIterator::ITERATION_TYPE_DESCENDENTS, true, false);
    }

    StageNodeIteratorPair each_sibling() {
        return StageNodeIteratorPair(this, StageNodeIterator::ITERATION_TYPE_SIBLINGS, false, false);
    }

    StageNodeIteratorPair each_sibling_and_self() {
        return StageNodeIteratorPair(this, StageNodeIterator::ITERATION_TYPE_SIBLINGS, true, false);
    }

    StageNodeIteratorPair each_descendent_and_self_lf() {
        return StageNodeIteratorPair(this, StageNodeIterator::ITERATION_TYPE_DESCENDENTS, true, true);
    }

    StageNodeIteratorPair each_child() {
        return StageNodeIteratorPair(this, StageNodeIterator::ITERATION_TYPE_CHILDREN, false, false);
    }

    std::string repr() const override {
        return name();
    }

    StageNode(Stage* stage);

    virtual ~StageNode();


    /* Without a parent, these are the same as move_to/rotate_to. With a parent
     * this applies a relative position / rotation to the parent position / rotation
     * so the node appears where you want */
    void move_to_absolute(const Vec3& position);
    void move_to_absolute(float x, float y, float z);
    void rotate_to_absolute(const Quaternion& rotation);
    void rotate_to_absolute(const Degrees& degrees, float x, float y, float z);

    Vec3 absolute_position() const;
    Quaternion absolute_rotation() const;
    Vec3 absolute_scaling() const;
    Mat4 absolute_transformation() const;

    bool is_visible() const;

    bool is_intended_visible() const { return is_visible_; }
    void set_visible(bool visible) { is_visible_ = visible; }

    Property<generic::DataCarrier StageNode::*> data = { this, &StageNode::data_ };
    Property<Stage* StageNode::*> stage = { this, &StageNode::stage_ };

    template<typename T>
    void set_parent(const UniqueID<T>& id) {
        TreeNode::set_parent(id.fetch());
    }

    void set_parent(TreeNode* node) {
        TreeNode::set_parent(node);
    }

    void set_parent(CameraID id);

    void update(float dt) override;
    void late_update(float dt) override;
    void fixed_update(float step) override;

    bool parent_is_stage() const { return parent() == (TreeNode*) stage_; }

    void clean_up() override;

    const AABB transformed_aabb() const override;

    /* Control shading on the stage node (behaviour depends on the type of node) */
    ShadowCast shadow_cast() const { return shadow_cast_; }
    void set_shadow_cast(ShadowCast cast) {
        shadow_cast_ = cast;
    }

    ShadowReceive shadow_receive() const { return shadow_receive_; }
    void set_shadow_receive(ShadowReceive receive) { shadow_receive_ = receive; }

    StageNode* find_child_with_name(const std::string& name);

    /* Return a list of renderables to pass into the render queue */
    virtual void _get_renderables(
        batcher::RenderQueue* render_queue,
        const CameraPtr camera,
        const DetailLevel detail_level
    ) = 0;

protected:
    // Faster than properties, useful for subclasses where a clean API isn't as important
    Stage* get_stage() const { return stage_; }

    void on_transformation_changed() override;
    void on_parent_set(TreeNode* oldp, TreeNode* newp) override;

    virtual void update_transformation_from_parent();

    void recalc_bounds_if_necessary() const;
    void mark_transformed_aabb_dirty();

private:
    AABB calculate_transformed_aabb() const;

    Stage* stage_ = nullptr;

    generic::DataCarrier data_;

    bool is_visible_ = true;

    Vec3 absolute_position_;
    Quaternion absolute_rotation_;
    Vec3 absolute_scale_ = Vec3(1, 1, 1);

    /* Mutable so that AABB accesses can be const, but we delay
     * calculation until access */
    mutable AABB transformed_aabb_;
    mutable bool transformed_aabb_dirty_ = false;

    // By default, always cast and receive shadows
    ShadowCast shadow_cast_ = SHADOW_CAST_ALWAYS;
    ShadowReceive shadow_receive_ = SHADOW_RECEIVE_ALWAYS;
};


class ContainerNode : public StageNode {
public:
    ContainerNode(Stage* stage):
        StageNode(stage) {}

    /* Containers don't directly have renderables, but their children do */
    void _get_renderables(batcher::RenderQueue*, const CameraPtr, const DetailLevel) override {

    }

    virtual ~ContainerNode() {}
};

}
