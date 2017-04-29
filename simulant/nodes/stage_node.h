#pragma once

#include "./tree_node.h"
#include "../interfaces/nameable.h"
#include "../interfaces/printable.h"
#include "../interfaces/transformable.h"
#include "../interfaces/updateable.h"
#include "../interfaces/ownable.h"
#include "../interfaces/boundable.h"
#include "../controllers/controller.h"
#include "../interfaces/has_auto_id.h"
#include "../generic/data_carrier.h"

namespace smlt {

typedef sig::signal<void (AABB)> BoundsUpdatedSignal;

class StageNode:
    public TreeNode,
    public Nameable,
    public Printable,
    public Transformable,
    public Updateable,
    public Ownable,
    public BoundableEntity,
    public Controllable,
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
    const AABB& bounds() const { return bounds_; }

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
protected:
    // Faster than properties, useful for subclasses where a clean API isn't as important
    Stage* get_stage() const { return stage_; }

private:
    void on_position_set(const Vec3& oldp, const Vec3& newp) override;
    void on_rotation_set(const Quaternion& oldr, const Quaternion& newr) override;
    void on_scaling_set(const Vec3& olds, const Vec3& news) override;
    void on_parent_set(TreeNode* oldp, TreeNode* newp) override;

    void update_rotation_from_parent();
    void update_position_from_parent();
    void update_scaling_from_parent();
    void recalc_bounds();

    Stage* stage_ = nullptr;

    generic::DataCarrier data_;

    bool is_visible_ = true;

    Vec3 absolute_position_;
    Quaternion absolute_rotation_;
    Vec3 absolute_scale_ = Vec3(1, 1, 1);

    AABB bounds_;
};

}
