#pragma once

#include "./tree_node.h"
#include "../interfaces/nameable.h"
#include "../interfaces/printable.h"
#include "../interfaces/transformable.h"
#include "../interfaces/updateable.h"
#include "../interfaces/ownable.h"
#include "../interfaces/boundable.h"
#include "../behaviours/behaviour.h"
#include "../interfaces/has_auto_id.h"
#include "../generic/data_carrier.h"
#include "../shadows.h"

namespace smlt {

typedef sig::signal<void (AABB)> BoundsUpdatedSignal;

typedef std::vector<std::shared_ptr<Renderable>> RenderableList;

/* Used for multiple levels of detail when rendering stage nodes */

enum DetailLevel {
    DETAIL_LEVEL_NEAREST = 0,
    DETAIL_LEVEL_NEAR,
    DETAIL_LEVEL_MID,
    DETAIL_LEVEL_FAR,
    DETAIL_LEVEL_FARTHEST,
    DETAIL_LEVEL_MAX
};

class StageNode:
    public TreeNode,
    public Nameable,
    public Printable,
    public Transformable,
    public Updateable,
    public Ownable,
    public virtual BoundableEntity,
    public Organism,
    public HasAutoID<StageNode> {


    DEFINE_SIGNAL(BoundsUpdatedSignal, signal_bounds_updated);

public:
    unicode to_unicode() const override {
        return Nameable::to_unicode();
    }

    StageNode(Stage* stage);
    virtual ~StageNode() {}


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

    bool is_visible() const { return is_visible_; }
    void set_visible(bool visible) { is_visible_ = visible; }

    Property<StageNode, generic::DataCarrier> data = { this, &StageNode::data_ };
    Property<StageNode, Stage> stage = { this, &StageNode::stage_ };

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

    void cleanup();

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
    virtual RenderableList _get_renderables(
        const smlt::Frustum& frustum,
        DetailLevel detail_level
    ) const = 0;

protected:
    // Faster than properties, useful for subclasses where a clean API isn't as important
    Stage* get_stage() const { return stage_; }

    void on_transformation_changed() override;
    void on_parent_set(TreeNode* oldp, TreeNode* newp) override;

    virtual void update_transformation_from_parent();

private:
    AABB calculate_transformed_aabb() const;
    void recalc_bounds();

    Stage* stage_ = nullptr;

    generic::DataCarrier data_;

    bool is_visible_ = true;

    Vec3 absolute_position_;
    Quaternion absolute_rotation_;
    Vec3 absolute_scale_ = Vec3(1, 1, 1);

    AABB transformed_aabb_;

    // By default, always cast and receive shadows
    ShadowCast shadow_cast_ = SHADOW_CAST_ALWAYS;
    ShadowReceive shadow_receive_ = SHADOW_RECEIVE_ALWAYS;
};


class ContainerNode : public StageNode {
public:
    ContainerNode(Stage* stage):
        StageNode(stage) {}

    /* Containers don't directly have renderables, but their children do */
    std::vector<std::shared_ptr<Renderable>> _get_renderables(const Frustum& frustum, DetailLevel detail_level) const {
        return std::vector<std::shared_ptr<Renderable>>();
    }

    virtual ~ContainerNode() {}
};

}
