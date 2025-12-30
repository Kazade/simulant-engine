#pragma once

namespace smlt {

struct PhysicsMaterial {
    PhysicsMaterial() = default;
    PhysicsMaterial(float density, float friction, float bounciness) :
        density(density), friction(friction), bounciness(bounciness) {}

    float density = 0.0f;
    float friction = 0.0f;
    float bounciness = 0.0f;

    static PhysicsMaterial wood(float density_multiplier = 1.0f) {
        return PhysicsMaterial(0.005f * density_multiplier, 0.4f, 0.2f);
    }

    static PhysicsMaterial rubber(float density_multiplier = 1.0f) {
        return PhysicsMaterial(0.001f * density_multiplier, 0.3f, 0.8f);
    }

    static PhysicsMaterial iron(float density_multiplier = 1.0f) {
        return PhysicsMaterial(0.1f * density_multiplier, 0.2f, 0.00001f);
    }

    static PhysicsMaterial stone(float density_multiplier = 1.0f) {
        return PhysicsMaterial(0.1f * density_multiplier, 0.8f, 0.00001f);
    }
};

} // namespace smlt
