#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t argb_color_t;

#define ARGB_COLOR(r, g, b, a) (0) // FIXME: implement

enum PVRMaterialMode {
    PVR_MATERIAL_MODE_DIFFUSE = 0x1,
    PVR_MATERIAL_MODE_AMBIENT = 0x2,
    PVR_MATERIAL_MODE_SPECULAR = 0x4,
};

enum PVRState {
    PVR_STATE_LIGHTING,
    PVR_STATE_DEPTH_TEST,
    PVR_STATE_CULL_FACE,
    PVR_STATE_BLENDING,
    PVR_STATE_TEXTURE_2D,
    PVR_STATE_SCISSOR_TEST,
    PVR_STATE_LIGHT0,
    PVR_STATE_LIGHT1,
    PVR_STATE_LIGHT2,
    PVR_STATE_LIGHT3
};

enum PVRBool {
    PVR_FALSE,
    PVR_TRUE
};

enum PVRDepthFunc {
    PVR_DEPTH_FUNC_NEVER,
    PVR_DEPTH_FUNC_ALWAYS,
    PVR_DEPTH_FUNC_LESS,
    PVR_DEPTH_FUNC_LEQUAL,
    PVR_DEPTH_FUNC_EQUAL,
    PVR_DEPTH_FUNC_GEQUAL,
    PVR_DEPTH_FUNC_GREATER,
};

enum PVRWinding {
    PVR_WINDING_CW,
    PVR_WINDING_CCW,
};

enum PVRLight {
    PVR_LIGHT0 = PVR_STATE_LIGHT0,
    PVR_LIGHT1 = PVR_STATE_LIGHT1,
    PVR_LIGHT2 = PVR_STATE_LIGHT2,
    PVR_LIGHT3 = PVR_STATE_LIGHT3,
};

enum PVRLightType {
    PVR_LIGHT_TYPE_DIRECTIONAL,
    PVR_LIGHT_TYPE_POINT,
    PVR_LIGHT_TYPE_SPOTLIGHT,
};

enum PVRLightComponent {
    PVR_LIGHT_COMP_AMBIENT = 0x1,
    PVR_LIGHT_COMP_DIFFUSE = 0x2,
    PVR_LIGHT_COMP_SPECULAR = 0x4,
    PVR_LIGHT_COMP_AMBIENT_AND_DIFFUSE =
        PVR_LIGHT_COMP_AMBIENT | PVR_LIGHT_COMP_DIFFUSE,
    PVR_LIGHT_COMP_DIFFUSE_AND_SPECULAR =
        PVR_LIGHT_COMP_DIFFUSE | PVR_LIGHT_COMP_SPECULAR,
};

enum PVRPrimitive {
    PVR_PRIM_TRIANGLES,
    PVR_PRIM_TRIANGLE_STRIP
};

enum PVRShadeModel {
    PVR_SHADE_MODEL_SMOOTH,
    PVR_SHADE_MODEL_FLAT
};

enum PVRClearFlag {
    PVR_CLEAR_FLAG_COLOR_BUFFER,
    PVR_CLEAR_FLAG_DEPTH_BUFFER,
};

enum PVRTexFilter {
    PVR_TEX_FILTER_NEAREST,
    PVR_TEX_FILTER_LINEAR,
    PVR_TEX_FILTER_NEAREST_MIPMAP_NEAREST,
    PVR_TEX_FILTER_LINEAR_MIPMAP_NEAREST,
    PVR_TEX_FILTER_NEAREST_MIPMAP_LINEAR,
    PVR_TEX_FILTER_LINEAR_MIPMAP_LINEAR,
};

enum PVRTexFormat {
    PVR_TEX_FORMAT_RGB565,
    PVR_TEX_FORMAT_RGBA4444,
    PVR_TEX_FORMAT_RGBA5551,
    PVR_TEX_FORMAT_RGB565_VQ,
    PVR_TEX_FORMAT_RGBA4444_VQ,
    PVR_TEX_FORMAT_RGBA5551_VQ,
    PVR_TEX_FORMAT_PAL_8BPP,
    PVR_TEX_FORMAT_PAL_4BPP,
};

enum PVRPaletteFormat {
    PVR_PALETTE_FORMAT_NONE,
    PVR_PALETTE_FORMAT_RGB565,
    PVR_PALETTE_FORMAT_RGBA4444,
    PVR_PALETTE_FORMAT_RGBA5551,
    PVR_PALETTE_FORMAT_RGBA8888,
};

typedef struct pvr_vec3 {
    float x, y, z;
} pvr_vec3_t;

typedef struct pvr_vertex {
    uint32_t flags;
    float x, y, z;
    float u, v;
    argb_color_t color;
    argb_color_t offset_color;
    // Extra stuff for processing
    float s, t;       // secondary uv
    float w;          // w coordinate
    float nx, ny, nz; // Normal

    uint8_t unused[8]; // Make the whole struct 64 bytes
} pvr_vertex_t;

typedef uint32_t pvr_material_mode_mask_t;
typedef uint32_t pvr_clear_flag_mask_t;

void pvr_start(void* context);
void pvr_finish();

void pvr_viewport(int x, int y, int width, int height);
void pvr_shade_model(PVRShadeModel model);
void pvr_clear_color(argb_color_t color);
void pvr_clear(pvr_clear_flag_mask_t mask);
void pvr_ambient_color(argb_color_t color);
void pvr_material(PVRMaterialMode mode, argb_color_t color);
void pvr_specular(float shininess);
void pvr_color_material(pvr_material_mode_mask_t mask);
void pvr_depth_mask(PVRBool value);
void pvr_depth_func(PVRDepthFunc func);
void pvr_enable(PVRState state);
void pvr_disable(PVRState state);
void pvr_front_face(PVRWinding winding);
void pvr_scissor(int x, int y, int width, int height);
void pvr_light(PVRLight i, PVRLightType type, PVRLightComponent components,
               pvr_vec3_t* position);

void pvr_light_color(PVRLight i, PVRLightComponent comp, argb_color_t color);
void pvr_light_attenuation(PVRLight i, float constant, float linear,
                           float quadratic);

void pvr_draw_array(PVRPrimitive prim, size_t start, size_t count,
                    pvr_vertex_t* vertices);

void pvr_tex_mode(PVRTexFormat format, int mipmaps, bool twiddled);
void pvr_tex_image(int level, int w, int h, void* data);
void pvr_tex_filter(PVRTexFilter min, PVRTexFilter mag);
void pvr_clut_mode(PVRPaletteFormat format);
void pvr_clut_load(int entries, void* palette);

#ifdef __cplusplus
}
#endif
