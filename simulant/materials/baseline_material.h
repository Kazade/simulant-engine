#pragma once

#include "../types.h"

namespace smlt {

typedef uint32_t EnumType;

struct BaselineMaterial {
    const Colour diffuse = Colour(1, 1, 1, 1);
    const Colour ambient = Colour(1, 1, 1, 1);
    const Colour emission = Colour(0, 0, 0, 1);
    const Colour specular = Colour(0, 0, 0, 1);

    const float shininess = 0.0f;
    const float point_size = 1.0f;

    const bool depth_writes_enabled = true;
    const bool depth_test_enabled = true;

    const bool lighting_enabled = true;
    const bool texturing_enabled = true;

    const TextureUnit diffuse_map;
    const TextureUnit specular_map;
    const TextureUnit light_map;
    const TextureUnit normal_map;

    const EnumType blend_func = (EnumType) BLEND_NONE;
    const EnumType polygon_mode = (EnumType) POLYGON_MODE_FILL;
    const EnumType shade_model = (EnumType) SHADE_MODEL_SMOOTH;
    const EnumType colour_material = (EnumType) COLOUR_MATERIAL_NONE;
    const EnumType cull_mode = (EnumType) CULL_MODE_NONE;
};

extern const static BaselineMaterial BASELINE_MATERIAL;

unsigned constexpr const_hash(char const *input) {
    return *input ?
        static_cast<unsigned int>(*input) + 33 * const_hash(input + 1) :
        5381;
}

bool is_core_property(const char* name) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash(DIFFUSE_PROPERTY):
        case const_hash(AMBIENT_PROPERTY):
        case const_hash(EMISSION_PROPERTY):
        case const_hash(SPECULAR_PROPERTY):
        case const_hash(SHININESS_PROPERTY):
        case const_hash(POINT_SIZE_PROPERTY):
        case const_hash(DEPTH_WRITES_ENABLED_PROPERTY):
        case const_hash(DEPTH_TEST_ENABLED_PROPERTY):
        case const_hash(LIGHTING_ENABLED_PROPERTY):
        case const_hash(TEXTURING_ENABLED_PROPERTY):
        case const_hash(DIFFUSE_MAP_PROPERTY):
        case const_hash(SPECULAR_MAP_PROPERTY):
        case const_hash(LIGHT_MAP_PROPERTY):
        case const_hash(NORMAL_MAP_PROPERTY):
    }
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
