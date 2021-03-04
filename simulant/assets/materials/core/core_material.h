#pragma once

#include "../constants.h"

namespace smlt {

typedef uint32_t EnumType;

struct CoreMaterial {
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

const CoreMaterial& core_material();

unsigned constexpr const_hash(char const *input) {
    return *input ?
        static_cast<unsigned int>(*input) + 33 * const_hash(input + 1) :
        5381;
}

bool is_core_property(const char* name);

bool core_material_property_value(const char* name, Colour* out);
bool core_material_property_value(const char* name, float* out);
bool core_material_property_value(const char* name, int32_t*);
bool core_material_property_value(const char* name, bool* out);
bool core_material_property_value(const char* name, Vec2* out);
bool core_material_property_value(const char* name, Vec3* out);
bool core_material_property_value(const char* name, Vec4* out);
bool core_material_property_value(const char* name, Mat3* out);
bool core_material_property_value(const char* name, Mat4* out);

}
