#pragma once

#include "../generic/optional.h"

#include "vec3.h"

namespace smlt {

struct Vec3;

enum PlaneClassification {
    PLANE_CLASSIFICATION_IS_BEHIND_PLANE,
    PLANE_CLASSIFICATION_IS_ON_PLANE,
    PLANE_CLASSIFICATION_IS_IN_FRONT_OF_PLANE
};


struct Plane {
    Vec3 n;
    float d;

    Plane():
        n(Vec3()),
        d(0) {
    }

    Plane(const Vec3& N, float D):
        n(N),
        d(D) {

    }

    Plane(float A, float B, float C, float D):
        n(A, B, C),
        d(D) {

    }

    Vec3 project(const Vec3& p);

    Vec3 normal() const {
        return n;
    }

    float distance_to(const Vec3& p);

    PlaneClassification classify_point(const Vec3& p) const;

    static smlt::optional<Vec3> intersect_planes(
        const Plane& p1,
        const Plane& p2,
        const Plane& p3
    );
};


}
