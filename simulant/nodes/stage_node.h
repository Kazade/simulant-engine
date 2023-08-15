#pragma once

#include <queue>

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
#include "../coroutines/helpers.h"

#include "iterators/sibling_iterator.h"
#include "iterators/child_iterator.h"
#include "iterators/descendent_iterator.h"
#include "iterators/ancestor_iterator.h"

#include "builtins.h"

namespace smlt {

class RenderableFactory;
class Seconds;
class Scene;

typedef sig::signal<void (AABB)> BoundsUpdatedSignal;
typedef sig::signal<void ()> CleanedUpSignal;

typedef sig::signal<void (CameraPtr, Viewport*)> PipelineStartedSignal;
typedef sig::signal<void (CameraPtr, Viewport*)> PipelineFinishedSignal;

/* Used for multiple levels of detail when rendering stage nodes */

enum DetailLevel {
    DETAIL_LEVEL_NEAREST = 0,
    DETAIL_LEVEL_NEAR,
    DETAIL_LEVEL_MID,
    DETAIL_LEVEL_FAR,
    DETAIL_LEVEL_FARTHEST,
    DETAIL_LEVEL_MAX
};


/* Must be implemented as follows for each node type
 *
 * template<>
 * struct stage_node_traits<MyNode> {
 *      const static StageNodeType stage_node_type = X;
 *      typedef MyNodeParams params_type;
 * };
 */
template<typename T>
struct stage_node_traits;

class StageNode:
    public generic::Identifiable<StageNodeID>,
    public DestroyableObject,
    public virtual Nameable,
    public Printable,
    public Transformable,
    public Updateable,
    public virtual BoundableEntity,
    public virtual TwoPhaseConstructed {

    DEFINE_SIGNAL(BoundsUpdatedSignal, signal_bounds_updated);
    DEFINE_SIGNAL(CleanedUpSignal, signal_cleaned_up); // Fired when the node is cleaned up later, following destroy
    DEFINE_SIGNAL(PipelineStartedSignal, signal_pipeline_started);
    DEFINE_SIGNAL(PipelineFinishedSignal, signal_pipeline_finished);

private:
    /* Heirarchy */

    friend class DescendentIterator<false>;
    friend class DescendentIterator<true>;

    StageNode* parent_ = nullptr;
    StageNode* next_ = nullptr;
    StageNode* prev_ = nullptr;
    StageNode* first_child_ = nullptr;
    StageNode* last_child_ = nullptr;

    virtual void on_parent_set(StageNode* oldp, StageNode* newp) {
        _S_UNUSED(oldp);

        if(!newp) {
            return;
        }

        update_transformation_from_parent();
        recalc_visibility();
    }

public:
    StageNode* find_descendent_with_id(StageNodeID id) {
        for(auto& it: each_descendent()) {
            if(it.id() == id) {
                return &it;
            }
        }

        return nullptr;
    }

    bool is_root() const {
        return !has_parent();
    }

    StageNode* first_sibling() {
        return parent_->first_child_;
    }

    const StageNode* first_sibling() const {
        return parent_->first_child_;
    }

    StageNode* next_sibling() {
        return next_;
    }

    const StageNode* next_sibling() const {
        return next_;
    }

    StageNode* parent() {
        return parent_;
    }

    const StageNode* parent() const {
        return parent_;
    }

    StageNode* first_child() {
        return first_child_;
    }

    const StageNode* first_child() const {
        return first_child_;
    }

    StageNode* last_child() {
        return last_child_;
    }

    const StageNode* last_child() const {
        return last_child_;
    }

    const StageNode* child_at(std::size_t i) const;

    std::size_t child_count() const;

    bool has_parent() const {
        return parent_;
    }

    void remove_from_parent();
    void set_parent(StageNode* new_parent);
    void append_child(StageNode* new_child) {
        new_child->set_parent(this);
    }

    /** If this returns true, then generate_renderables will not be called on
     *  descendents of this node. It is assumed that this node generates
     *  renderables for all its children. */
    bool generates_renderables_for_descendents() const {
        return _generates_renderables_for_descendents();
    }

    /** Populates the render queue with a list of renderables to send to be
     *  rendered by the render pipelines */
    void generate_renderables(
        batcher::RenderQueue* render_queue,
        const CameraPtr& camera,
        const DetailLevel detail_level
    );

private:
    friend class StageNodeManager;

    // NVI idiom
    bool _create(void* params) {
        return on_create(params);
    }

    void _destroy() override final;

    void update(float dt) override final;
    void late_update(float dt) override final;
    void fixed_update(float step) override final;

    virtual bool on_create(void* params) = 0;
    virtual bool on_destroy() { return true; }
    virtual bool on_destroy_immediately() { return true; }

    virtual void on_update(float dt) { _S_UNUSED(dt); }
    virtual void on_fixed_update(float step) { _S_UNUSED(step); }
    virtual void on_late_update(float dt) { _S_UNUSED(dt); }

    virtual bool _generates_renderables_for_descendents() const {
        return false;
    }

    /* Return a list of renderables to pass into the render queue */
    virtual void _generate_renderables(
        batcher::RenderQueue* render_queue,
        const CameraPtr& camera,
        const DetailLevel detail_level
    ) = 0;

    virtual void _destroy_immediately() final;

private:
    /* This is ugly, but it's here for performance to avoid
     * a map lookup when staging writes to the partitioner */
    friend class Partitioner;
    bool partitioner_dirty_ = false;
    bool partitioner_added_ = false;

public:
    class SiblingIteratorPair {
        friend class StageNode;

        SiblingIteratorPair(StageNode* root):
            root_(root) {}

        StageNode* root_;

    public:
        SiblingIterator<false> begin() {
            if(!root_->parent()) {
                return SiblingIterator<false>(root_, nullptr);
            }

            return SiblingIterator<false>(
                root_,
                (root_->next_sibling()) ? root_->next_sibling() : root_->parent_->first_child()
            );
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
            return ChildIterator<false>(root_, root_->first_child_);
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

    StageNode(Scene* owner, StageNodeType node_type);

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
    Property<Scene* StageNode::*> scene = { this, &StageNode::owner_ };


    smlt::Promise<void> destroy_after(const Seconds& seconds);

    bool parent_is_scene() const;

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


    void set_cullable(bool v);
    bool is_cullable() const;

protected:
    // Faster than properties, useful for subclasses where a clean API isn't as important
    Scene* get_scene() const { return owner_; }

    void on_transformation_changed() override;

    virtual void update_transformation_from_parent();

    void recalc_bounds_if_necessary() const;
    void mark_transformed_aabb_dirty();

    void mark_absolute_transformation_dirty();
private:
    AABB calculate_transformed_aabb() const;
    Scene* owner_ = nullptr;
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

    /* Passed to coroutines and used to detect when the object has been destroyed */
    std::shared_ptr<bool> alive_marker_ = std::make_shared<bool>(true);
};


class StageNodeVisitorBFS {
public:
    template<typename Func>
    StageNodeVisitorBFS(StageNode* start, Func&& callback):
        callback_(callback) {

        queue_.push(start);
    }

    bool call_next() {
        StageNode* it = queue_.front();
        queue_.pop();

        for(auto& node: it->each_child()) {
            queue_.push(&node);
        }

        callback_(it);

        return !queue_.empty();
    }

private:
    std::function<void (StageNode*)> callback_;
    StageNode* it_ = nullptr;
    std::queue<StageNode*> queue_;
};


class ContainerNode : public StageNode {
public:
    ContainerNode(Scene* scene, StageNodeType node_type):
        StageNode(scene, node_type) {}

    /* Containers don't directly have renderables, but their children do */
    void _generate_renderables(batcher::RenderQueue*, const CameraPtr&, const DetailLevel) override {}

    virtual ~ContainerNode() {}
};

typedef StageNode* StageNodePtr;

}

#include "iterators/sibling_iterator.inc"
#include "iterators/child_iterator.inc"
#include "iterators/descendent_iterator.inc"
#include "iterators/ancestor_iterator.inc"


