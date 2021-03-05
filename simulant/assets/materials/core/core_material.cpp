#include "core_material.h"

namespace smlt {

bool core_material_property_value(const char* name, const Colour *&out) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash(DIFFUSE_PROPERTY):
            out = &core_material().diffuse;
        break;
        case const_hash(AMBIENT_PROPERTY):
            out = &core_material().ambient;
        break;
        case const_hash(EMISSION_PROPERTY):
            out = &core_material().emission;
        break;
        case const_hash(SPECULAR_PROPERTY):
            out = &core_material().specular;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const bool*& out) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash(DEPTH_TEST_ENABLED_PROPERTY):
            out = &core_material().depth_test_enabled;
        break;
        case const_hash(DEPTH_WRITE_ENABLED_PROPERTY):
            out = &core_material().depth_writes_enabled;
        break;
        case const_hash(LIGHTING_ENABLED_PROPERTY):
            out = &core_material().lighting_enabled;
        break;
        case const_hash(TEXTURING_ENABLED_PROPERTY):
            out = &core_material().texturing_enabled;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const float*& out) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash(SHININESS_PROPERTY):
            out = &core_material().shininess;
        break;
        case const_hash(POINT_SIZE_PROPERTY):
            out = &core_material().point_size;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const int32_t*& out) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash(BLEND_FUNC_PROPERTY):
            out = &core_material().blend_func;
        break;
        case const_hash(DEPTH_FUNC_PROPERTY):
            out = &core_material().depth_func;
        break;
        case const_hash(POLYGON_MODE_PROPERTY):
            out = &core_material().polygon_mode;
        break;
        case const_hash(SHADE_MODEL_PROPERTY):
            out = &core_material().shade_model;
        break;
        case const_hash(COLOUR_MATERIAL_PROPERTY):
            out = &core_material().colour_material;
        break;
        case const_hash(CULL_MODE_PROPERTY):
            out = &core_material().cull_mode;
        break;
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Vec2*&) {
    auto hsh = const_hash(name);
    switch(hsh) {
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Vec3*&) {
    auto hsh = const_hash(name);
    switch(hsh) {
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Vec4*&) {
    auto hsh = const_hash(name);
    switch(hsh) {
        default:
        return false;
    }

    return true;
}

bool core_material_property_value(const char* name, const Mat3*&) {
    auto hsh = const_hash(name);
    switch(hsh) {
        default:
        return false;
    }

    return true;
}


bool core_material_property_value(const char* name, const TexturePtr*&) {
    auto hsh = const_hash(name);
    switch(hsh) {
        default:
        return false;
    }

    return true;
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
        case const_hash(DEPTH_WRITE_ENABLED_PROPERTY):
        case const_hash(DEPTH_TEST_ENABLED_PROPERTY):
        case const_hash(LIGHTING_ENABLED_PROPERTY):
        case const_hash(TEXTURING_ENABLED_PROPERTY):
        case const_hash(DIFFUSE_MAP_PROPERTY):
        case const_hash(SPECULAR_MAP_PROPERTY):
        case const_hash(LIGHT_MAP_PROPERTY):
        case const_hash(NORMAL_MAP_PROPERTY):
        case const_hash(BLEND_FUNC_PROPERTY):
        case const_hash(POLYGON_MODE_PROPERTY):
        case const_hash(SHADE_MODEL_PROPERTY):
        case const_hash(COLOUR_MATERIAL_PROPERTY):
        case const_hash(CULL_MODE_PROPERTY):
        return true;
        default:
        return false;
    }
}

const CoreMaterial& core_material() {
    static const CoreMaterial mat;
    return mat;
}



}
