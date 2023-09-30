#pragma once

#include <memory>
#include "../types.h"

namespace smlt {

enum TransformSmoothing {
    TRANSFORM_SMOOTHING_NONE,
    TRANSFORM_SMOOTHING_EXTRAPOLATE,
    TRANSFORM_SMOOTHING_INTERPOLATE
};

enum MovementType {
    MOVEMENT_TYPE_LOCAL,
    MOVEMENT_TYPE_GLOBAL
};

class TransformListener {
public:
    virtual void on_transformation_changed() = 0;
    virtual void on_transformation_change_attempted() = 0;

private:
    friend class Transform;

    /* A weak reference to this is stored in Transform for all registered
     * listeners so we can tell if the listener got destroyed without notice */
    std::shared_ptr<bool> alive_check_ = std::make_shared<bool>();
};

class Transform {
public:
    const Vec3& position() const; // WS position
    const Quaternion& orientation() const; // WS-position

    const Vec3& translation() const; // Local position
    const Quaternion& rotation() const; // Local rotation
    const Vec3& scale_factor() const; // Local scale factor

    /* Set the absolute world-space position of the object */
    void set_position(const Vec3& position);

    /* Set the absolute world-space orientation of the object */
    void set_orientation(const Quaternion& orientation);

    /* Set the translation relative to the parent node. */
    void set_translation(const Vec3& translation);

    /* Set the rotation relative to the parent node */
    void set_rotation(const Quaternion& rotation);

    /* Set scale-factor relative to parent */
    void set_scale_factor(const Vec3& scale);

    /* Change the relative translation by v. If movement type is local
       then v will represent movement along the local x, y, z */
    void translate(const Vec3& v, MovementType movement=MOVEMENT_TYPE_GLOBAL);

    /* Change the relative rotation by q */
    void rotate(const Quaternion& q);
    void rotate(const Vec3& axis, const Degrees& amount, MovementType movement=MOVEMENT_TYPE_GLOBAL);
    void rotate(const Degrees& x, const Degrees& y, const Degrees& z, MovementType movement=MOVEMENT_TYPE_GLOBAL);

    /* Change the relative scale */
    void scale_by(const Vec3& v);
    void scale_by(float xyz);

    /* Return a matrix that can take a local point, and
     * turn it into world space */
    Mat4 world_space_matrix() const;

    /* 2D helpers */
    Vec2 position_2d() const;
    Vec2 translation_2d() const;

    smlt::Degrees orientation_2d() const;
    smlt::Degrees rotation_2d() const;

    void set_position_2d(const Vec2& pos);
    void set_translation_2d(const Vec2& trans);

    void set_orientation_2d(const Degrees& orientation);
    void set_rotation_2d(const Degrees& rotation);

    void set_scale_factor_2d(const Vec2& xy);

    void translate_2d(const Vec2& xy);
    void rotate_2d(const smlt::Degrees& rot);

    Vec3 forward() const;
    Vec3 up() const;
    Vec3 right() const;

    Vec2 up_2d() const;
    Vec2 right_2d() const;

    /* Sets the transform smoothing mode. This is a hint to nodes and
     * services which manipulate the position on whether they should be
     * doing so through interpolation, or extrapolation, or without smoothing
     * at all.
     *
     * Enabling this may be costly depending on the node doing the updating
     * so the default is no smoothing. Returns true if the smoothing mode was
     * changed. */
    bool set_transform_smoothing_mode(TransformSmoothing mode);
    TransformSmoothing transform_smoothing_mode() const;

    bool add_listener(TransformListener* listener);
    bool remove_listener(TransformListener* listener);

    void update_transformation_from_parent();

private:
    /* THis is for access to set_parent primarily */
    friend class StageNode;

    bool has_parent() const { return parent_ != nullptr; }
    void set_parent(Transform* new_parent);

    std::vector<TransformListener*> listeners_;

    void signal_change_attempted();

    void signal_change();

    Transform* parent_ = nullptr;

    TransformSmoothing smoothing_ = TRANSFORM_SMOOTHING_NONE;

    Vec3 position_;
    Quaternion orientation_;

    Vec3 translation_;
    Quaternion rotation_;
    Vec3 scale_factor_;
};

}
