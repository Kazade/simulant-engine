#include "core_material.h"

namespace smlt {



bool is_core_property(const char* name) {
    auto hsh = material_property_hash(name);
    return is_core_property(hsh);
}

bool core_property_type(MaterialPropertyNameHash hsh, MaterialPropertyType* type) {
    if(!is_core_property(hsh)) {
        return false;
    }

    switch(hsh) {
        case material_property_hash(DIFFUSE_PROPERTY_NAME):
        case material_property_hash(AMBIENT_PROPERTY_NAME):
        case material_property_hash(EMISSION_PROPERTY_NAME):
        case material_property_hash(SPECULAR_PROPERTY_NAME):
        case FOG_COLOR_PROPERTY_HASH:
            *type = MATERIAL_PROPERTY_TYPE_VEC4;
        break;
        case material_property_hash(SHININESS_PROPERTY_NAME):
        case material_property_hash(POINT_SIZE_PROPERTY_NAME):
        case ALPHA_THRESHOLD_PROPERTY_HASH:
        case FOG_DENSITY_PROPERTY_HASH:
        case FOG_START_PROPERTY_HASH:
        case FOG_END_PROPERTY_HASH:
            *type = MATERIAL_PROPERTY_TYPE_FLOAT;
        break;
        case material_property_hash(DEPTH_WRITE_ENABLED_PROPERTY_NAME):
        case material_property_hash(DEPTH_TEST_ENABLED_PROPERTY_NAME):
        case material_property_hash(LIGHTING_ENABLED_PROPERTY_NAME):
            *type = MATERIAL_PROPERTY_TYPE_BOOL;
        break;
        case material_property_hash(DIFFUSE_MAP_PROPERTY_NAME):
        case material_property_hash(SPECULAR_MAP_PROPERTY_NAME):
        case material_property_hash(LIGHT_MAP_PROPERTY_NAME):
        case material_property_hash(NORMAL_MAP_PROPERTY_NAME):
            *type = MATERIAL_PROPERTY_TYPE_TEXTURE;
        break;
        case material_property_hash(DIFFUSE_MAP_MATRIX_PROPERTY_NAME):
        case material_property_hash(SPECULAR_MAP_MATRIX_PROPERTY_NAME):
        case material_property_hash(LIGHT_MAP_MATRIX_PROPERTY_NAME):
        case material_property_hash(NORMAL_MAP_MATRIX_PROPERTY_NAME):
            *type = MATERIAL_PROPERTY_TYPE_MAT4;
        break;
        case material_property_hash(BLEND_FUNC_PROPERTY_NAME):
        case material_property_hash(POLYGON_MODE_PROPERTY_NAME):
        case material_property_hash(SHADE_MODEL_PROPERTY_NAME):
        case material_property_hash(COLOR_MATERIAL_PROPERTY_NAME):
        case material_property_hash(CULL_MODE_PROPERTY_NAME):
        case material_property_hash(TEXTURES_ENABLED_PROPERTY_NAME):
        case material_property_hash(ALPHA_FUNC_PROPERTY_NAME):
        case FOG_MODE_PROPERTY_HASH:
            *type = MATERIAL_PROPERTY_TYPE_INT;
        break;
        default:
            return false;
    }

    return true;
}

bool core_property_type(const char* name, MaterialPropertyType* type) {
    return core_property_type(material_property_hash(name), type);
}

const PropertyList& core_properties() {
    static const PropertyList core_properties = {
        {DIFFUSE_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_VEC4},
        {AMBIENT_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_VEC4},
        {EMISSION_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_VEC4},
        {SPECULAR_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_VEC4},

        {SHININESS_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_FLOAT},
        {POINT_SIZE_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_FLOAT},

        {DEPTH_WRITE_ENABLED_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_BOOL},
        {DEPTH_TEST_ENABLED_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_BOOL},

        {DIFFUSE_MAP_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_TEXTURE},
        {SPECULAR_MAP_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_TEXTURE},
        {LIGHT_MAP_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_TEXTURE},
        {NORMAL_MAP_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_TEXTURE},

        {DIFFUSE_MAP_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_MAT4},
        {SPECULAR_MAP_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_MAT4},
        {LIGHT_MAP_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_MAT4},
        {NORMAL_MAP_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_MAT4},

        {BLEND_FUNC_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {POLYGON_MODE_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {SHADE_MODEL_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {COLOR_MATERIAL_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {CULL_MODE_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {DEPTH_FUNC_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {ALPHA_FUNC_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {ALPHA_THRESHOLD_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_FLOAT},

        {FOG_MODE_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {FOG_START_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_FLOAT},
        {FOG_END_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_FLOAT},
        {FOG_DENSITY_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_FLOAT},
        {FOG_COLOR_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_VEC4},
    };

    return core_properties;
}

}
