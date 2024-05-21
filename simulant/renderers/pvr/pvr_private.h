#pragma once

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
    union {
        __attribute__((aligned(8))) pvr_vertex3_t v;
        __attribute__((aligned(8))) pvr_command_t c;
    };

    float w;
    uint32_t unused;
} pvr_work_command_t;

typedef struct {
    pvr_command_header_t header;
    union {
        pvr_poly_instruction_t poly;
        pvr_mod_instruction_t mod;
    } isp_tsp_instruction;
    pvr_tsp_instruction_t tsp_instruction;
    union {
        pvr_rgb_texture_control_t rgb;
        pvr_pal_texture_control_t pal;
    } texture_control;
    uint32_t padding[4];
} pvr_poly_header_t;
