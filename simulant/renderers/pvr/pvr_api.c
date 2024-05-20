#include <dc/pvr.h>
#include <string.h>

#include "pvr_api.h"

/* Note: kos already has a pvr_list_t however
 * this has nicer type-safety and is only used
 * internally. This is similar but different to the
 * target list (which is a user-facing enum with an AUTO)
 * pvr_poly_list_t is an explicit list type. The order matches
 * the header value.
 */

typedef enum {
    PVR_POLY_LIST_OPAQUE = PVR_LIST_OP_POLY,
    PVR_POLY_LIST_OPAQUE_MODIFIER = PVR_LIST_OP_MOD,
    PVR_POLY_LIST_TRANSPARENT = PVR_LIST_TR_POLY,
    PVR_POLY_LIST_TRANSPARENT_MODIFIER = PVR_LIST_TR_MOD,
    PVR_POLY_LIST_PUNCH_THROUGH = PVR_LIST_PT_POLY,
} pvr_poly_list_t;

/* This is a nicer version of pvr_poly_hdr_t
 * that doesn't require compiling (which is slow)
 */
typedef struct pvr_command_header {
    unsigned int para_type : 3;
    unsigned int end_of_strip : 1;
    unsigned int unknown : 1;
    unsigned int list_type : 3;

    unsigned int group_enable : 1;
    unsigned int unknown2 : 3;
    unsigned int strip_length : 2;
    unsigned int user_clip : 2;

    unsigned int unknown3 : 8;

    unsigned int shadow : 1;
    unsigned int volume : 1;
    unsigned int color_type : 2;
    unsigned int texture : 1;
    unsigned int offset : 1;
    unsigned int gouraud : 1;
    unsigned int uv_16bit : 1;
} pvr_command_header_t;

typedef struct pvr_poly_instruction {
    unsigned int depth_func : 3;
    unsigned int cull_mode : 2;
    unsigned int write_disable : 1;
    unsigned int texture : 1;
    unsigned int offset : 1;
    unsigned int gouraud : 1;
    unsigned int uv_16bit : 1;
    unsigned int cache_bypass : 1;
    unsigned int mipmap_bias : 1;
    unsigned int unknown : 20;
} pvr_poly_instruction_t;

typedef struct pvr_mod_instruction {
    unsigned int volume_instruction : 3;
    unsigned int culling_mode : 2;
    unsigned int unknown : 27;
} pvr_mod_instruction_t;

typedef struct pvr_tsp_instruction {
    unsigned int src_blend : 3;
    unsigned int dst_blend : 3;
    unsigned int src_select : 1;
    unsigned int dst_select : 1;
    unsigned int fog_control : 2;
    unsigned int color_clamp : 1;
    unsigned int use_alpha : 1;
    unsigned int ignore_tex_alpha : 1;
    unsigned int flip_uv : 2;
    unsigned int clamp_uv : 2;
    unsigned int filter_mode : 2;
    unsigned int super_sample : 1;
    unsigned int mipmap_bias : 4;
    unsigned int texture_shading : 2;
    unsigned int texture_width : 3;
    unsigned int texture_height : 3;
} pvr_tsp_instruction_t;

typedef struct pvr_rgb_texture_control {
    unsigned int mipmap_enabled : 1;
    unsigned int compressed : 1;
    unsigned int pixel_format : 3;
    unsigned int scan_order : 1;
    unsigned int stride_select : 1;
    unsigned int unknown : 4;
    unsigned int data_address : 21;
} pvr_rgb_texture_control_t;

typedef struct pvr_pal_texture_control {
    unsigned int mipmap_enabled : 1;
    unsigned int compressed : 1;
    unsigned int pixel_format : 3;
    unsigned int palette_selector : 6;
    unsigned int data_address : 21;
} pvr_pal_texture_control_t;

/* Generic command */
typedef struct {
    pvr_command_header_t header;
    uint32_t data[7];
} pvr_command_t;

/*
 * Our standard vertex command (aka pvr_vertex_t)
 */
typedef struct {
    pvr_command_header_t header;
    float x, y, z;
    float u, v;
    argb_color_t color;
    argb_color_t offset_color;
} pvr_vertex3_t;

typedef struct {
    pvr_command_header_t header;
    uint32_t isp_tsp_instruction;
    pvr_tsp_instruction_t tsp_struction;
    uint32_t texture_control;
    uint32_t padding[4];
} pvr_poly_header_t;

static struct {
    pvr_poly_header_t poly_conf;
    pvr_command_t* command_list;
    size_t command_counter;
} state;

#define REG_SOFTRESET 0x005F8008
#define REG_TA_LIST_INIT 0x005F8144
#define REG_TA_LIST_CONT 0x005F8160

static void ta_soft_reset() {
    PVR_SET(REG_SOFTRESET, 1);
}

static void ta_list_init() {
    PVR_SET(REG_TA_LIST_INIT, 0x80000000);
    PVR_GET(REG_TA_LIST_INIT);
}

static void ta_list_continue() {
    PVR_SET(REG_TA_LIST_CONT, 0x80000000);
    PVR_GET(REG_TA_LIST_CONT);
}

static void ta_await_list_finish(pvr_poly_list_t list) {}

static void set_active_pvr_list(pvr_poly_list_t list) {
    pvr_poly_list_t current_list = state.poly_conf.header.list_type;

    if(list == current_list) {
        return;
    }

    ta_await_list_finish(current_list);

    if(list > current_list) {
        /* We can just move onto the list, we don't need to do
         * multi-pass */

    } else {
        /* Need to continue the list */
        ta_list_continue();
    }

    state.poly_conf.header.list_type = list;
}

static bool started = false;

/*
 * We need to hijack the interrupts set up by KOS
 * as we need to manually handle list submission etc.
 *
 * If this ever gets upstreamed we'll need to figure out how the
 * existing API and this one interact, or maybe this one can replace it */

void pvr_api_init() {

    started = true;
}

void pvr_start(pvr_command_mode_t mode, void* context) {
    state.command_list = (command_t*)context;
    state.command_counter = 0;

    // Start the command list of with the header state
    memcpy(state.command_list[state.command_counter++], state.header,
           sizeof(state.header));

    // Reset everything to the opaque list
    state.poly_conf.header.list_type = PVR_POLY_LIST_OPAQUE;
    ta_list_init();
}

void pvr_finish() {}

void pvr_viewport(int x, int y, int width, int height) {}

void pvr_shade_model(pvr_shade_model_t model) {}

void pvr_clear_color(argb_color_t color) {
    // FIXME: Read from color
    pvr_set_bg_color(0, 0, 0);
}

void pvr_clear(pvr_clear_flag_mask_t mask) {}

void pvr_ambient_color(argb_color_t color) {}

void pvr_material(pvr_material_mode_t mode, argb_color_t color) {}

void pvr_specular(float shininess) {}

void pvr_color_material(pvr_material_mode_mask_t mask) {}

void pvr_depth_mask(pvr_bool_t value) {}

void pvr_depth_func(pvr_depth_func_t func) {}

void pvr_enable(pvr_state_t state) {}

void pvr_disable(pvr_state_t state) {}

void pvr_front_face(pvr_winding_t winding) {}

void pvr_scissor(int x, int y, int width, int height) {}

void pvr_light(pvr_light_t i, pvr_light_type_t type,
               pvr_light_component_t components, pvr_vec3_t* position) {}

void pvr_light_color(pvr_light_t i, pvr_light_component_t comp,
                     argb_color_t color) {}

void pvr_light_attenuation(pvr_light_t i, float constant, float linear,
                           float quadratic) {}

void pvr_vertex_pointers(pvr_vec3_t* positions, pvr_vec2_t* uvs,
                         argb_color_t* colors, argb_color_t* color_offsets,
                         pvr_vec3_t* normals, pvr_vec2_t* sts, size_t stride) {}

void pvr_draw_arrays(pvr_primitive_t prim, size_t start, size_t count) {}

void pvr_draw_elements(pvr_primitive_t prim, size_t count,
                       const uint32_t* indices) {}

void pvr_multi_draw_arrays(pvr_primitive_t prim, size_t* starts, size_t* counts,
                           size_t draw_count) {}

void pvr_tex_mode(pvr_tex_format_t format, int mipmaps, pvr_bool_t twiddled) {}

void pvr_tex_image(int level, int w, int h, void* data) {}

void pvr_tex_filter(pvr_tex_filter_t min, pvr_tex_filter_t mag) {}

void pvr_clut_mode(pvr_palette_format_t format) {}

void pvr_clut_load(int entries, void* palette) {}

void pvr_set_matrix_identity(pvr_matrix_mode_t mode) {}

void pvr_set_matrix(pvr_matrix_mode_t mode, pvr_mat4_t* matrix) {}

void pvr_blend_func(pvr_blend_factor_t src_factor,
                    pvr_blend_factor_t dst_factor) {}
