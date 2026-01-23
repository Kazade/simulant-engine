#pragma once

#include "../constants.h"
#include "../property_value.h"

#include "../../../utils/hash/fnv1.h"

namespace smlt {

typedef uint32_t MaterialPropertyNameHash;

// unsigned constexpr material_property_hash(char const *input) {
//     return *input ?
//         static_cast<unsigned int>(*input) + 33 * material_property_hash(input
//         + 1) : 5381;
// }

#define material_property_hash(x) smlt::fnv1<uint32_t>::hash(x)

constexpr const MaterialPropertyNameHash BASE_COLOR_PROPERTY_HASH =
    material_property_hash(BASE_COLOR_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash SPECULAR_COLOR_PROPERTY_HASH =
    material_property_hash(SPECULAR_COLOR_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash SPECULAR_PROPERTY_HASH =
    material_property_hash(SPECULAR_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash POINT_SIZE_PROPERTY_HASH = material_property_hash(POINT_SIZE_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash ROUGHNESS_PROPERTY_HASH =
    material_property_hash(ROUGHNESS_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash METALLIC_PROPERTY_HASH =
    material_property_hash(METALLIC_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash DEPTH_WRITE_ENABLED_PROPERTY_HASH = material_property_hash(DEPTH_WRITE_ENABLED_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash DEPTH_TEST_ENABLED_PROPERTY_HASH = material_property_hash(DEPTH_TEST_ENABLED_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash LIGHTING_ENABLED_PROPERTY_HASH = material_property_hash(LIGHTING_ENABLED_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash TEXTURES_ENABLED_PROPERTY_HASH = material_property_hash(TEXTURES_ENABLED_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash BASE_COLOR_MAP_PROPERTY_HASH =
    material_property_hash(BASE_COLOR_MAP_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash LIGHT_MAP_PROPERTY_HASH = material_property_hash(LIGHT_MAP_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash NORMAL_MAP_PROPERTY_HASH = material_property_hash(NORMAL_MAP_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash METALLIC_ROUGHNESS_MAP_PROPERTY_HASH =
    material_property_hash(METALLIC_ROUGHNESS_MAP_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash BASE_COLOR_MAP_MATRIX_PROPERTY_HASH =
    material_property_hash(BASE_COLOR_MAP_MATRIX_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash LIGHT_MAP_MATRIX_PROPERTY_HASH = material_property_hash(LIGHT_MAP_MATRIX_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash NORMAL_MAP_MATRIX_PROPERTY_HASH = material_property_hash(NORMAL_MAP_MATRIX_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash
    METALLIC_ROUGHNESS_MAP_MATRIX_PROPERTY_HASH =
        material_property_hash(METALLIC_ROUGHNESS_MAP_MATRIX_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash BLEND_FUNC_PROPERTY_HASH = material_property_hash(BLEND_FUNC_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash POLYGON_MODE_PROPERTY_HASH = material_property_hash(POLYGON_MODE_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash SHADE_MODEL_PROPERTY_HASH = material_property_hash(SHADE_MODEL_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash COLOR_MATERIAL_PROPERTY_HASH = material_property_hash(COLOR_MATERIAL_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash CULL_MODE_PROPERTY_HASH = material_property_hash(CULL_MODE_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash DEPTH_FUNC_PROPERTY_HASH = material_property_hash(DEPTH_FUNC_PROPERTY_NAME);

constexpr const MaterialPropertyNameHash FOG_MODE_PROPERTY_HASH = material_property_hash(FOG_MODE_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash FOG_DENSITY_PROPERTY_HASH = material_property_hash(FOG_DENSITY_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash FOG_START_PROPERTY_HASH = material_property_hash(FOG_START_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash FOG_END_PROPERTY_HASH = material_property_hash(FOG_END_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash FOG_COLOR_PROPERTY_HASH = material_property_hash(FOG_COLOR_PROPERTY_NAME);
constexpr const MaterialPropertyNameHash ALPHA_THRESHOLD_PROPERTY_HASH = material_property_hash(ALPHA_THRESHOLD_PROPERTY_NAME);

inline constexpr bool is_core_property(const MaterialPropertyNameHash hsh) {
    switch(hsh) {
        case BASE_COLOR_PROPERTY_HASH:
        case SPECULAR_COLOR_PROPERTY_HASH:
        case SPECULAR_PROPERTY_HASH:
        case ROUGHNESS_PROPERTY_HASH:
        case METALLIC_PROPERTY_HASH:
        case POINT_SIZE_PROPERTY_HASH:
        case DEPTH_WRITE_ENABLED_PROPERTY_HASH:
        case DEPTH_TEST_ENABLED_PROPERTY_HASH:
        case DEPTH_FUNC_PROPERTY_HASH:
        case LIGHTING_ENABLED_PROPERTY_HASH:
        case TEXTURES_ENABLED_PROPERTY_HASH:
        case BASE_COLOR_MAP_PROPERTY_HASH:
        case METALLIC_ROUGHNESS_MAP_PROPERTY_HASH:
        case LIGHT_MAP_PROPERTY_HASH:
        case NORMAL_MAP_PROPERTY_HASH:
        case BASE_COLOR_MAP_MATRIX_PROPERTY_HASH:
        case METALLIC_ROUGHNESS_MAP_MATRIX_PROPERTY_HASH:
        case LIGHT_MAP_MATRIX_PROPERTY_HASH:
        case NORMAL_MAP_MATRIX_PROPERTY_HASH:
        case BLEND_FUNC_PROPERTY_HASH:
        case POLYGON_MODE_PROPERTY_HASH:
        case SHADE_MODEL_PROPERTY_HASH:
        case COLOR_MATERIAL_PROPERTY_HASH:
        case CULL_MODE_PROPERTY_HASH:
        case FOG_COLOR_PROPERTY_HASH:
        case FOG_DENSITY_PROPERTY_HASH:
        case FOG_START_PROPERTY_HASH:
        case FOG_END_PROPERTY_HASH:
        case FOG_MODE_PROPERTY_HASH:
        case ALPHA_THRESHOLD_PROPERTY_HASH:
            return true;
        default:
            return false;
    }
}


typedef std::vector<std::pair<std::string, MaterialPropertyType>> PropertyList;

const PropertyList &core_properties();

bool is_core_property(const char* name);

bool core_property_type(const char* name, MaterialPropertyType* type);
bool core_property_type(MaterialPropertyNameHash hsh, MaterialPropertyType* type);

bool core_material_property_value(const char* name, const Color*& out);
bool core_material_property_value(const MaterialPropertyNameHash hsh, const Color*& out);

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
