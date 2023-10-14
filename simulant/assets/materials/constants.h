#pragma once

#include <string>
#include <memory>
#include "../../types.h"

namespace smlt {

typedef int32_t EnumType;

enum PolygonMode : EnumType {
    POLYGON_MODE_FILL,
    POLYGON_MODE_LINE,
    POLYGON_MODE_POINT
};

enum ShadeModel : EnumType {
    SHADE_MODEL_SMOOTH,
    SHADE_MODEL_FLAT
};

enum ColorMaterial : EnumType {
    COLOR_MATERIAL_NONE,
    COLOR_MATERIAL_AMBIENT,
    COLOR_MATERIAL_DIFFUSE,
    COLOR_MATERIAL_AMBIENT_AND_DIFFUSE
};

enum CullMode : EnumType {
    CULL_MODE_NONE,
    CULL_MODE_BACK_FACE,
    CULL_MODE_FRONT_FACE,
    CULL_MODE_FRONT_AND_BACK_FACE
};

enum BlendType : EnumType {
    BLEND_NONE,
    BLEND_ADD,
    BLEND_MODULATE,
    BLEND_COLOR,
    BLEND_ALPHA,
    BLEND_ONE_ONE_MINUS_ALPHA
};

enum DepthFunc : EnumType {
    DEPTH_FUNC_NEVER,
    DEPTH_FUNC_LESS,
    DEPTH_FUNC_LEQUAL,
    DEPTH_FUNC_EQUAL,
    DEPTH_FUNC_GEQUAL,
    DEPTH_FUNC_GREATER,
    DEPTH_FUNC_ALWAYS
};

enum FogMode : EnumType {
    FOG_MODE_NONE,
    FOG_MODE_LINEAR,
    FOG_MODE_EXP,
    FOG_MODE_EXP2
};

enum EnabledTexture : EnumType {
    DIFFUSE_MAP_ENABLED = 1,
    LIGHT_MAP_ENABLED = 2,
    NORMAL_MAP_ENABLED = 4,
    SPECULAR_MAP_ENABLED = 8
};

typedef int32_t EnabledTextureMask;

PolygonMode polygon_mode_from_name(const char* name);
ShadeModel shade_model_from_name(const char* name);
ColorMaterial color_material_from_name(const char* name);
CullMode cull_mode_from_name(const char* name);
BlendType blend_type_from_name(const char* name);
DepthFunc depth_func_from_name(const char* name);

#define DIFFUSE_PROPERTY_NAME "s_material_diffuse"
#define AMBIENT_PROPERTY_NAME "s_material_ambient"
#define EMISSION_PROPERTY_NAME "s_material_emission"
#define SPECULAR_PROPERTY_NAME "s_material_specular"
#define SHININESS_PROPERTY_NAME "s_material_shininess"
#define DIFFUSE_MAP_PROPERTY_NAME "s_diffuse_map"
#define LIGHT_MAP_PROPERTY_NAME "s_light_map"
#define NORMAL_MAP_PROPERTY_NAME "s_normal_map"
#define SPECULAR_MAP_PROPERTY_NAME "s_specular_map"
#define DEPTH_WRITE_ENABLED_PROPERTY_NAME "s_depth_write_enabled"
#define DEPTH_TEST_ENABLED_PROPERTY_NAME "s_depth_test_enabled"
#define DEPTH_FUNC_PROPERTY_NAME "s_depth_func"
#define BLEND_FUNC_PROPERTY_NAME "s_blend_func"
#define CULL_MODE_PROPERTY_NAME "s_cull_mode"
#define SHADE_MODEL_PROPERTY_NAME "s_shade_model"
#define LIGHTING_ENABLED_PROPERTY_NAME "s_lighting_enabled"
#define TEXTURES_ENABLED_PROPERTY_NAME "s_textures_enabled"
#define POINT_SIZE_PROPERTY_NAME "s_point_size"
#define POLYGON_MODE_PROPERTY_NAME "s_polygon_mode"
#define COLOR_MATERIAL_PROPERTY_NAME "s_color_material"
#define DIFFUSE_MAP_MATRIX_PROPERTY_NAME "s_diffuse_map_matrix"
#define LIGHT_MAP_MATRIX_PROPERTY_NAME "s_light_map_matrix"
#define NORMAL_MAP_MATRIX_PROPERTY_NAME "s_normal_map_matrix"
#define SPECULAR_MAP_MATRIX_PROPERTY_NAME "s_specular_map_matrix"

#define FOG_MODE_PROPERTY_NAME "s_fog_mode"
#define FOG_DENSITY_PROPERTY_NAME "s_fog_density"
#define FOG_START_PROPERTY_NAME "s_fog_start"
#define FOG_END_PROPERTY_NAME "s_fog_end"
#define FOG_COLOR_PROPERTY_NAME "s_fog_color"

}
