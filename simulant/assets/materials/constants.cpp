#include "constants.h"
#include "core/core_material.h"

namespace smlt {

PolygonMode polygon_mode_from_name(const char* name) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash("line"): return POLYGON_MODE_LINE;
        case const_hash("fill"): return POLYGON_MODE_FILL;
        case const_hash("point"): return POLYGON_MODE_POINT;
        default:
            S_WARN("Invalid polygon mode name: {0}", name);
            return POLYGON_MODE_FILL;
    }
}

ShadeModel shade_model_from_name(const char* name) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash("smooth"): return SHADE_MODEL_SMOOTH;
        case const_hash("flat"): return SHADE_MODEL_FLAT;
        default:
            S_WARN("Invalid shade model name: {0}", name);
            return SHADE_MODEL_SMOOTH;
    }
}

ColourMaterial colour_material_from_name(const char* name) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash("ambient"): return COLOUR_MATERIAL_AMBIENT;
        case const_hash("ambient_and_diffuse"): return COLOUR_MATERIAL_AMBIENT_AND_DIFFUSE;
        case const_hash("diffuse"): return COLOUR_MATERIAL_DIFFUSE;
        case const_hash("none"): return COLOUR_MATERIAL_NONE;
        default:
            S_WARN("Invalid colour material name: {0}", name);
            return COLOUR_MATERIAL_NONE;
    }
}

CullMode cull_mode_from_name(const char* name) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash("back_face"): return CULL_MODE_BACK_FACE;
        case const_hash("front_face"): return CULL_MODE_FRONT_FACE;
        case const_hash("front_and_back_face"): return CULL_MODE_FRONT_AND_BACK_FACE;
        case const_hash("none"): return CULL_MODE_NONE;
        default:
            S_WARN("Invalid cull mode name: {0}", name);
            return CULL_MODE_BACK_FACE;
    }
}

BlendType blend_type_from_name(const char* name) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash("add"): return BLEND_ADD;
        case const_hash("alpha"): return BLEND_ALPHA;
        case const_hash("colour"): return BLEND_COLOUR;
        case const_hash("modulate"): return BLEND_MODULATE;
        case const_hash("one_one_minus_alpha"): return BLEND_ONE_ONE_MINUS_ALPHA;
        case const_hash("none"): return BLEND_NONE;
        default:
            S_WARN("Invalid blend type name: {0}", name);
            return BLEND_NONE;
    }
}

DepthFunc depth_func_from_name(const char* name) {
    auto hsh = const_hash(name);
    switch(hsh) {
        case const_hash("always"): return DEPTH_FUNC_ALWAYS;
        case const_hash("less"): return DEPTH_FUNC_LESS;
        case const_hash("lequal"): return DEPTH_FUNC_LEQUAL;
        case const_hash("equal"): return DEPTH_FUNC_EQUAL;
        case const_hash("gequal"): return DEPTH_FUNC_GEQUAL;
        case const_hash("greater"): return DEPTH_FUNC_GREATER;
        case const_hash("never"): return DEPTH_FUNC_NEVER;
        default:
            S_WARN("Invalid depth func name: {0}", name);
            return DEPTH_FUNC_LEQUAL;
    }
}

}
