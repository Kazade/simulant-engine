#pragma once

#include "../types.h"

namespace smlt {

struct Sphere {
    float radius = 0.0f;
    smlt::Vec3 center;
};

class SphereApproximation {
private:
    std::vector<Sphere> spheres_;

    friend class Mesh;

    SphereApproximation(const std::vector<Sphere>& spheres):
        spheres_(spheres) {}

public:
    std::size_t sphere_count() const {
        return spheres_.size();
    }

    const Sphere* sphere(std::size_t i) const {
        return &spheres_.at(i);
    }
};

}
