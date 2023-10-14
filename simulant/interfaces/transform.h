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

enum TransformRetainMode {
    TRANSFORM_RETAIN_MODE_KEEP,
    TRANSFORM_RETAIN_MODE_LOSE,
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
    const Vec3& position() const {
        return position_;
    }

    const Quaternion& orientation() const {
        return orientation_;
    }

    const Vec3& translation() const {
        return translation_;
    }

    // Local rotation
    const Quaternion& rotation() const {
        return rotation_;
    }

    // Local scale factor
    const Vec3& scale_factor() const {
        return scale_factor_;
    }

    /* Set the absolute world-space position of the object */
    void set_position(const Vec3& position);

    /* Set the absolute world-space orientation of the object */
    void set_orientation(const Quaternion& orientation);

    /* Set the translation relative to the parent node. */
    void set_translation(const Vec3& translation);

    /* Set the rotation relative to the parent node */
    void set_rotation(const Quaternion& rotation);

    /* Set scale-factor relative to parent */
    void set_scale_factor(const Vec3& scale) {
        scale_factor_ = scale;
        signal_change();
    }

    /* Change the relative translation by v */
    void translate(const Vec3& v) {
        translation_ += v;
        signal_change();
    }

    /* Change the relative rotation by q */
    void rotate(const Quaternion& q);

    void rotate(const Vec3& axis, const Degrees& amount);

    void rotate(const Degrees& x, const Degrees& y, const Degrees& z);

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

    Vec3 forward() const {
        return orientation_.forward();
    }

    Vec3 up() const {
        return orientation_.up();
    }

    Vec3 right() const {
        return orientation_.right();
    }

    Vec2 up_2d() const {
        return up().xy();
    }

    Vec2 right_2d() const {
        return right().xy();
    }

    /* Sets the transform smoothing mode. This is a hint to nodes and
     * services which manipulate the position on whether they should be
     * doing so through interpolation, or extrapolation, or without smoothing
     * at all.
     *
     * Enabling this may be costly depending on the node doing the updating
     * so the default is no smoothing. Returns true if the smoothing mode was
     * changed. */
    bool set_smoothing_mode(TransformSmoothing mode) {
        bool ret = mode != smoothing_;
        smoothing_ = mode;
        return ret;
    }

    TransformSmoothing smoothing_mode() const {
        return smoothing_;
    }

    bool add_listener(TransformListener* listener);
    bool remove_listener(TransformListener* listener);

    void update_transformation_from_parent();
    void sync(const Transform* other);
    void look_at(const Vec3& target, const Vec3& up=Vec3::POSITIVE_Y);

private:
    /* THis is for access to set_parent primarily */
    friend class StageNode;

    bool has_parent() const { return parent_ != nullptr; }
    void set_parent(Transform* new_parent, TransformRetainMode retain_mode=TRANSFORM_RETAIN_MODE_LOSE);

    std::vector<TransformListener*> listeners_;

    void signal_change_attempted();

    void signal_change();

    Transform* parent_ = nullptr;

    TransformSmoothing smoothing_ = TRANSFORM_SMOOTHING_NONE;

    Vec3 position_;
    Quaternion orientation_;

    Vec3 translation_;
    Quaternion rotation_;
    Vec3 scale_factor_ = Vec3(1);

    mutable Mat4 absolute_transformation_;
    mutable bool absolute_transformation_is_dirty_ = false;

    void set_translation_if_necessary(const Vec3& trans);
    void set_rotation_if_necessary(const Quaternion& rot);
};

}
