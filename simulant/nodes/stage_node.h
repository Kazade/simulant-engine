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

#include "iterators/sibling_iterator.h"
#include "iterators/child_iterator.h"
#include "iterators/descendent_iterator.h"
#include "iterators/ancestor_iterator.h"

namespace smlt {

class RenderableFactory;
class Seconds;

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

enum StageNodeType {
    STAGE_NODE_TYPE_STAGE,
    STAGE_NODE_TYPE_CAMERA,
    STAGE_NODE_TYPE_ACTOR,
    STAGE_NODE_TYPE_LIGHT,
    STAGE_NODE_TYPE_PARTICLE_SYSTEM,
    STAGE_NODE_TYPE_GEOM,
    STAGE_NODE_TYPE_OTHER
};

class StageNode:
    public virtual DestroyableObject,
    public TreeNode,
    public virtual Nameable,
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

public:
    class SiblingIteratorPair {
        friend class StageNode;

        SiblingIteratorPair(StageNode* root):
            root_(root) {}

        StageNode* root_;

    public:
        SiblingIterator<false> begin() {
            return SiblingIterator<false>(root_);
        }

        SiblingIterator<false> end() {
            return SiblingIterator<false>(root_, nullptr);
        }
    };

    class ChildIteratorPair {
        friend class StageNode;

        ChildIteratorPair(StageNode* root):
            root_(root) {}

        StageNode* root_;

    public:
        ChildIterator<false> begin() {
            return ChildIterator<false>((StageNode*) root_);
        }

        ChildIterator<false> end() {
            return ChildIterator<false>(root_, nullptr);
        }
    };

    class DescendentIteratorPair {
        friend class StageNode;

        DescendentIteratorPair(StageNode* root):
            root_(root) {}

        StageNode* root_;

    public:
        DescendentIterator<false> begin() {
            return DescendentIterator<false>((StageNode*) root_);
        }

        DescendentIterator<false> end() {
            return DescendentIterator<false>(root_, nullptr);
        }
    };

    class AncestorIteratorPair {
        friend class StageNode;

        AncestorIteratorPair(StageNode* root):
            root_(root) {}

        StageNode* root_;

    public:
        AncestorIterator<false> begin() {
            return AncestorIterator<false>((StageNode*) root_);
        }

        AncestorIterator<false> end() {
            return AncestorIterator<false>(root_, nullptr);
        }
    };

    AncestorIteratorPair each_ancestor() {
        return AncestorIteratorPair(this);
    }

    DescendentIteratorPair each_descendent() {
        return DescendentIteratorPair(this);
    }

    SiblingIteratorPair each_sibling() {
        return SiblingIteratorPair(this);
    }

    ChildIteratorPair each_child() {
        return ChildIteratorPair(this);
    }

    std::string repr() const override {
        return name();
    }

    StageNode(Stage* stage, StageNodeType node_type);

    virtual ~StageNode();

    StageNodeType node_type() const;

    /** Link the position of this `StageNode` to another
     * stage node. This is effectively the same behaviour
     * as calling set_absolute_position(other->absolute_position())
     * in late_update() */
    void link_position(StageNode* other);

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
    void set_visible(bool visible);

    Property<generic::DataCarrier StageNode::*> data = { this, &StageNode::data_ };
    Property<Stage* StageNode::*> stage = { this, &StageNode::stage_ };

    template<typename T>
    void set_parent(const UniqueID<T>& id) {
        set_parent(id.fetch());
    }

    void set_parent(TreeNode* node);

    void destroy_after(const Seconds& seconds);

    void update(float dt) override;
    void late_update(float dt) override;
    void fixed_update(float step) override;

    bool parent_is_stage() const;

    void clean_up() override;

    const AABB transformed_aabb() const override;

    /* Control shading on the stage node (behaviour depends on the type of node) */
    ShadowCast shadow_cast() const { return shadow_cast_; }
    void set_shadow_cast(ShadowCast cast) {
        shadow_cast_ = cast;
    }

    ShadowReceive shadow_receive() const { return shadow_receive_; }
    void set_shadow_receive(ShadowReceive receive) { shadow_receive_ = receive; }

    StageNode* find_descendent_with_name(const std::string& name);

    /* Return a list of renderables to pass into the render queue */
    virtual void _get_renderables(
        batcher::RenderQueue* render_queue,
        const CameraPtr camera,
        const DetailLevel detail_level
    ) = 0;

    void set_cullable(bool v);
    bool is_cullable() const;

protected:
    // Faster than properties, useful for subclasses where a clean API isn't as important
    Stage* get_stage() const { return stage_; }

    void on_transformation_changed() override;
    void on_parent_set(TreeNode* oldp, TreeNode* newp) override;

    virtual void update_transformation_from_parent();

    void recalc_bounds_if_necessary() const;
    void mark_transformed_aabb_dirty();

    void mark_absolute_transformation_dirty();
private:
    AABB calculate_transformed_aabb() const;

    Stage* stage_ = nullptr;
    StageNode* parent_stage_node_ = nullptr;

    StageNodeType node_type_ = STAGE_NODE_TYPE_ACTOR;

    generic::DataCarrier data_;

    StageNode* linked_position_node_ = nullptr;
    sig::connection linked_position_node_destroyed_;

    bool is_visible_ = true;
    bool self_and_parents_visible_ = true;
    void recalc_visibility();

    Vec3 absolute_position_;
    Quaternion absolute_rotation_;
    Vec3 absolute_scale_ = Vec3(1, 1, 1);

    mutable Mat4 absolute_transformation_;
    mutable bool absolute_transformation_is_dirty_ = true;

    /* Mutable so that AABB accesses can be const, but we delay
     * calculation until access */
    mutable AABB transformed_aabb_;
    mutable bool transformed_aabb_dirty_ = false;

    // By default, always cast and receive shadows
    ShadowCast shadow_cast_ = SHADOW_CAST_ALWAYS;
    ShadowReceive shadow_receive_ = SHADOW_RECEIVE_ALWAYS;

    /* Whether or not this node should be culled by the partitioner (e.g. when offscreen) */
    bool cullable_ = true;
};


class ContainerNode : public StageNode {
public:
    ContainerNode(Stage* stage, StageNodeType node_type):
        StageNode(stage, node_type) {}

    /* Containers don't directly have renderables, but their children do */
    void _get_renderables(batcher::RenderQueue*, const CameraPtr, const DetailLevel) override {

    }

    virtual ~ContainerNode() {}
};

}

#include "iterators/sibling_iterator.inc"
#include "iterators/child_iterator.inc"
#include "iterators/descendent_iterator.inc"
#include "iterators/ancestor_iterator.inc"
