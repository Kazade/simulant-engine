#include "constants.h"
#include "core/core_material.h"

namespace smlt {

PolygonMode polygon_mode_from_name(const char* name) {
    auto hsh = material_property_hash(name);
    switch(hsh) {
        case material_property_hash("line"): return POLYGON_MODE_LINE;
        case material_property_hash("fill"): return POLYGON_MODE_FILL;
        case material_property_hash("point"): return POLYGON_MODE_POINT;
        default:
            S_WARN("Invalid polygon mode name: {0}", name);
            return POLYGON_MODE_FILL;
    }
}

ShadeModel shade_model_from_name(const char* name) {
    auto hsh = material_property_hash(name);
    switch(hsh) {
        case material_property_hash("smooth"): return SHADE_MODEL_SMOOTH;
        case material_property_hash("flat"): return SHADE_MODEL_FLAT;
        default:
            S_WARN("Invalid shade model name: {0}", name);
            return SHADE_MODEL_SMOOTH;
    }
}

ColorMaterial color_material_from_name(const char* name) {
    auto hsh = material_property_hash(name);
    switch(hsh) {
        case material_property_hash("ambient"): return COLOR_MATERIAL_AMBIENT;
        case material_property_hash("ambient_and_diffuse"): return COLOR_MATERIAL_AMBIENT_AND_DIFFUSE;
        case material_property_hash("diffuse"): return COLOR_MATERIAL_DIFFUSE;
        case material_property_hash("none"): return COLOR_MATERIAL_NONE;
        default:
            S_WARN("Invalid color material name: {0}", name);
            return COLOR_MATERIAL_NONE;
    }
}

CullMode cull_mode_from_name(const char* name) {
    auto hsh = material_property_hash(name);
    switch(hsh) {
        case material_property_hash("back_face"): return CULL_MODE_BACK_FACE;
        case material_property_hash("front_face"): return CULL_MODE_FRONT_FACE;
        case material_property_hash("front_and_back_face"): return CULL_MODE_FRONT_AND_BACK_FACE;
        case material_property_hash("none"): return CULL_MODE_NONE;
        default:
            S_WARN("Invalid cull mode name: {0}", name);
            return CULL_MODE_BACK_FACE;
    }
}

BlendType blend_type_from_name(const char* name) {
    auto hsh = material_property_hash(name);
    switch(hsh) {
        case material_property_hash("add"): return BLEND_ADD;
        case material_property_hash("alpha"): return BLEND_ALPHA;
        case material_property_hash("color"): return BLEND_COLOR;
        case material_property_hash("modulate"): return BLEND_MODULATE;
        case material_property_hash("one_one_minus_alpha"): return BLEND_ONE_ONE_MINUS_ALPHA;
        case material_property_hash("none"): return BLEND_NONE;
        default:
            S_WARN("Invalid blend type name: {0}", name);
            return BLEND_NONE;
    }
}

DepthFunc depth_func_from_name(const char* name) {
    auto hsh = material_property_hash(name);
    switch(hsh) {
        case material_property_hash("always"): return DEPTH_FUNC_ALWAYS;
        case material_property_hash("less"): return DEPTH_FUNC_LESS;
        case material_property_hash("lequal"): return DEPTH_FUNC_LEQUAL;
        case material_property_hash("equal"): return DEPTH_FUNC_EQUAL;
        case material_property_hash("gequal"): return DEPTH_FUNC_GEQUAL;
        case material_property_hash("greater"): return DEPTH_FUNC_GREATER;
        case material_property_hash("never"): return DEPTH_FUNC_NEVER;
        default:
            S_WARN("Invalid depth func name: {0}", name);
            return DEPTH_FUNC_LEQUAL;
    }
}

}
