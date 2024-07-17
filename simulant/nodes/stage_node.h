#pragma once

#include <functional>
#include <queue>

#include "../coroutines/helpers.h"
#include "../generic/data_carrier.h"
#include "../generic/manual_object.h"
#include "../interfaces/boundable.h"
#include "../interfaces/has_auto_id.h"
#include "../interfaces/nameable.h"
#include "../interfaces/printable.h"
#include "../interfaces/transform.h"
#include "../interfaces/updateable.h"
#include "../shadows.h"
#include "../sound.h"

#include "iterators/ancestor_iterator.h"
#include "iterators/child_iterator.h"
#include "iterators/descendent_iterator.h"
#include "iterators/sibling_iterator.h"

#include "../texture.h"
#include "builtins.h"
#include "simulant/generic/any/any.h"
#include "simulant/generic/managed.h"
#include "simulant/utils/params.h"

namespace smlt {

class RenderableFactory;
class Seconds;
class Scene;
struct GeomCullerOptions;
struct TextureFlags;
class StageNode;
namespace ui {
struct UIConfig;
struct WidgetStyle;
typedef std::shared_ptr<WidgetStyle> WidgetStylePtr;
} // namespace ui

typedef sig::signal<void(AABB)> BoundsUpdatedSignal;
typedef sig::signal<void()> CleanedUpSignal;

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

} // namespace impl

enum NodeParamType {
    NODE_PARAM_TYPE_FLOAT,
    NODE_PARAM_TYPE_FLOAT_ARRAY,
    NODE_PARAM_TYPE_INT,
    NODE_PARAM_TYPE_INT_ARRAY,
    NODE_PARAM_TYPE_BOOL,
    NODE_PARAM_TYPE_BOOL_ARRAY,
    NODE_PARAM_TYPE_STRING,
    NODE_PARAM_TYPE_MESH_PTR,
    NODE_PARAM_TYPE_TEXTURE_PTR,
    NODE_PARAM_TYPE_PARTICLE_SCRIPT_PTR,
    NODE_PARAM_TYPE_STAGE_NODE_PTR,
    // FIXME: Ideally these wouldn't exist and instead
    // widgets would take base types as arguments
    NODE_PARAM_TYPE_UI_CONFIG,
    NODE_PARAM_TYPE_WIDGET_STYLE_PTR,
    NODE_PARAM_TYPE_GEOM_CULLER_OPTS,
    NODE_PARAM_TYPE_TEXTURE_FLAGS,
};

template<typename T>
struct type_to_node_param_type;

template<>
struct type_to_node_param_type<float> {
    static const NodeParamType value = NODE_PARAM_TYPE_FLOAT;
};

template<>
struct type_to_node_param_type<FloatArray> {
    static const NodeParamType value = NODE_PARAM_TYPE_FLOAT_ARRAY;
};

template<>
struct type_to_node_param_type<int> {
    static const NodeParamType value = NODE_PARAM_TYPE_INT;
};

template<>
struct type_to_node_param_type<IntArray> {
    static const NodeParamType value = NODE_PARAM_TYPE_INT_ARRAY;
};

template<>
struct type_to_node_param_type<bool> {
    static const NodeParamType value = NODE_PARAM_TYPE_BOOL;
};

template<>
struct type_to_node_param_type<BoolArray> {
    static const NodeParamType value = NODE_PARAM_TYPE_BOOL_ARRAY;
};

template<>
struct type_to_node_param_type<std::string> {
    static const NodeParamType value = NODE_PARAM_TYPE_STRING;
};

template<>
struct type_to_node_param_type<MeshPtr> {
    static const NodeParamType value = NODE_PARAM_TYPE_MESH_PTR;
};

template<>
struct type_to_node_param_type<TexturePtr> {
    static const NodeParamType value = NODE_PARAM_TYPE_TEXTURE_PTR;
};

template<>
struct type_to_node_param_type<ParticleScriptPtr> {
    static const NodeParamType value = NODE_PARAM_TYPE_PARTICLE_SCRIPT_PTR;
};

template<>
struct type_to_node_param_type<ui::UIConfig> {
    static const NodeParamType value = NODE_PARAM_TYPE_UI_CONFIG;
};

template<>
struct type_to_node_param_type<ui::WidgetStylePtr> {
    static const NodeParamType value = NODE_PARAM_TYPE_WIDGET_STYLE_PTR;
};

template<>
struct type_to_node_param_type<GeomCullerOptions> {
    static const NodeParamType value = NODE_PARAM_TYPE_GEOM_CULLER_OPTS;
};

template<>
struct type_to_node_param_type<TextureFlags> {
    static const NodeParamType value = NODE_PARAM_TYPE_TEXTURE_FLAGS;
};

template<>
struct type_to_node_param_type<StageNode*> {
    static const NodeParamType value = NODE_PARAM_TYPE_STAGE_NODE_PTR;
};

template<NodeParamType T>
struct node_param_type_to_type;

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_FLOAT> {
    typedef float type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_FLOAT_ARRAY> {
    typedef FloatArray type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_INT> {
    typedef int type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_INT_ARRAY> {
    typedef IntArray type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_BOOL> {
    typedef bool type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_BOOL_ARRAY> {
    typedef BoolArray type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_STRING> {
    typedef std::string type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_MESH_PTR> {
    typedef MeshPtr type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_TEXTURE_PTR> {
    typedef TexturePtr type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_PARTICLE_SCRIPT_PTR> {
    typedef ParticleScriptPtr type;
};

template<>
struct node_param_type_to_type<NODE_PARAM_TYPE_STAGE_NODE_PTR> {
    typedef StageNode* type;
};

constexpr bool has_spaces(const char* s) {
    return *s && (*s == ' ' || has_spaces(s + 1));
}

class NodeParam {
public:
    NodeParam(int order, const char* name, NodeParamType type,
              optional<ParamValue> default_value, const char* desc) :
        order_(order),
        name_(name),
        type_(type),
        default_value_(default_value),
        desc_(desc) {}

    bool operator<(const NodeParam& rhs) const {
        return order_ < rhs.order_;
    }

    const char* name() const {
        return name_;
    }

    NodeParamType type() const {
        return type_;
    }

    const char* description() const {
        return desc_;
    }

    optional<ParamValue> default_value() const {
        return default_value_;
    }

private:
    int order_;
    const char* name_;
    NodeParamType type_;
    optional<ParamValue> default_value_;
    const char* desc_;
};

template<typename T>
std::set<NodeParam>& get_node_params() {
    static std::set<NodeParam> properties;
    return properties;
}

template<typename T, typename C>
class TypedNodeParam {
public:
    typedef T type;

    template<typename F>
    TypedNodeParam(int order, const char* name, const F& fallback,
                   const char* desc) :
        param_(NodeParam(order, name, type_to_node_param_type<T>::value,
                         to_param(fallback), desc)) {

        get_node_params<C>().insert(param_);
    }

    const NodeParam& param() const {
        return param_;
    }

private:
    template<typename F>
    optional<ParamValue> to_param(const F& fallback) {
        /* We abuse the default coersion rules of Params */
        Params tmp;
        tmp.set("value", fallback);
        return tmp.raw("value");
    }

    optional<ParamValue> to_param(const OptionalInit&) {
        return no_value;
    }

    optional<ParamValue> to_param(const std::nullptr_t&) {
        return no_value;
    }

    NodeParam param_;
};

/**
    Defines a new parameter for a stage node:

    - name: this is the name of the parameter, it should be a valid variable
            name, ideally in snake-case format.
    - type: this is the type of the parameter, e.g. float, int, TexturePtr etc.
    - fallback: if provided this will be the default value for the parameter, if
                the parameter is required, you should pass no_value
    - desc: A short description < 64 chars
*/
#define __S_GEN_PARAM(param, line) param##line
#define _S_GEN_PARAM(param, line) __S_GEN_PARAM(param, line)

#define _S_DEFINE_STAGE_NODE_PARAM(line, klass, name, type, fallback, desc)    \
    static_assert(!smlt::has_spaces(name), "Param name must not have spaces"); \
    static_assert(std::is_same<type, int>::value ||                            \
                  std::is_same<type, smlt::IntArray>::value ||                 \
                  std::is_same<type, float>::value ||                          \
                  std::is_same<type, smlt::FloatArray>::value ||               \
                  std::is_same<type, smlt::ParticleScriptPtr>::value ||        \
                  std::is_same<type, smlt::MeshPtr>::value ||                  \
                  std::is_same<type, smlt::GeomCullerOptions>::value ||        \
                  std::is_same<type, std::string>::value ||                    \
                  std::is_same<type, smlt::TextureFlags>::value ||             \
                  std::is_same<type, smlt::TexturePtr>::value ||               \
                  std::is_same<type, smlt::ui::UIConfig>::value ||             \
                  std::is_same<type, smlt::StageNode*>::value ||               \
                  std::is_same<type, smlt::ui::WidgetStylePtr>::value);        \
    static inline auto _S_GEN_PARAM(param_, line) =                            \
        smlt::TypedNodeParam<type, klass>(line, name, fallback, desc)

#define S_DEFINE_STAGE_NODE_PARAM(klass, name, type, fallback, desc)           \
    _S_DEFINE_STAGE_NODE_PARAM(__LINE__, klass, name, type, fallback, desc)

/* We need to coerce const char* into the correct string class */
template<typename T>
void params_set(Params& params, const NodeParam& p, T x) {
    params.set(p.name(), x);
}

inline void params_set(Params& params, const NodeParam& p, const char* x) {
    params.set(p.name(), std::string(x));
}

/* Simple coercion via copy constructor */
template<typename Source, typename Dest>
optional<Dest> do_coerce(const any& in) {
    try {
        Source s = any_cast<Source>(in);
        return (Dest)s;
    } catch(bad_any_cast&) {
        return no_value;
    }
}

template<typename T>
void params_unpack(Params& params, std::set<NodeParam>::iterator it,
                   std::set<NodeParam>::iterator end, T x) {
    if(it == end) {
        return;
    }

    params_set(params, *it, x);
}

template<typename T, typename... Args>
void params_unpack(Params& params, std::set<NodeParam>::iterator it,
                   std::set<NodeParam>::iterator end, T x, Args&&... args) {
    if(it == end) {
        S_WARN("Ignoring additional unknown parameters");
        return;
    }

    params_set(params, *it, x);

    params_unpack(params, ++it, end, std::forward<Args>(args)...);
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
    DEFINE_SIGNAL(CleanedUpSignal,
                  signal_cleaned_up); // Fired when the node is cleaned up
                                      // later, following destroy

private:
    /* Heirarchy */

    friend class DescendentIterator<false>;
    friend class DescendentIterator<true>;

    StageNode* parent_ = nullptr;
    StageNode* next_ = nullptr;
    StageNode* prev_ = nullptr;
    StageNode* first_child_ = nullptr;
    StageNode* last_child_ = nullptr;

    virtual void on_parent_set(StageNode* oldp, StageNode* newp,
                               TransformRetainMode transform_retain) {
        _S_UNUSED(oldp);

        if(newp) {
            transform->set_parent(newp->transform, transform_retain);
        } else {
            transform->set_parent(nullptr);
        }

        recalc_visibility();
    }

public:
    std::vector<StageNode*> find_descendents_by_types(
        std::initializer_list<StageNodeType> type_list) const;

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
        TransformRetainMode transform_retain = TRANSFORM_RETAIN_MODE_LOSE);

    template<typename T, typename... Args>
    T* create_child(Args&&... args) {
        Params params;

        auto node_params = get_node_params<T>();
        params_unpack(params, node_params.begin(), node_params.end(),
                      std::forward<Args>(args)...);

        return impl::child_factory<decltype(owner_), T>(
            owner_, this, std::forward<const Params&>(params));
    }

    template<typename T>
    T* create_child() {
        Params args;
        return impl::child_factory<decltype(owner_), T>(
            owner_, this, std::forward<const Params&>(args));
    }

    template<typename T>
    T* create_child(Params args) {
        return impl::child_factory<decltype(owner_), T>(
            owner_, this, std::forward<Params>(args));
    }

    void adopt_children(StageNode* node) {
        node->set_parent(this);
    }

    template<typename... Params>
    void adopt_children(StageNode* node, Params... args) {
        node->set_parent(this);
        adopt_children(args...);
    }

    template<typename T, typename... Args>
    T* create_mixin(Args&&... args) {
        return impl::mixin_factory<decltype(owner_), T, Args...>(
            owner_, this, std::forward<Args>(args)...);
    }

    StageNode* create_mixin(const std::string& node_name, const Params& params);

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
    void generate_renderables(batcher::RenderQueue* render_queue, const Camera*,
                              const Viewport* viewport,
                              const DetailLevel detail_level);

    void update(float dt) override final;
    void late_update(float dt) override final;
    void fixed_update(float step) override final;

    /** This is a very-slow utility function that calls
     *  dynamic_cast on the entire subtree. Use for testing
     *  *only*. */
    template<typename T>
    size_t count_nodes_by_type(bool include_destroyed = false) const {
        size_t ret = 0;
        for(auto& node: each_descendent()) {
            if(node.is_destroyed() && !include_destroyed) {
                continue;
            }

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
    virtual bool on_create(Params params) = 0;
    virtual bool on_destroy() override {
        return true;
    }
    virtual void on_update(float dt) override {
        _S_UNUSED(dt);
    }
    virtual void on_fixed_update(float step) override {
        _S_UNUSED(step);
    }
    virtual void on_late_update(float dt) override {
        _S_UNUSED(dt);
    }

    void on_transformation_changed() override {
        mark_transformed_aabb_dirty();

        /* If this node's transform changes in some way, we need
         * to trigger updates on child transforms too */
        for(auto& child: each_child()) {
            child.transform->signal_change();
        }
    }

    void on_transformation_change_attempted() override {}

protected:
    template<typename N>
    bool clean_params(Params& params) {
        Params cleaned;
        for(auto param: get_node_params<N>()) {
            auto name = param.name();
            bool passed = params.contains(name);
            if(!passed && !param.default_value()) {
                // No default and not provided
                return false;
            } else if(passed) {
                cleaned.set(name, params.raw(name).value());
            } else {
                cleaned.set(name, param.default_value().value());
            }
        }

        params = cleaned;
        return true;
    }

private:
    friend class StageNodeManager;

    Transform transform_;

    // NVI idiom
    bool _create(const Params& params) {
        return on_create(params);
    }

    void _clean_up() override;

    virtual bool do_generates_renderables_for_descendents() const {
        return false;
    }

    /* Return a list of renderables to pass into the render queue */
    virtual void do_generate_renderables(batcher::RenderQueue* render_queue,
                                         const Camera*,
                                         const Viewport* viewport,
                                         const DetailLevel detail_level) {
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

        SiblingIteratorPair(const StageNode* root) :
            root_(root) {}

        const StageNode* root_;

    public:
        SiblingIterator<false> begin() {
            if(!root_->parent()) {
                return SiblingIterator<false>(root_, nullptr);
            }

            return SiblingIterator<false>(
                root_, (root_->next_sibling()) ? root_->next_sibling()
                                               : root_->parent_->first_child());
        }

        SiblingIterator<false> end() {
            return SiblingIterator<false>(root_, nullptr);
        }
    };

    class ChildIteratorPair {
        friend class StageNode;

        ChildIteratorPair(const StageNode* root) :
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

        DescendentIteratorPair(const StageNode* root) :
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

        AncestorIteratorPair(const StageNode* root) :
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

    bool is_intended_visible() const {
        return is_visible_;
    }
    void set_visible(bool visible);

    Property<generic::DataCarrier StageNode::*> data = {this,
                                                        &StageNode::data_};
    Property<Scene * StageNode::*> scene = {this, &StageNode::owner_};

    smlt::Promise<void> destroy_after(const Seconds& seconds);

    bool parent_is_scene() const;

    const AABB transformed_aabb() const override;

    /* Default implementation, assume nodes have no volume unless they do */
    const AABB& aabb() const override {
        static auto zero = AABB::zero();
        return zero;
    }

    /* Control shading on the stage node (behaviour depends on the type of node)
     */
    ShadowCast shadow_cast() const {
        return shadow_cast_;
    }
    void set_shadow_cast(ShadowCast cast) {
        shadow_cast_ = cast;
    }

    ShadowReceive shadow_receive() const {
        return shadow_receive_;
    }
    void set_shadow_receive(ShadowReceive receive) {
        shadow_receive_ = receive;
    }

    StageNode* find_descendent_with_name(const std::string& name);

    void set_cullable(bool v);
    bool is_cullable() const;

    void set_precedence(int16_t precedence);

    int16_t precedence() const;

protected:
    // Faster than properties, useful for subclasses where a clean API isn't as
    // important
    Scene* get_scene() const {
        return owner_;
    }

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

    /* Whether or not this node should be culled by the partitioner (e.g. when
     * offscreen) */
    bool cullable_ = true;

    /* Passed to coroutines and used to detect when the object has been
     * destroyed */
    std::shared_ptr<bool> alive_marker_ = std::make_shared<bool>(true);

    int16_t precedence_ = 0;

public:
    Transform* get_transform() const {
        return &base_->transform_;
    }

    S_DEFINE_PROPERTY(transform, &StageNode::get_transform);
};

class StageNodeVisitorBFS {
public:
    template<typename Func>
    StageNodeVisitorBFS(StageNode* start, Func&& callback) :
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
    std::function<void(StageNode*)> callback_;
    std::queue<StageNode*> queue_;
};

class ContainerNode: public StageNode {
public:
    ContainerNode(Scene* scene, StageNodeType node_type) :
        StageNode(scene, node_type) {}

    /* Containers don't directly have renderables, but their children do */
    void do_generate_renderables(batcher::RenderQueue*, const Camera*,
                                 const Viewport*, const DetailLevel) override {}

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

} // namespace impl

typedef StageNode* StageNodePtr;

} // namespace smlt

#include "iterators/ancestor_iterator.inc"
#include "iterators/child_iterator.inc"
#include "iterators/descendent_iterator.inc"
#include "iterators/sibling_iterator.inc"

#define S_DEFINE_STAGE_NODE_META(node_type_id, alias)                          \
    struct Meta {                                                              \
        const static smlt::StageNodeType node_type = node_type_id;             \
        inline static const char* name = alias;                                \
    };                                                                         \
    const char* node_type_name() const {                                       \
        return Meta::name;                                                     \
    }                                                                          \
    struct _unused_ {}
