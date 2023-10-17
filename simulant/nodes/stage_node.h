#pragma once

#include <queue>

#include "../interfaces/nameable.h"
#include "../interfaces/printable.h"
#include "../interfaces/updateable.h"
#include "../interfaces/boundable.h"
#include "../interfaces/transform.h"
#include "../interfaces/has_auto_id.h"
#include "../generic/data_carrier.h"
#include "../shadows.h"
#include "../generic/manual_object.h"
#include "../coroutines/helpers.h"
#include "../sound.h"

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

/* Order of things:
 *
 * 1. constructor
 * 2. init() -> on_init()
 * 3. create(params) -> on_create(params)
 * 4. destroy() -> on_destroy()
 * 5. clean_up() -> on_clean_up()
 * 6. destructor
 */

namespace impl {

template<typename F, typename T, typename... Args>
T* child_factory(F& factory, StageNode* parent, Args&&... args) {
    auto node = factory->template create_node<T>(std::forward<Args>(args)...);
    node->set_parent(parent);
    return node;
}

template<typename F, typename T, typename... Args>
T* mixin_factory(F& factory, StageNode* base, Args&&... args);

}

class StageNode:
    public generic::Identifiable<StageNodeID>,
    public DestroyableObject,
    public virtual Nameable,
    public Printable,
    public Updateable,
    public virtual BoundableEntity,
    public virtual TwoPhaseConstructed,
    public TransformListener {

    DEFINE_SIGNAL(BoundsUpdatedSignal, signal_bounds_updated);
    DEFINE_SIGNAL(CleanedUpSignal, signal_cleaned_up); // Fired when the node is cleaned up later, following destroy

private:
    /* Heirarchy */

    friend class DescendentIterator<false>;
    friend class DescendentIterator<true>;

    StageNode* parent_ = nullptr;
    StageNode* next_ = nullptr;
    StageNode* prev_ = nullptr;
    StageNode* first_child_ = nullptr;
    StageNode* last_child_ = nullptr;

    virtual void on_parent_set(StageNode* oldp, StageNode* newp, TransformRetainMode transform_retain) {
        _S_UNUSED(oldp);

        if(newp) {
            transform->set_parent(newp->transform, transform_retain);
        } else {
            transform->set_parent(nullptr);
        }

        recalc_visibility();
    }

public:
    std::vector<StageNode*> find_descendents_by_types(std::initializer_list<StageNodeType> type_list) const;

    StageNode* find_descendent_with_id(StageNodeID id) {
        for(auto& it: each_descendent()) {
            if(it.id() == id) {
                return &it;
            }
        }

        return nullptr;
    }

    const StageNode* find_descendent_with_id(StageNodeID id) const {
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
        return parent_ && parent_ != this;
    }

    void remove_from_parent();
    std::list<StageNode*> detach();

    void set_parent(
        StageNode* new_parent,
        TransformRetainMode transform_retain=TRANSFORM_RETAIN_MODE_LOSE
    );

    template<typename T, typename... Args>
    T* create_child(Args&&... args) {
        return impl::child_factory<decltype(owner_), T, Args...>(owner_, this, std::forward<Args>(args)...);
    }

    void adopt_children(StageNode* node) {
        node->set_parent(this);
    }

    template<typename... Args>
    void adopt_children(StageNode* node, Args... args) {
        node->set_parent(this);
        adopt_children(args...);
    }

    template<typename T, typename... Args>
    T* create_mixin(Args&& ... args) {
        return impl::mixin_factory<decltype(owner_), T, Args...>(owner_, this, std::forward<Args>(args)...);
    }

    StageNode* find_mixin(const std::string& name) const;

    template<typename T>
    T* find_mixin() const {
        auto type = T::Meta::node_type;
        auto it = mixins_.find(type);
        if(it != mixins_.end()) {
            return static_cast<T*>(it->second.ptr);
        }

        return nullptr;
    }

    std::size_t mixin_count() const {
        return mixins_.size();
    }

    StageNode* base() {
        return base_;
    }

    const StageNode* base() const {
        return base_;
    }

    bool is_mixin() const {
        return base_ != this;
    }

    /** If this returns true, then generate_renderables will not be called on
     *  descendents of this node. It is assumed that this node generates
     *  renderables for all its children. */
    bool generates_renderables_for_descendents() const {
        return do_generates_renderables_for_descendents();
    }

    /** Populates the render queue with a list of renderables to send to be
     *  rendered by the render pipelines */
    void generate_renderables(
        batcher::RenderQueue* render_queue,
        const Camera*, const Viewport* viewport,
        const DetailLevel detail_level
    );

    void update(float dt) override final;
    void late_update(float dt) override final;
    void fixed_update(float step) override final;

    /** This is a very-slow utility function that calls
     *  dynamic_cast on the entire subtree. Use for testing
     *  *only*. */
    template<typename T>
    size_t count_nodes_by_type() const {
        size_t ret = 0;
        for(auto& node: each_descendent()) {
            if(dynamic_cast<const T*>(&node)) {
                ++ret;
            }
        }
        return ret;
    }

    bool is_part_of_active_pipeline() const {
        return active_pipeline_count_ > 0;
    }

protected:
    virtual bool on_create(void* params) = 0;
    virtual bool on_destroy() { return true; }
    virtual void on_update(float dt) override { _S_UNUSED(dt); }
    virtual void on_fixed_update(float step) override { _S_UNUSED(step); }
    virtual void on_late_update(float dt) override { _S_UNUSED(dt); }

    void on_transformation_changed() override {
        mark_transformed_aabb_dirty();

        /* If this node's transform changes in some way, we need
         * to trigger updates on child transforms too */
        for(auto& child: each_child()) {
            child.transform->signal_change();
        }
    }

    void on_transformation_change_attempted() override {}

private:
    friend class StageNodeManager;

    Transform transform_;

    // NVI idiom
    bool _create(void* params) {
        return on_create(params);
    }

    void _clean_up() override;

    virtual bool do_generates_renderables_for_descendents() const {
        return false;
    }

    /* Return a list of renderables to pass into the render queue */
    virtual void do_generate_renderables(
        batcher::RenderQueue* render_queue,
        const Camera*, const Viewport* viewport,
        const DetailLevel detail_level
    ) {
        _S_UNUSED(render_queue);
        _S_UNUSED(viewport);
        _S_UNUSED(detail_level);
    }

    virtual void finalize_destroy() override final;
    virtual void finalize_destroy_immediately() final;

private:
    template<typename F, typename T, typename... Args>
    friend T* impl::mixin_factory(F& factory, StageNode* base, Args&&... args);

    /* Mixin handling */
    StageNode* base_ = this;

    struct MixinInfo {
        sig::connection destroy_connection;
        StageNode* ptr;
    };

    std::unordered_map<StageNodeType, MixinInfo> mixins_;
    void add_mixin(StageNode* mixin);

private:
    /* This is ugly, but it's here for performance to avoid
     * a map lookup when staging writes to the partitioner */
    friend class Partitioner;
    bool partitioner_dirty_ = false;
    bool partitioner_added_ = false;

public:
    class SiblingIteratorPair {
        friend class StageNode;

        SiblingIteratorPair(const StageNode* root):
            root_(root) {}

        const StageNode* root_;

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

        ChildIteratorPair(const StageNode* root):
            root_(root) {}

        const StageNode* root_;

    public:
        ChildIterator<false> begin() const {
            return ChildIterator<false>(root_, root_->first_child_);
        }

        ChildIterator<false> end() const {
            return ChildIterator<false>(root_, nullptr);
        }
    };

    class DescendentIteratorPair {
        friend class StageNode;

        DescendentIteratorPair(const StageNode* root):
            root_(root) {}

        const StageNode* root_;

    public:
        DescendentIterator<false> begin() const {
            return DescendentIterator<false>(root_);
        }

        DescendentIterator<false> end() const {
            return DescendentIterator<false>(root_, nullptr);
        }
    };

    class AncestorIteratorPair {
        friend class StageNode;

        AncestorIteratorPair(const StageNode* root):
            root_(root) {}

        const StageNode* root_;

    public:
        AncestorIterator<false> begin() {
            return AncestorIterator<false>(root_);
        }

        AncestorIterator<false> end() {
            return AncestorIterator<false>(root_, nullptr);
        }
    };

    AncestorIteratorPair each_ancestor() {
        return AncestorIteratorPair(this);
    }

    AncestorIteratorPair each_ancestor() const {
        return AncestorIteratorPair(this);
    }

    DescendentIteratorPair each_descendent() {
        return DescendentIteratorPair(this);
    }

    DescendentIteratorPair each_descendent() const {
        return DescendentIteratorPair(this);
    }

    SiblingIteratorPair each_sibling() {
        return SiblingIteratorPair(this);
    }

    SiblingIteratorPair each_sibling() const {
        return SiblingIteratorPair(this);
    }

    ChildIteratorPair each_child() {
        return ChildIteratorPair(this);
    }

    ChildIteratorPair each_child() const {
        return ChildIteratorPair(this);
    }

    std::string repr() const override {
        return name();
    }

    StageNode(Scene* owner, StageNodeType node_type);

    virtual ~StageNode();

    StageNodeType node_type() const;

    bool is_visible() const;

    bool is_intended_visible() const { return is_visible_; }
    void set_visible(bool visible);

    Property<generic::DataCarrier StageNode::*> data = { this, &StageNode::data_ };
    Property<Scene* StageNode::*> scene = { this, &StageNode::owner_ };

    smlt::Promise<void> destroy_after(const Seconds& seconds);

    bool parent_is_scene() const;

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

    void recalc_bounds_if_necessary() const;
    void mark_transformed_aabb_dirty();
private:
    friend class Layer;

    AABB calculate_transformed_aabb() const;
    Scene* owner_ = nullptr;
    StageNodeType node_type_ = STAGE_NODE_TYPE_ACTOR;

    generic::DataCarrier data_;

    /* How many pipelines is this node the root of? */
    uint16_t active_pipeline_count_ = 0;

    bool is_visible_ = true;
    bool self_and_parents_visible_ = true;
    void recalc_visibility();

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

public:
    Transform* get_transform() const {
        return &base_->transform_;
    }

    S_DEFINE_PROPERTY(transform, &StageNode::get_transform);
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
    void do_generate_renderables(batcher::RenderQueue*, const Camera*, const Viewport*, const DetailLevel) override {}

    virtual ~ContainerNode() {}
};

namespace impl {

template<typename F, typename T, typename... Args>
T* mixin_factory(F& factory, StageNode* base, Args&&... args) {
    if(base->is_mixin()) {
        S_WARN("Tried to create nested mixin");
        return nullptr;
    }

    auto type = T::Meta::node_type;
    if(base->mixins_.count(type)) {
        S_WARN("Tried to create duplicate mixin");
        return nullptr;
    }

    auto node = factory->template create_node<T>(std::forward<Args>(args)...);

    base->add_mixin(node);

    return node;
}

}

typedef default_init_ptr<StageNode> StageNodePtr;

}

#include "iterators/sibling_iterator.inc"
#include "iterators/child_iterator.inc"
#include "iterators/descendent_iterator.inc"
#include "iterators/ancestor_iterator.inc"


