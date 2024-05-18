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
    PVR_STATE_ALPHA_TEST,
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

/**
 * @brief The PVRCommandMode enum
 *
 * When making calls to draw primitives, apply scissor rectangles etc.
 * the context buffer will be filled with PVR commands and vertices. In
 * DIRECT mode, the buffer will be submitted after each draw call. In QUEUE
 * mode the buffer will be appended until a call to pvr_flush or pvr_finish
 * is made.
 */
enum PVRCommandMode {
    PVR_COMMAND_MODE_DIRECT,
    PVR_COMMAND_MODE_QUEUE,
};

enum PVRTargetList {
    PVR_TARGET_LIST_AUTO,
    PVR_TARGET_LIST_OPAQUE,
    PVR_TARGET_LIST_OPAQUE_MODIFIER,
    PVR_TARGET_LIST_PUNCH_THROUGH,
    PVR_TARGET_LIST_TRANSPARENT,
    PVR_TARGET_LIST_TRANSPARENT_MODIFIER,
};

enum PVRBlendFactor {
    PVR_BLEND_FACTOR_ZERO,
    PVR_BLEND_FACTOR_ONE,
    PVR_BLEND_FACTOR_OTHER_COLOR,
    PVR_BLEND_FACTOR_ONE_MINUS_OTHER_COLOR,
    PVR_BLEND_FACTOR_SRC_ALPHA,
    PVR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    PVR_BLEND_FACTOR_DST_ALPHA,
    PVR_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
};

enum PVRMatrixMode {
    PVR_MATRIX_MODE_MODELVIEW,
    PVR_MATRIX_MODE_PROJECTION,
};

typedef struct pvr_vec3 {
    float x, y, z;
} pvr_vec3_t;

typedef struct pvr_vec2 {
    float x, y;
} pvr_vec2_t;

typedef struct pvr_mat4 {
    float m[16];
} pvr_mat4_t;

typedef uint32_t pvr_material_mode_mask_t;
typedef uint32_t pvr_clear_flag_mask_t;

/**
 * @brief pvr_start starts a new PVR command list
 * @param mode
 * Whether to flush the command list after each
 * draw call, or to wait until an explicit flush.
 *
 * @param context
 *
 * A pointer to a 32 byte aligned RAM buffer to hold
 * commands until they are ready for submission.
 */
void pvr_start(PVRCommandMode mode, void* buffer);

/**
 * @brief pvr_flush Flushes the current command buffer
 * to the pvr
 */
void pvr_flush();

/**
 * @brief pvr_finish Flushes the command buffer and finishes
 * the frame.
 */
void pvr_finish();


/**
 * @brief pvr_set_target_list
 *
 * Sets the target polygon list for subsequent drawing operations.
 * Changing the target list will implicitly flush any outstanding commands
 * in the queue.
 *
 * If the target list is PVR_TARGET_LIST_AUTO then the target list
 * will be determined based on the following:
 *
 * 1. If blending is enabled, the target list will be PVR_TARGET_LIST_TRANSPARENT
 * 2. If blending is not enabled, but alpha testing is, the target list will
 *    be PVR_TARGET_LIST_PUNCH_THROUGH
 * 3. If blending is not enabled, and alpha testing is disabled, the target
 *    list will be PVR_TARGET_LIST_OPAQUE
 *
 * @param list  the target list to set
 *
 */
void pvr_set_target_list(PVRTargetList list);

void pvr_viewport(int x, int y, int width, int height);
void pvr_shade_model(PVRShadeModel model);
void pvr_blend_func(PVRBlendFactor src_factor, PVRBlendFactor dst_factor);
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

void pvr_set_matrix_identity(PVRMatrixMode mode);
void pvr_set_matrix(PVRMatrixMode mode, pvr_mat4_t* matrix);

/**
 * @brief pvr_vertex_pointer
 *
 * Set up source data for pvr_draw_arrays etc.
 *
 * If `stride` is zero, then it is assumed all
 * arrays are tightly packed. If stride is non-zero
 * then it is assumed all arrays are interleved by
 * `stride`
 *
 * @param positions
 * @param uvs
 * @param colors
 * @param color_offsets
 * @param normals
 * @param sts
 * @param stride
 */
void pvr_vertex_pointers(
    pvr_vec3_t* positions,
    pvr_vec2_t* uvs,
    argb_color_t* colors,
    argb_color_t* color_offsets,
    pvr_vec3_t* normals,
    pvr_vec2_t* sts,
    size_t stride
);

void pvr_draw_arrays(PVRPrimitive prim, size_t start, size_t count);
void pvr_draw_elements(PVRPrimitive prim, size_t count, const uint32_t* indices);
void pvr_multi_draw_arrays(PVRPrimitive prim, size_t* starts, size_t* counts,
                           size_t draw_count);

void pvr_tex_mode(PVRTexFormat format, int mipmaps, bool twiddled);
void pvr_tex_image(int level, int w, int h, void* data);
void pvr_tex_filter(PVRTexFilter min, PVRTexFilter mag);
void pvr_clut_mode(PVRPaletteFormat format);
void pvr_clut_load(int entries, void* palette);

#ifdef __cplusplus
}
#endif
