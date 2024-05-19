#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t argb_color_t;

#define ARGB_COLOR(r, g, b, a) (0) // FIXME: implement

typedef enum pvr_material_mode {
    PVR_MATERIAL_MODE_DIFFUSE = 0x1,
    PVR_MATERIAL_MODE_AMBIENT = 0x2,
    PVR_MATERIAL_MODE_SPECULAR = 0x4,
} pvr_material_mode_t;

typedef enum pvr_state {
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
} pvr_state_t;

typedef enum pvr_bool {
    PVR_FALSE,
    PVR_TRUE
} pvr_bool_t;

typedef enum pvr_depth_func {
    PVR_DEPTH_FUNC_NEVER,
    PVR_DEPTH_FUNC_ALWAYS,
    PVR_DEPTH_FUNC_LESS,
    PVR_DEPTH_FUNC_LEQUAL,
    PVR_DEPTH_FUNC_EQUAL,
    PVR_DEPTH_FUNC_GEQUAL,
    PVR_DEPTH_FUNC_GREATER,
} pvr_depth_func_t;

typedef enum pvr_winding {
    PVR_WINDING_CW,
    PVR_WINDING_CCW,
} pvr_winding_t;

typedef enum pvr_light {
    PVR_LIGHT0 = PVR_STATE_LIGHT0,
    PVR_LIGHT1 = PVR_STATE_LIGHT1,
    PVR_LIGHT2 = PVR_STATE_LIGHT2,
    PVR_LIGHT3 = PVR_STATE_LIGHT3,
} pvr_light_t;

typedef enum pvr_light_type {
    PVR_LIGHT_TYPE_DIRECTIONAL,
    PVR_LIGHT_TYPE_POINT,
    PVR_LIGHT_TYPE_SPOTLIGHT,
} pvr_light_type_t;

typedef enum pvr_light_component {
    PVR_LIGHT_COMP_AMBIENT = 0x1,
    PVR_LIGHT_COMP_DIFFUSE = 0x2,
    PVR_LIGHT_COMP_SPECULAR = 0x4,
    PVR_LIGHT_COMP_AMBIENT_AND_DIFFUSE =
        PVR_LIGHT_COMP_AMBIENT | PVR_LIGHT_COMP_DIFFUSE,
    PVR_LIGHT_COMP_DIFFUSE_AND_SPECULAR =
        PVR_LIGHT_COMP_DIFFUSE | PVR_LIGHT_COMP_SPECULAR,
} pvr_light_component_t;

typedef enum pvr_primitive {
    PVR_PRIM_TRIANGLES,
    PVR_PRIM_TRIANGLE_STRIP
} pvr_primitive_t;

typedef enum pvr_shade_model {
    PVR_SHADE_MODEL_SMOOTH,
    PVR_SHADE_MODEL_FLAT
} pvr_shade_model_t;

typedef enum pvr_clear_flag {
    PVR_CLEAR_FLAG_COLOR_BUFFER,
    PVR_CLEAR_FLAG_DEPTH_BUFFER,
} pvr_clear_flag_t;

typedef enum pvr_tex_filter {
    PVR_TEX_FILTER_NEAREST,
    PVR_TEX_FILTER_LINEAR,
    PVR_TEX_FILTER_NEAREST_MIPMAP_NEAREST,
    PVR_TEX_FILTER_LINEAR_MIPMAP_NEAREST,
    PVR_TEX_FILTER_NEAREST_MIPMAP_LINEAR,
    PVR_TEX_FILTER_LINEAR_MIPMAP_LINEAR,
} pvr_tex_filter_t;

typedef enum pvr_tex_format {
    PVR_TEX_FORMAT_INVALID,
    PVR_TEX_FORMAT_RGB565,
    PVR_TEX_FORMAT_RGBA4444,
    PVR_TEX_FORMAT_RGBA5551,
    PVR_TEX_FORMAT_RGB565_VQ,
    PVR_TEX_FORMAT_RGBA4444_VQ,
    PVR_TEX_FORMAT_RGBA5551_VQ,
    PVR_TEX_FORMAT_PAL_8BPP,
    PVR_TEX_FORMAT_PAL_4BPP,
} pvr_tex_format_t;

typedef enum pvr_palette_format {
    PVR_PALETTE_FORMAT_NONE,
    PVR_PALETTE_FORMAT_RGB565,
    PVR_PALETTE_FORMAT_RGBA4444,
    PVR_PALETTE_FORMAT_RGBA5551,
    PVR_PALETTE_FORMAT_RGBA8888,
} pvr_palette_format_t;

/**
 * @brief The PVRCommandMode enum
 *
 * When making calls to draw primitives, apply scissor rectangles etc.
 * the context buffer will be filled with PVR commands and vertices. In
 * DIRECT mode, the buffer will be submitted after each draw call. In QUEUE
 * mode the buffer will be appended until a call to pvr_flush or pvr_finish
 * is made.
 */
typedef enum pvr_command_mode {
    PVR_COMMAND_MODE_DIRECT,
    PVR_COMMAND_MODE_QUEUE,
} pvr_command_mode_t;

typedef enum pvr_target_list {
    PVR_TARGET_LIST_AUTO,
    PVR_TARGET_LIST_OPAQUE,
    PVR_TARGET_LIST_OPAQUE_MODIFIER,
    PVR_TARGET_LIST_PUNCH_THROUGH,
    PVR_TARGET_LIST_TRANSPARENT,
    PVR_TARGET_LIST_TRANSPARENT_MODIFIER,
} pvr_target_list_t;

typedef enum pvr_blend_factor {
    PVR_BLEND_FACTOR_ZERO,
    PVR_BLEND_FACTOR_ONE,
    PVR_BLEND_FACTOR_OTHER_COLOR,
    PVR_BLEND_FACTOR_ONE_MINUS_OTHER_COLOR,
    PVR_BLEND_FACTOR_SRC_ALPHA,
    PVR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    PVR_BLEND_FACTOR_DST_ALPHA,
    PVR_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
} pvr_blend_factor_t;

typedef enum pvr_matrix_mode {
    PVR_MATRIX_MODE_MODELVIEW,
    PVR_MATRIX_MODE_PROJECTION,
} pvr_matrix_mode_t;

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
void pvr_start(pvr_command_mode_t mode, void* buffer);

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
void pvr_set_target_list(pvr_target_list_t list);

void pvr_viewport(int x, int y, int width, int height);
void pvr_shade_model(pvr_shade_model_t model);
void pvr_blend_func(pvr_blend_factor_t src_factor, pvr_blend_factor_t dst_factor);
void pvr_clear_color(argb_color_t color);
void pvr_clear(pvr_clear_flag_mask_t mask);
void pvr_ambient_color(argb_color_t color);
void pvr_material(pvr_material_mode_t mode, argb_color_t color);
void pvr_specular(float shininess);
void pvr_color_material(pvr_material_mode_mask_t mask);
void pvr_depth_mask(pvr_bool_t value);
void pvr_depth_func(pvr_depth_func_t func);
void pvr_enable(pvr_state_t state);
void pvr_disable(pvr_state_t state);
void pvr_front_face(pvr_winding_t winding);
void pvr_scissor(int x, int y, int width, int height);
void pvr_light(pvr_light_t i, pvr_light_type_t type, pvr_light_component_t components,
               pvr_vec3_t* position);

void pvr_light_color(pvr_light_t i, pvr_light_component_t comp, argb_color_t color);
void pvr_light_attenuation(pvr_light_t i, float constant, float linear,
                           float quadratic);

void pvr_set_matrix_identity(pvr_matrix_mode_t mode);
void pvr_set_matrix(pvr_matrix_mode_t mode, pvr_mat4_t* matrix);

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

void pvr_draw_arrays(pvr_primitive_t prim, size_t start, size_t count);
void pvr_draw_elements(pvr_primitive_t prim, size_t count, const uint32_t* indices);
void pvr_multi_draw_arrays(pvr_primitive_t prim, size_t* starts, size_t* counts,
                           size_t draw_count);

void pvr_tex_mode(pvr_tex_format_t format, int mipmaps, pvr_bool_t twiddled);
void pvr_tex_image(int level, int w, int h, void* data);
void pvr_tex_filter(pvr_tex_filter_t min, pvr_tex_filter_t mag);
void pvr_clut_mode(pvr_palette_format_t format);
void pvr_clut_load(int entries, void* palette);

#ifdef __cplusplus
}
#endif
