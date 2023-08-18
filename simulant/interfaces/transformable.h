#pragma once

#include "../types.h"
#include "./locateable.h"

namespace smlt {

typedef sig::signal<void ()> TransformationChangedSignal;


/**
 * @brief The Transformable class
 *
 * An interface that describes objects that can be moved and rotated
 */
class Transformable:
    public Locateable {

    DEFINE_SIGNAL(TransformationChangedSignal, signal_transformation_changed);

public:
    virtual ~Transformable() {}

    smlt::Vec3 position() const { return position_; }
    smlt::Vec2 position_2d() const { return Vec2(position_.x, position_.y); }
    smlt::Quaternion rotation() const { return rotation_; }
    smlt::Vec3 scale() const { return scaling_; }

    virtual void transform_to(const smlt::Mat4& transform);

    virtual void move_to(const smlt::Vec3& pos);
    virtual void move_to(const smlt::Vec2& pos);
    virtual void move_to(float x, float y, float z);
    virtual void move_to(float x, float y);

    virtual void move_by(const smlt::Vec3& pos);
    virtual void move_by(const smlt::Vec2& pos);
    virtual void move_by(float x, float y, float z);
    virtual void move_by(float x, float y);

    virtual void move_forward_by(float amount);
    virtual void move_right_by(float amount);
    virtual void move_up_by(float amount);

    virtual void rotate_to(const smlt::Degrees& angle, float axis_x, float axis_y, float axis_z);
    virtual void rotate_to(const smlt::Degrees& angle, const smlt::Vec3& axis);
    virtual void rotate_to(const smlt::Quaternion& rotation);

    virtual void rotate_x_by(const smlt::Degrees& angle);
    virtual void rotate_y_by(const smlt::Degrees& angle);
    virtual void rotate_z_by(const smlt::Degrees& angle);
    virtual void rotate_by(const smlt::Degrees& angle_x, const smlt::Degrees& angle_y, const smlt::Degrees& angle_z);
    virtual void rotate_by(const smlt::Vec3& angles);

    virtual void scale_x_by(const float scale);
    virtual void scale_y_by(const float scale);
    virtual void scale_z_by(const float scale);
    virtual void scale_by(float x);
    virtual void scale_by(const Vec3& x);

    virtual void scale_to(const Vec3& x);
    virtual void scale_to(const float x, const float y, const float z);

    virtual void rotate_around(const smlt::Vec3& axis, const smlt::Degrees& degrees);

    void rotate_global_x_by(const smlt::Degrees& degrees);
    void rotate_global_y_by(const smlt::Degrees& degrees);
    void rotate_global_z_by(const smlt::Degrees& degrees);

    virtual void look_at(const smlt::Vec3& target, const Vec3 &up=Vec3::POSITIVE_Y);
    virtual void look_at(float x, float y, float z, const Vec3 &up=Vec3::POSITIVE_Y);

    Vec3 right() const;
    Vec3 up() const;
    Vec3 forward() const;

    void constrain_to_aabb(const AABB& region);
    bool is_constrained() const;
    void remove_constraint();

protected:
    void set_position(const Vec3& p);
    void set_rotation(const Quaternion& q);
    void set_scaling(const Vec3& s);

    virtual void on_transformation_changed() {}

    /* Called when a transformation is attempted, even if it doesn't
     * result in any change */
    virtual void on_transformation_change_attempted() {}

    Vec3 position_;
    Quaternion rotation_;
    Vec3 scaling_ = Vec3(1, 1, 1);

    std::unique_ptr<AABB> constraint_;
};

}
