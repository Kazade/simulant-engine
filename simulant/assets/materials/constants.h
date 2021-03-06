#pragma once

#include <string>
#include <memory>
#include "../../types.h"

namespace smlt {

enum PolygonMode {
    POLYGON_MODE_FILL,
    POLYGON_MODE_LINE,
    POLYGON_MODE_POINT
};

enum ShadeModel {
    SHADE_MODEL_SMOOTH,
    SHADE_MODEL_FLAT
};

enum ColourMaterial {
    COLOUR_MATERIAL_NONE,
    COLOUR_MATERIAL_AMBIENT,
    COLOUR_MATERIAL_DIFFUSE,
    COLOUR_MATERIAL_AMBIENT_AND_DIFFUSE
};

enum CullMode {
    CULL_MODE_NONE,
    CULL_MODE_BACK_FACE,
    CULL_MODE_FRONT_FACE,
    CULL_MODE_FRONT_AND_BACK_FACE
};

enum BlendType {
    BLEND_NONE,
    BLEND_ADD,
    BLEND_MODULATE,
    BLEND_COLOUR,
    BLEND_ALPHA,
    BLEND_ONE_ONE_MINUS_ALPHA
};

enum DepthFunc {
    DEPTH_FUNC_NEVER,
    DEPTH_FUNC_LESS,
    DEPTH_FUNC_LEQUAL,
    DEPTH_FUNC_EQUAL,
    DEPTH_FUNC_GEQUAL,
    DEPTH_FUNC_GREATER,
    DEPTH_FUNC_ALWAYS
};

PolygonMode polygon_mode_from_name(const char* name);
ShadeModel shade_model_from_name(const char* name);
ColourMaterial colour_material_from_name(const char* name);
CullMode cull_mode_from_name(const char* name);
BlendType blend_type_from_name(const char* name);
DepthFunc depth_func_from_name(const char* name);

constexpr const char* const DIFFUSE_PROPERTY = "s_material_diffuse";
constexpr const char* const AMBIENT_PROPERTY = "s_material_ambient";
constexpr const char* const EMISSION_PROPERTY = "s_material_emission";
constexpr const char* const SPECULAR_PROPERTY = "s_material_specular";
constexpr const char* const SHININESS_PROPERTY = "s_material_shininess";
constexpr const char* const DIFFUSE_MAP_PROPERTY = "s_diffuse_map";
constexpr const char* const LIGHT_MAP_PROPERTY = "s_light_map";
constexpr const char* const NORMAL_MAP_PROPERTY = "s_normal_map";
constexpr const char* const SPECULAR_MAP_PROPERTY = "s_specular_map";
constexpr const char* const DEPTH_WRITE_ENABLED_PROPERTY = "s_depth_write_enabled";
constexpr const char* const DEPTH_TEST_ENABLED_PROPERTY = "s_depth_test_enabled";
constexpr const char* const DEPTH_FUNC_PROPERTY = "s_depth_func";
constexpr const char* const BLEND_FUNC_PROPERTY = "s_blend_func";
constexpr const char* const CULL_MODE_PROPERTY = "s_cull_mode";
constexpr const char* const SHADE_MODEL_PROPERTY = "s_shade_model";
constexpr const char* const LIGHTING_ENABLED_PROPERTY = "s_lighting_enabled";
constexpr const char* const TEXTURING_ENABLED_PROPERTY = "s_texturing_enabled";
constexpr const char* const POINT_SIZE_PROPERTY = "s_point_size";
constexpr const char* const POLYGON_MODE_PROPERTY = "s_polygon_mode";
constexpr const char* const COLOUR_MATERIAL_PROPERTY = "s_colour_material";
constexpr const char* const LIGHT_POSITION_PROPERTY = "s_light_position";
constexpr const char* const LIGHT_AMBIENT_PROPERTY = "s_light_ambient";
constexpr const char* const LIGHT_DIFFUSE_PROPERTY = "s_light_diffuse";
constexpr const char* const LIGHT_SPECULAR_PROPERTY = "s_light_specular";
constexpr const char* const LIGHT_CONSTANT_ATTENUATION_PROPERTY = "s_light_constant_attenuation";
constexpr const char* const LIGHT_LINEAR_ATTENUATION_PROPERTY = "s_light_linear_attenuation";
constexpr const char* const LIGHT_QUADRATIC_ATTENUATION_PROPERTY = "s_light_quadratic_attenuation";

constexpr const char* const VIEW_MATRIX_PROPERTY = "s_view";
constexpr const char* const MODELVIEW_PROJECTION_MATRIX_PROPERTY = "s_modelview_projection";
constexpr const char* const PROJECTION_MATRIX_PROPERTY = "s_projection";
constexpr const char* const MODELVIEW_MATRIX_PROPERTY = "s_modelview";
constexpr const char* const INVERSE_TRANSPOSE_MODELVIEW_MATRIX_PROPERTY = "s_inverse_transpose_modelview";

constexpr const char* const DIFFUSE_MAP_MATRIX_PROPERTY = "s_diffuse_map_matrix";
constexpr const char* const LIGHT_MAP_MATRIX_PROPERTY = "s_light_map_matrix";
constexpr const char* const NORMAL_MAP_MATRIX_PROPERTY = "s_normal_map_matrix";
constexpr const char* const SPECULAR_MAP_MATRIX_PROPERTY = "s_specular_map_matrix";

}
