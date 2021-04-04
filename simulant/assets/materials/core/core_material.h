#pragma once

#include "../constants.h"

namespace smlt {

typedef int32_t EnumType;
typedef uint32_t MaterialPropertyNameHash;

unsigned constexpr material_property_hash(char const *input) {
    return *input ?
        static_cast<unsigned int>(*input) + 33 * material_property_hash(input + 1) :
        5381;
}

constexpr const MaterialPropertyNameHash DIFFUSE_PROPERTY_HASH = material_property_hash(DIFFUSE_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash AMBIENT_PROPERTY_HASH = material_property_hash(AMBIENT_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash SPECULAR_PROPERTY_HASH = material_property_hash(SPECULAR_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash EMISSION_PROPERTY_HASH = material_property_hash(EMISSION_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash SHININESS_PROPERTY_HASH = material_property_hash(SHININESS_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash POINT_SIZE_PROPERTY_HASH = material_property_hash(POINT_SIZE_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash DEPTH_WRITE_ENABLED_PROPERTY_HASH = material_property_hash(DEPTH_WRITE_ENABLED_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash DEPTH_TEST_ENABLED_PROPERTY_HASH = material_property_hash(DEPTH_TEST_ENABLED_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash LIGHTING_ENABLED_PROPERTY_HASH = material_property_hash(LIGHTING_ENABLED_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash TEXTURES_ENABLED_PROPERTY_HASH = material_property_hash(TEXTURES_ENABLED_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash DIFFUSE_MAP_PROPERTY_HASH = material_property_hash(DIFFUSE_MAP_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash LIGHT_MAP_PROPERTY_HASH = material_property_hash(LIGHT_MAP_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash NORMAL_MAP_PROPERTY_HASH = material_property_hash(NORMAL_MAP_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash SPECULAR_MAP_PROPERTY_HASH = material_property_hash(SPECULAR_MAP_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash DIFFUSE_MAP_MATRIX_PROPERTY_HASH = material_property_hash(DIFFUSE_MAP_MATRIX_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash LIGHT_MAP_MATRIX_PROPERTY_HASH = material_property_hash(LIGHT_MAP_MATRIX_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash NORMAL_MAP_MATRIX_PROPERTY_HASH = material_property_hash(NORMAL_MAP_MATRIX_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash SPECULAR_MAP_MATRIX_PROPERTY_HASH = material_property_hash(SPECULAR_MAP_MATRIX_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash BLEND_FUNC_PROPERTY_HASH = material_property_hash(BLEND_FUNC_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash POLYGON_MODE_PROPERTY_HASH = material_property_hash(POLYGON_MODE_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash SHADE_MODEL_PROPERTY_HASH = material_property_hash(SHADE_MODEL_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash COLOUR_MATERIAL_PROPERTY_HASH = material_property_hash(COLOUR_MATERIAL_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash CULL_MODE_PROPERTY_HASH = material_property_hash(CULL_MODE_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash DEPTH_FUNC_PROPERTY_HASH = material_property_hash(DEPTH_FUNC_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash FOG_MODE_PROPERTY_HASH = material_property_hash(FOG_MODE_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash FOG_DENSITY_PROPERTY_HASH = material_property_hash(FOG_DENSITY_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash FOG_START_PROPERTY_HASH = material_property_hash(FOG_START_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash FOG_END_PROPERTY_HASH = material_property_hash(FOG_END_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash FOG_COLOUR_PROPERTY_HASH = material_property_hash(FOG_COLOUR_PROPERTY_NAME);


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
    const int32_t textures_enabled = DIFFUSE_MAP_ENABLED | LIGHT_MAP_ENABLED | SPECULAR_MAP_ENABLED | NORMAL_MAP_ENABLED; /* Enable first 4 texture units */

    TexturePtr diffuse_map;
    TexturePtr light_map;
    TexturePtr normal_map;
    TexturePtr specular_map;

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

    const EnumType fog_mode = (EnumType) FOG_MODE_NONE;
    const float fog_density = 1.0f;
    const float fog_start = 100.0f;
    const float fog_end = 1000.0f;
    const Colour fog_colour = smlt::Colour::WHITE;
};

void init_core_material(const CoreMaterial& base);
const CoreMaterial& core_material();

typedef std::vector<std::pair<std::string, MaterialPropertyType>> PropertyList;

const PropertyList &core_properties();

bool is_core_property(const char* name);
bool is_core_property(const MaterialPropertyNameHash hsh);

bool core_property_type(const char* name, MaterialPropertyType* type);
bool core_property_type(MaterialPropertyNameHash hsh, MaterialPropertyType* type);

bool core_material_property_value(const char* name, const Colour*& out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const Colour*& out);

bool core_material_property_value(const char* name, const float *&out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const float*& out);

bool core_material_property_value(const char* name, const int32_t*&);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const int32_t*& out);

bool core_material_property_value(const char* name, const bool*& out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const bool*& out);

bool core_material_property_value(const char* name, const Vec2*& out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const Vec2*& out);

bool core_material_property_value(const char* name, const Vec3*& out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const Vec3*& out);

bool core_material_property_value(const char* name, const Vec4*& out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const Vec4*& out);

bool core_material_property_value(const char* name, const Mat3*& out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const Mat3*& out);

bool core_material_property_value(const char* name, const Mat4*& out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const Mat4*& out);

bool core_material_property_value(const char* name, const TexturePtr*& out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const TexturePtr*& out);

}
