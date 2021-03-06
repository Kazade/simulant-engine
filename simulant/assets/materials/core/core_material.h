#pragma once

#include "../constants.h"

namespace smlt {

typedef int32_t EnumType;
typedef uint32_t MaterialPropertyNameHash;

enum MaterialPropertyType {
    MATERIAL_PROPERTY_TYPE_BOOL,
    MATERIAL_PROPERTY_TYPE_INT,
    MATERIAL_PROPERTY_TYPE_FLOAT,
    MATERIAL_PROPERTY_TYPE_VEC2,
    MATERIAL_PROPERTY_TYPE_VEC3,
    MATERIAL_PROPERTY_TYPE_VEC4,
    MATERIAL_PROPERTY_TYPE_MAT3,
    MATERIAL_PROPERTY_TYPE_MAT4,
    MATERIAL_PROPERTY_TYPE_TEXTURE
};

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

    const TexturePtr diffuse_map;
    const TexturePtr specular_map;
    const TexturePtr light_map;
    const TexturePtr normal_map;

    const Mat4 diffuse_map_matrix;
    const Mat4 specular_map_matrix;
    const Mat4 light_map_matrix;
    const Mat4 normal_map_matrix;

    const EnumType blend_func = (EnumType) BLEND_NONE;
    const EnumType polygon_mode = (EnumType) POLYGON_MODE_FILL;
    const EnumType shade_model = (EnumType) SHADE_MODEL_SMOOTH;
    const EnumType colour_material = (EnumType) COLOUR_MATERIAL_NONE;
    const EnumType cull_mode = (EnumType) CULL_MODE_NONE;
    const EnumType depth_func = (EnumType) DEPTH_FUNC_LEQUAL;
};

const CoreMaterial& core_material();

unsigned constexpr const_hash(char const *input) {
    return *input ?
        static_cast<unsigned int>(*input) + 33 * const_hash(input + 1) :
        5381;
}

bool is_core_property(const char* name);
bool is_core_property(MaterialPropertyNameHash hsh);

bool core_property_type(const char* name, MaterialPropertyType* type);
bool core_property_type(MaterialPropertyNameHash hsh, MaterialPropertyType* type);

bool core_material_property_value(const char* name, const Colour*& out);
bool core_material_property_value(const char* name, const float *&out);
bool core_material_property_value(const char* name, const int32_t*&);
bool core_material_property_value(const char* name, const bool*& out);
bool core_material_property_value(const char* name, const Vec2*& out);
bool core_material_property_value(const char* name, const Vec3*& out);
bool core_material_property_value(const char* name, const Vec4*& out);
bool core_material_property_value(const char* name, const Mat3*& out);
bool core_material_property_value(const char* name, const Mat4*& out);
bool core_material_property_value(const char* name, const TexturePtr*& out);

}
