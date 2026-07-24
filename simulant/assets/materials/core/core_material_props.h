#pragma once
#include "../constants.h"
#include "core_material.h"

namespace smlt {

/* Flat storage for all core non-texture, non-matrix material properties.
 * ~128 bytes — fits in 4 SH4 cache lines. Replaces per-property pool
 * allocations for the hottest render-state fields. */
struct CoreMaterialProps {
    Color   base_color          = Color::white();
    Color   fog_color           = Color::white();
    float   metallic            = 0.0f;
    float   roughness           = 0.4f;
    float   line_width          = 1.0f;
    float   alpha_threshold     = 1.0f;
    float   fog_density         = 1.0f;
    float   fog_start           = 100.0f;
    float   fog_end             = 1000.0f;
    int32_t textures_enabled    = (int32_t)(BASE_COLOR_MAP_ENABLED | LIGHT_MAP_ENABLED |
                                            METALLIC_ROUGHNESS_MAP_ENABLED | NORMAL_MAP_ENABLED);
    int32_t blend_func          = (int32_t)BLEND_NONE;
    int32_t depth_func          = (int32_t)DEPTH_FUNC_LEQUAL;
    int32_t cull_mode           = (int32_t)CULL_MODE_NONE;
    int32_t shade_model         = (int32_t)SHADE_MODEL_SMOOTH;
    int32_t color_material      = (int32_t)COLOR_MATERIAL_NONE;
    int32_t polygon_mode        = (int32_t)POLYGON_MODE_FILL;
    int32_t fog_mode            = (int32_t)FOG_MODE_NONE;
    bool    depth_write_enabled  = true;
    bool    depth_test_enabled   = true;
    bool    lighting_enabled     = true;
    bool    stencil_test_enabled = false;
    bool    color_write_enabled  = true;
    int32_t stencil_func         = (int32_t)STENCIL_FUNC_ALWAYS;
    int32_t stencil_ref          = 0;
    int32_t stencil_mask         = 0xFF;
    int32_t stencil_fail         = (int32_t)STENCIL_OP_KEEP;
    int32_t stencil_depth_fail   = (int32_t)STENCIL_OP_KEEP;
    int32_t stencil_pass         = (int32_t)STENCIL_OP_KEEP;
    int32_t polygon_list_target  = (int32_t)POLYGON_LIST_TARGET_NONE;
};

/* Bitmask tracking which properties a MaterialPass overrides vs. falling
 * through to the parent Material's CoreMaterialProps. */
enum CorePropMask : uint32_t {
    CORE_MASK_BASE_COLOR       = 1u << 0,
    CORE_MASK_FOG_COLOR        = 1u << 2,
    CORE_MASK_METALLIC         = 1u << 3,
    CORE_MASK_ROUGHNESS        = 1u << 4,
    CORE_MASK_LINE_WIDTH       = 1u << 6,
    CORE_MASK_ALPHA_THRESHOLD  = 1u << 7,
    CORE_MASK_FOG_DENSITY      = 1u << 8,
    CORE_MASK_FOG_START        = 1u << 9,
    CORE_MASK_FOG_END          = 1u << 10,
    CORE_MASK_TEXTURES_ENABLED = 1u << 11,
    CORE_MASK_BLEND_FUNC       = 1u << 12,
    CORE_MASK_DEPTH_FUNC       = 1u << 13,
    CORE_MASK_CULL_MODE        = 1u << 14,
    CORE_MASK_SHADE_MODEL      = 1u << 15,
    CORE_MASK_COLOR_MATERIAL   = 1u << 16,
    CORE_MASK_POLYGON_MODE     = 1u << 17,
    CORE_MASK_FOG_MODE         = 1u << 18,
    CORE_MASK_DEPTH_WRITE      = 1u << 19,
    CORE_MASK_DEPTH_TEST       = 1u << 20,
    CORE_MASK_LIGHTING         = 1u << 21,
};

inline uint32_t core_mask_for_hash(MaterialPropertyNameHash hsh) {
    switch(hsh) {
        case BASE_COLOR_PROPERTY_HASH:          return CORE_MASK_BASE_COLOR;
        case FOG_COLOR_PROPERTY_HASH:           return CORE_MASK_FOG_COLOR;
        case METALLIC_PROPERTY_HASH:            return CORE_MASK_METALLIC;
        case ROUGHNESS_PROPERTY_HASH:           return CORE_MASK_ROUGHNESS;
        case LINE_WIDTH_PROPERTY_HASH:          return CORE_MASK_LINE_WIDTH;
        case ALPHA_THRESHOLD_PROPERTY_HASH:     return CORE_MASK_ALPHA_THRESHOLD;
        case FOG_DENSITY_PROPERTY_HASH:         return CORE_MASK_FOG_DENSITY;
        case FOG_START_PROPERTY_HASH:           return CORE_MASK_FOG_START;
        case FOG_END_PROPERTY_HASH:             return CORE_MASK_FOG_END;
        case TEXTURES_ENABLED_PROPERTY_HASH:    return CORE_MASK_TEXTURES_ENABLED;
        case BLEND_FUNC_PROPERTY_HASH:          return CORE_MASK_BLEND_FUNC;
        case DEPTH_FUNC_PROPERTY_HASH:          return CORE_MASK_DEPTH_FUNC;
        case CULL_MODE_PROPERTY_HASH:           return CORE_MASK_CULL_MODE;
        case SHADE_MODEL_PROPERTY_HASH:         return CORE_MASK_SHADE_MODEL;
        case COLOR_MATERIAL_PROPERTY_HASH:      return CORE_MASK_COLOR_MATERIAL;
        case POLYGON_MODE_PROPERTY_HASH:        return CORE_MASK_POLYGON_MODE;
        case FOG_MODE_PROPERTY_HASH:            return CORE_MASK_FOG_MODE;
        case DEPTH_WRITE_ENABLED_PROPERTY_HASH: return CORE_MASK_DEPTH_WRITE;
        case DEPTH_TEST_ENABLED_PROPERTY_HASH:  return CORE_MASK_DEPTH_TEST;
        case LIGHTING_ENABLED_PROPERTY_HASH:    return CORE_MASK_LIGHTING;
        default:                                return 0;
    }
}

/* Type-dispatched read pointer into a CoreMaterialProps.
 * Generic fallback returns nullptr (for TexturePtr, Mat4, custom props). */
template<typename T>
inline const T* core_scalar_ptr(const CoreMaterialProps&, MaterialPropertyNameHash) { return nullptr; }
template<typename T>
inline T* core_scalar_mutable_ptr(CoreMaterialProps&, MaterialPropertyNameHash) { return nullptr; }

template<>
inline const float* core_scalar_ptr<float>(const CoreMaterialProps& p, MaterialPropertyNameHash hsh) {
    switch(hsh) {
        case METALLIC_PROPERTY_HASH:        return &p.metallic;
        case ROUGHNESS_PROPERTY_HASH:       return &p.roughness;
        case LINE_WIDTH_PROPERTY_HASH:      return &p.line_width;
        case ALPHA_THRESHOLD_PROPERTY_HASH: return &p.alpha_threshold;
        case FOG_DENSITY_PROPERTY_HASH:     return &p.fog_density;
        case FOG_START_PROPERTY_HASH:       return &p.fog_start;
        case FOG_END_PROPERTY_HASH:         return &p.fog_end;
        default: return nullptr;
    }
}
template<>
inline float* core_scalar_mutable_ptr<float>(CoreMaterialProps& p, MaterialPropertyNameHash hsh) {
    return const_cast<float*>(core_scalar_ptr<float>(p, hsh));
}

template<>
inline const int32_t* core_scalar_ptr<int32_t>(const CoreMaterialProps& p, MaterialPropertyNameHash hsh) {
    switch(hsh) {
        case TEXTURES_ENABLED_PROPERTY_HASH: return &p.textures_enabled;
        case BLEND_FUNC_PROPERTY_HASH:       return &p.blend_func;
        case DEPTH_FUNC_PROPERTY_HASH:       return &p.depth_func;
        case CULL_MODE_PROPERTY_HASH:        return &p.cull_mode;
        case SHADE_MODEL_PROPERTY_HASH:      return &p.shade_model;
        case COLOR_MATERIAL_PROPERTY_HASH:   return &p.color_material;
        case POLYGON_MODE_PROPERTY_HASH:     return &p.polygon_mode;
        case FOG_MODE_PROPERTY_HASH:         return &p.fog_mode;
        default: return nullptr;
    }
}
template<>
inline int32_t* core_scalar_mutable_ptr<int32_t>(CoreMaterialProps& p, MaterialPropertyNameHash hsh) {
    return const_cast<int32_t*>(core_scalar_ptr<int32_t>(p, hsh));
}

template<>
inline const bool* core_scalar_ptr<bool>(const CoreMaterialProps& p, MaterialPropertyNameHash hsh) {
    switch(hsh) {
        case DEPTH_WRITE_ENABLED_PROPERTY_HASH: return &p.depth_write_enabled;
        case DEPTH_TEST_ENABLED_PROPERTY_HASH:  return &p.depth_test_enabled;
        case LIGHTING_ENABLED_PROPERTY_HASH:    return &p.lighting_enabled;
        default: return nullptr;
    }
}
template<>
inline bool* core_scalar_mutable_ptr<bool>(CoreMaterialProps& p, MaterialPropertyNameHash hsh) {
    return const_cast<bool*>(core_scalar_ptr<bool>(p, hsh));
}

/* Vec4: Color (r,g,b,a) and Vec4 (x,y,z,w) share the same 16-byte layout,
 * which the existing codebase already relies on via reinterpret_cast. */
template<>
inline const Vec4* core_scalar_ptr<Vec4>(const CoreMaterialProps& p, MaterialPropertyNameHash hsh) {
    switch(hsh) {
        case BASE_COLOR_PROPERTY_HASH:     return reinterpret_cast<const Vec4*>(&p.base_color);
        case FOG_COLOR_PROPERTY_HASH:      return reinterpret_cast<const Vec4*>(&p.fog_color);
        default: return nullptr;
    }
}
template<>
inline Vec4* core_scalar_mutable_ptr<Vec4>(CoreMaterialProps& p, MaterialPropertyNameHash hsh) {
    return const_cast<Vec4*>(core_scalar_ptr<Vec4>(p, hsh));
}

} // namespace smlt
