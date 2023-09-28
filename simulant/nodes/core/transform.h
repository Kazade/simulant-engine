#pragma once

namespace smlt {

enum TransformSmoothing {
    TRANSFORM_SMOOTHING_NONE,
    TRANSFORM_SMOOTHING_EXTRAPOLATE,
    TRANSFORM_SMOOTHING_INTERPOLATE
};

class Transform {
    TransformSmoothing smoothing = TRANSFORM_SMOOTHING_NONE;

    const Vec3& position() const; // WS position
    const Quaternion& orientation() const; // WS-position

    const Vec3& translation() const; // Local position
    const Quaterion& rotation() const; // Local rotation

    void translate(const Vec3& v);
    void rotate(const Quaternion& q);
};

}
