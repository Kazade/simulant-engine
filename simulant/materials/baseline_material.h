#pragma once

#include "../types.h"

namespace smlt {

struct BaselineMaterial {
    Colour diffuse = Colour();
    Colour ambient = Colour();
    Colour emission = Colour();
    Colour specular = Colour();
    float shininess = 0.0f;
    bool depth_writes_enabled = true;
    bool depth_test_enabled = true;
};

extern const static BaselineMaterial BASELINE_MATERIAL;

unsigned constexpr const_hash(char const *input) {
    return *input ?
        static_cast<unsigned int>(*input) + 33 * const_hash(input + 1) :
        5381;
}

template<typename T>
bool material_property_baseline_value(const char* name, T* out);

template<>
bool material_property_baseline_value<Colour>(const char* name, Colour* out) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash(DIFFUSE_PROPERTY):
            *out = BASELINE_MATERIAL.diffuse;
        break;
        case const_hash(AMBIENT_PROPERTY):
            *out = BASELINE_MATERIAL.ambient;
        break;
        case const_hash(EMISSION_PROPERTY):
            *out = BASELINE_MATERIAL.emission;
        break;
        case const_hash(SPECULAR_PROPERTY):
            *out = BASELINE_MATERIAL.specular;
        break;
    default:
        return false;
    }

    return true;
}

template<>
bool material_property_baseline_value<float>(const char* name, float* out) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash(SHININESS_PROPERTY):
            *out = BASELINE_MATERIAL.shininess;
        break;
    default:
        return false;
    }

    return true;
}

}
