#include "core_material.h"

namespace smlt {

bool core_material_property_value(const MaterialPropertyNameHash hsh, const Colour *&out) {
    switch(hsh) {
        case DIFFUSE_PROPERTY_HASH:
            out = &core_material().diffuse;
        break;
        case AMBIENT_PROPERTY_HASH:
            out = &core_material().ambient;
        break;
        case EMISSION_PROPERTY_HASH:
            out = &core_material().emission;
        break;
        case SPECULAR_PROPERTY_HASH:
            out = &core_material().specular;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Colour *&out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}

bool core_material_property_value(const MaterialPropertyNameHash hsh, const bool*& out) {
    switch(hsh) {
        case DEPTH_TEST_ENABLED_PROPERTY_HASH:
            out = &core_material().depth_test_enabled;
        break;
        case DEPTH_WRITE_ENABLED_PROPERTY_HASH:
            out = &core_material().depth_writes_enabled;
        break;
        case LIGHTING_ENABLED_PROPERTY_HASH:
            out = &core_material().lighting_enabled;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const bool*& out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}

bool core_material_property_value(const MaterialPropertyNameHash hsh, const float*& out) {
    switch(hsh) {
        case SHININESS_PROPERTY_HASH:
            out = &core_material().shininess;
        break;
        case POINT_SIZE_PROPERTY_HASH:
            out = &core_material().point_size;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const float*& out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}

bool core_material_property_value(const MaterialPropertyNameHash hsh, const int32_t*& out) {
    switch(hsh) {
        case BLEND_FUNC_PROPERTY_HASH:
            out = &core_material().blend_func;
        break;
        case DEPTH_FUNC_PROPERTY_HASH:
            out = &core_material().depth_func;
        break;
        case POLYGON_MODE_PROPERTY_HASH:
            out = &core_material().polygon_mode;
        break;
        case SHADE_MODEL_PROPERTY_HASH:
            out = &core_material().shade_model;
        break;
        case COLOUR_MATERIAL_PROPERTY_HASH:
            out = &core_material().colour_material;
        break;
        case CULL_MODE_PROPERTY_HASH:
            out = &core_material().cull_mode;
        break;
        case TEXTURES_ENABLED_PROPERTY_HASH:
            out = &core_material().textures_enabled;
        break;
        case FOG_MODE_PROPERTY_HASH:
            out = &core_material().fog_mode;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const int32_t*& out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}

bool core_material_property_value(const MaterialPropertyNameHash hsh, const Vec2*&) {
    switch(hsh) {
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Vec2*& out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}

bool core_material_property_value(const MaterialPropertyNameHash hsh, const Vec3*&) {
    switch(hsh) {
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Vec3*& out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}

bool core_material_property_value(const MaterialPropertyNameHash hsh, const Vec4*& out) {
    switch(hsh) {
        case DIFFUSE_PROPERTY_HASH:
            out = (Vec4*) &core_material().diffuse;
        break;
        case SPECULAR_PROPERTY_HASH:
            out = (Vec4*) &core_material().specular;
        break;
        case EMISSION_PROPERTY_HASH:
            out = (Vec4*) &core_material().emission;
        break;
        case AMBIENT_PROPERTY_HASH:
            out = (Vec4*) &core_material().ambient;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Vec4*& out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}

bool core_material_property_value(const MaterialPropertyNameHash hsh, const Mat3*&) {
    switch(hsh) {
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Mat3*& out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}

bool core_material_property_value(const MaterialPropertyNameHash hsh, const Mat4*& out) {
    switch(hsh) {
        case DIFFUSE_MAP_MATRIX_PROPERTY_HASH:
            out = &core_material().diffuse_map_matrix;
        break;
        case SPECULAR_MAP_MATRIX_PROPERTY_HASH:
            out = &core_material().specular_map_matrix;
        break;
        case NORMAL_MAP_MATRIX_PROPERTY_HASH:
            out = &core_material().normal_map_matrix;
        break;
        case LIGHT_MAP_MATRIX_PROPERTY_HASH:
            out = &core_material().light_map_matrix;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Mat4*& out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}

bool core_material_property_value(const MaterialPropertyNameHash hsh, const TexturePtr*& out) {
    switch(hsh) {
        case DIFFUSE_MAP_PROPERTY_HASH:
            out = &core_material().diffuse_map;
        break;
        case SPECULAR_MAP_PROPERTY_HASH:
            out = &core_material().specular_map;
        break;
        case LIGHT_MAP_PROPERTY_HASH:
            out = &core_material().light_map;
        break;
        case NORMAL_MAP_PROPERTY_HASH:
            out = &core_material().normal_map;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const TexturePtr*& out) {
    auto hsh = material_property_hash(name);
    return core_material_property_value(hsh, out);
}


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
        case FOG_COLOUR_PROPERTY_HASH:
            *type = MATERIAL_PROPERTY_TYPE_VEC4;
        break;
        case material_property_hash(SHININESS_PROPERTY_NAME):
        case material_property_hash(POINT_SIZE_PROPERTY_NAME):
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
        case material_property_hash(COLOUR_MATERIAL_PROPERTY_NAME):
        case material_property_hash(CULL_MODE_PROPERTY_NAME):
        case material_property_hash(TEXTURES_ENABLED_PROPERTY_NAME):
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


static CoreMaterial* CORE_MATERIAL = nullptr;

void init_core_material(const CoreMaterial& base) {
    if(CORE_MATERIAL) {
        return;
    }

    CORE_MATERIAL = new CoreMaterial(base);
}

const CoreMaterial& core_material() {
    return *CORE_MATERIAL;
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
        {COLOUR_MATERIAL_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {CULL_MODE_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {DEPTH_FUNC_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},

        {FOG_MODE_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_INT},
        {FOG_START_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_FLOAT},
        {FOG_END_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_FLOAT},
        {FOG_DENSITY_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_FLOAT},
        {FOG_COLOUR_PROPERTY_NAME, MATERIAL_PROPERTY_TYPE_VEC4},
    };

    return core_properties;
}

}
