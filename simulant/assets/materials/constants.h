#pragma once

#include <string>
#include <memory>
#include "../../types.h"

namespace smlt {

typedef int16_t MaterialPropertyID;

enum BlendType {
    BLEND_NONE,
    BLEND_ADD,
    BLEND_MODULATE,
    BLEND_COLOUR,
    BLEND_ALPHA,
    BLEND_ONE_ONE_MINUS_ALPHA
};

/* Value type, if the type is texture */
struct TextureUnit {
    TextureUnit() = default;
    TextureUnit(const TexturePtr& texture);

    Mat4& texture_matrix() {
        return texture_matrix_;
    }

    const Mat4& texture_matrix() const {
        return texture_matrix_;
    }

    void scroll_x(float amount);
    void scroll_y(float amount);

    const TextureID& texture_id() const;
    const TexturePtr texture() const {return texture_;}

private:    
    Mat4 texture_matrix_;

    /* Set when assigned as a material property to maintain a refcount */
    std::shared_ptr<Texture> texture_;
    TextureID texture_id_;
};

const char* const DIFFUSE_PROPERTY = "s_material_diffuse";
const char* const AMBIENT_PROPERTY = "s_material_ambient";
const char* const SPECULAR_PROPERTY = "s_material_specular";
const char* const SHININESS_PROPERTY = "s_material_shininess";
const char* const DIFFUSE_MAP_PROPERTY = "s_diffuse_map";
const char* const LIGHT_MAP_PROPERTY = "s_light_map";
const char* const NORMAL_MAP_PROPERTY = "s_normal_map";
const char* const SPECULAR_MAP_PROPERTY = "s_specular_map";
const char* const DEPTH_WRITE_ENABLED_PROPERTY = "s_depth_write_enabled";
const char* const DEPTH_TEST_ENABLED_PROPERTY = "s_depth_test_enabled";
const char* const DEPTH_FUNC_PROPERTY = "s_depth_func";
const char* const BLEND_FUNC_PROPERTY = "s_blend_func";
const char* const CULL_MODE_PROPERTY = "s_cull_mode";
const char* const SHADE_MODEL_PROPERTY = "s_shade_model";
const char* const LIGHTING_ENABLED_PROPERTY = "s_lighting_enabled";
const char* const TEXTURING_ENABLED_PROPERTY = "s_texturing_enabled";
const char* const POINT_SIZE_PROPERTY = "s_point_size";
const char* const POLYGON_MODE_PROPERTY = "s_polygon_mode";
const char* const COLOUR_MATERIAL_PROPERTY = "s_colour_material";
const char* const LIGHT_POSITION_PROPERTY = "s_light_position";
const char* const LIGHT_AMBIENT_PROPERTY = "s_light_ambient";
const char* const LIGHT_DIFFUSE_PROPERTY = "s_light_diffuse";
const char* const LIGHT_SPECULAR_PROPERTY = "s_light_specular";
const char* const LIGHT_CONSTANT_ATTENUATION_PROPERTY = "s_light_constant_attenuation";
const char* const LIGHT_LINEAR_ATTENUATION_PROPERTY = "s_light_linear_attenuation";
const char* const LIGHT_QUADRATIC_ATTENUATION_PROPERTY = "s_light_quadratic_attenuation";

const char* const VIEW_MATRIX_PROPERTY = "s_view";
const char* const MODELVIEW_PROJECTION_MATRIX_PROPERTY = "s_modelview_projection";
const char* const PROJECTION_MATRIX_PROPERTY = "s_projection";
const char* const MODELVIEW_MATRIX_PROPERTY = "s_modelview";
const char* const INVERSE_TRANSPOSE_MODELVIEW_MATRIX_PROPERTY = "s_inverse_transpose_modelview";

const char* const DIFFUSE_MAP_MATRIX_PROPERTY = "s_diffuse_map_matrix";
const char* const LIGHT_MAP_MATRIX_PROPERTY = "s_light_map_matrix";
const char* const NORMAL_MAP_MATRIX_PROPERTY = "s_normal_map_matrix";
const char* const SPECULAR_MAP_MATRIX_PROPERTY = "s_specular_map_matrix";

}
