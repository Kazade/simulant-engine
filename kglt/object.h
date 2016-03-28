#ifndef KGLT_OBJECT_H_INCLUDED
#define KGLT_OBJECT_H_INCLUDED

#include <iosfwd>
#include <cassert>
#include <vector>
#include <memory>
#include <stdexcept>

#include <kazbase/signals.h>

#include "generic/data_carrier.h"
#include "generic/visitor.h"

#include "kazmath/mat4.h"
#include "kazmath/vec3.h"
#include "kazmath/quaternion.h"
#include "types.h"

#include "scene_node.h"
#include "interfaces.h"
#include "controllers/controller.h"

namespace kglt {

class Object :
    public generic::DataCarrier, //And they allow additional data to be attached
    public Transformable,
    public SceneNode,
    public Controllable {

public:
    Object(Stage* parent_scene);
    virtual ~Object();

    void pre_update(double step) override {
        pre_update_controllers(step);
    }

    void post_update(double step) override {
        post_update_controllers(step);
    }

    void update(double dt) override {
        update_controllers(dt);
		do_update(dt);
        _update_constraint();
	}

	void set_visible(bool value=true) { is_visible_ = value; }
	bool is_visible() const { return is_visible_; }

    virtual void set_absolute_position(float x, float y, float z);
    virtual void set_absolute_position(const kglt::Vec3& pos) { set_absolute_position(pos.x, pos.y, pos.z); }
    virtual kglt::Vec3 absolute_position() const;

    std::pair<Vec3, Vec3> constraint() const;
    bool is_constrained() const;
    void constrain_to(const Vec3& min, const Vec3& max);
    void constrain_to(const AABB& box);
    void disable_constraint();

    ///Transformable interface
    void move_to(const kglt::Vec3& pos) override {
        set_absolute_position(pos);
    }

    void move_to(float x, float y, float z) override {
        set_absolute_position(x, y, z);
    }

    void move_to(float x, float y) override {
        move_to(x, y, 0);
    }

    void move_to(const kglt::Vec2& pos) override {
        move_to(pos.x, pos.y);
    }

    void rotate_to(const Degrees& degrees) override {
        set_absolute_rotation(degrees, 0, 0, 1);
    }

    void rotate_to(const kglt::Quaternion& q) override {
        set_absolute_rotation(q);
    }

    void rotate_to(const Degrees& angle, float x, float y, float z) override {
        set_absolute_rotation(angle, x, y, z);
    }

    void rotate_to(const Degrees& angle, const kglt::Vec3& axis) override {
        rotate_to(angle, axis.x, axis.y, axis.z);
    }

    void rotate_x(const Degrees& angle) override { rotate_absolute_x(angle.value_); }
    void rotate_y(const Degrees& angle) override { rotate_absolute_y(angle.value_); }
    void rotate_z(const Degrees& angle) override { rotate_absolute_z(angle.value_); }

    void look_at(const kglt::Vec3& position);
    void look_at(float x, float y, float z) {
        return look_at(kglt::Vec3(x, y, z));
    }

    // End Transformable Interface

    // Locateable Interface
    Vec3 position() const override { return absolute_position(); }
    Vec2 position_2d() const override { return Vec2(position().x, position().y); }
    Quaternion rotation() const override { return absolute_rotation(); }
    // End Locateable Interface

    // Nameable interface
    void set_name(const unicode &name) { name_ = name; }
    const unicode name() const { return name_; }
    const bool has_name() const { return !name_.empty(); }

    virtual void set_relative_position(float x, float y, float z);
    virtual void set_relative_position(const kglt::Vec3& pos) { set_relative_position(pos.x, pos.y, pos.z); }
    virtual kglt::Vec3 relative_position() const;

    virtual void set_absolute_rotation(const kglt::Quaternion& quaternion);
    virtual void set_absolute_rotation(const Degrees& angle, float x, float y, float z);
    virtual kglt::Quaternion absolute_rotation() const;

    virtual void set_relative_rotation(const kglt::Quaternion& quaternion);
    virtual kglt::Quaternion relative_rotation() const;

    Vec3 right() const {
        Vec3 result;
        Quaternion rot = absolute_rotation();
        kmQuaternionGetRightVec3(&result, &rot);
        return result;
    }

    Vec3 up() const {
        Vec3 result;
        Quaternion rot = absolute_rotation();
        kmQuaternionGetUpVec3(&result, &rot);
        return result;
    }

    Vec3 forward() const {
        Vec3 result;
        Quaternion rot = absolute_rotation();
        kmQuaternionGetForwardVec3RH(&result, &rot);
        return result;
    }

    void move_forward(float amount);

    kglt::Mat4 absolute_transformation() const;

    //Make this object ignore parent rotations or rotate commands until unlocked
    void lock_rotation();
    void unlock_rotation();

    //Make this object ignore parent translations or move commands until unlocked
    void lock_position();
    void unlock_position();

    uint64_t uuid() const { return uuid_; }

    virtual void _initialize() {}
    virtual void do_update(double dt) {}

    Stage* stage() {
        assert(stage_);
        return stage_;
    }

    const Stage* stage() const {
        assert(stage_);
        return stage_;
    }

    void destroy_children();

    bool parent_is_root() const {
        return has_parent() && !parent()->has_parent();
    }

    void _update_constraint();
protected:
    void update_from_parent();

private:
    static uint64_t object_counter;
    uint64_t uuid_;

    Stage* stage_; //Each object is owned by a scene

    kglt::Vec3 relative_position_;
    kglt::Quaternion relative_rotation_;

    kglt::Vec3 absolute_position_;
    kglt::Quaternion absolute_rotation_;

    sig::connection parent_changed_connection_;

    void parent_changed_callback(GenericTreeNode* old_parent, GenericTreeNode* new_parent);

    bool is_visible_;
    bool rotation_locked_;
    bool position_locked_;

    virtual void transformation_changed() {}

    std::unique_ptr<std::pair<Vec3, Vec3>> constraint_;

    virtual void rotate_absolute_x(float amount);
    virtual void rotate_absolute_y(float amount);
    virtual void rotate_absolute_z(float amount);

    unicode name_;
};

}

#endif // OBJECT_H_INCLUDED
