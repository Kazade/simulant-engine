#ifndef KGLT_OBJECT_H_INCLUDED
#define KGLT_OBJECT_H_INCLUDED

#include <iosfwd>
#include <cassert>
#include <vector>
#include <memory>
#include <stdexcept>

#include <kazsignal/kazsignal.h>

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

class Object:
    public SceneNode,
    public Controllable  {
public:
    Object(Stage* stage):
        stage_(stage),
        uuid_(++object_counter) {

        absolute_position_ = Vec3(0, 0, 0);
        absolute_rotation_ = Quaternion(0, 0, 0, 1);
        relative_position_ = Vec3(0, 0, 0);
        relative_rotation_ = Quaternion(0, 0, 0, 1);
    }

    Object(Stage* stage, const Vec3& position, const Quaternion& rotation):
        relative_position_(position),
        relative_rotation_(rotation),
        absolute_position_(position),
        absolute_rotation_(rotation),
        stage_(stage),
        uuid_(++object_counter) {

    }

    virtual ~Object() {}

    // Nameable interface
    void set_name(const unicode &name) override { name_ = name; }
    const unicode name() const override { return name_; }
    const bool has_name() const override { return !name_.empty(); }

    Property<Object, Stage> stage = { this, &Object::stage_ };
    uint64_t uuid() const { return uuid_; }

    // Locateable Interface
    Vec3 position() const override { return absolute_position(); }
    Vec2 position_2d() const override { return Vec2(position().x, position().y); }
    Quaternion rotation() const override { return absolute_rotation(); }
    // End Locateable Interface

    virtual kglt::Vec3 absolute_position() const {
        return absolute_position_;
    }

    virtual kglt::Quaternion absolute_rotation() const {
        return absolute_rotation_;
    }

    virtual kglt::Quaternion relative_rotation() const {
        return relative_rotation_;
    }

    virtual kglt::Vec3 relative_position() const {
        return relative_position_;
    }

    void pre_update(double step) override {
        pre_update_controllers(step);
    }

    void post_update(double step) override {
        post_update_controllers(step);
    }

    void update(double dt) override {
        update_controllers(dt);
        do_update(dt);
    }

    void pre_fixed_update(double step) override {
        pre_fixed_update_controllers(step);
    }

    void post_fixed_update(double step) override {
        post_fixed_update_controllers(step);
    }

    void fixed_update(double dt) override {
        fixed_update_controllers(dt);
    }

    Property<Object, generic::DataCarrier> data = { this, &Object::data_ };

protected:
    kglt::Vec3 relative_position_;
    kglt::Quaternion relative_rotation_;

    kglt::Vec3 absolute_position_;
    kglt::Quaternion absolute_rotation_;

private:
    Stage* stage_ = nullptr; //Each object is owned by a stage

    static uint64_t object_counter;
    uint64_t uuid_;

    unicode name_;

    virtual void do_update(double dt) {}

    generic::DataCarrier data_;
};

typedef sig::signal<void (const Vec3&, const Quaternion&)> TransformationChangedSignal;

class MoveableObject :
    public Object,
    public Transformable {

    DEFINE_SIGNAL(TransformationChangedSignal, signal_transformation_changed);

public:
    MoveableObject(Stage* parent_scene);
    virtual ~MoveableObject();

    void update(double dt) override {
        Object::update(dt);
        _update_constraint();
	}

	void set_visible(bool value=true) { is_visible_ = value; }
	bool is_visible() const { return is_visible_; }

    virtual void set_absolute_position(float x, float y, float z);
    virtual void set_absolute_position(const kglt::Vec3& pos) { set_absolute_position(pos.x, pos.y, pos.z); }

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

    void rotate_x(const Degrees& angle) override {
        kglt::Vec3 x = right();
        rotate_around(x, angle);
    }

    void rotate_y(const Degrees& angle) override {
        kglt::Vec3 y = up();
        rotate_around(y, angle);
    }

    void rotate_z(const Degrees& angle) override {
        kglt::Vec3 z = forward();
        rotate_around(z, angle);
    }

    void rotate_around(const kglt::Vec3& axis, const kglt::Degrees& degrees) override;

    void look_at(const kglt::Vec3& position) override;
    void look_at(float x, float y, float z) override {
        return look_at(kglt::Vec3(x, y, z));
    }

    Quaternion calc_look_at_rotation(const Vec3& target);

    // End Transformable Interface


    virtual void set_relative_position(float x, float y, float z);
    virtual void set_relative_position(const kglt::Vec3& pos) { set_relative_position(pos.x, pos.y, pos.z); }

    virtual void set_absolute_rotation(const kglt::Quaternion& quaternion);
    virtual void set_absolute_rotation(const Degrees& angle, float x, float y, float z);

    virtual void set_relative_rotation(const kglt::Quaternion& quaternion);

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

    virtual void _initialize() {}    

    void destroy_children();

    bool parent_is_root() const {
        return has_parent() && !parent()->has_parent();
    }

    void _update_constraint();
protected:
    void update_from_parent();

private:
    sig::connection parent_changed_connection_;

    void parent_changed_callback(GenericTreeNode* old_parent, GenericTreeNode* new_parent);

    bool is_visible_;
    bool rotation_locked_;
    bool position_locked_;

    virtual void transformation_changed() {}

    std::unique_ptr<std::pair<Vec3, Vec3>> constraint_;
};

}

#endif // OBJECT_H_INCLUDED
