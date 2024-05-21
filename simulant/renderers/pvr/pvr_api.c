#include <dc/pvr.h>
#include <stdio.h>
#include <string.h>

#include "pvr_api.h"
#include "pvr_private.h"

static struct {
    pvr_poly_header_t poly_conf;
    pvr_command_t* command_list;
    size_t command_counter;
    pvr_bool_t lists_submitting;
} state = {{}, NULL, 0, PVR_FALSE};

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

void pvr_interrupt_handler(uint32_t code, void* data) {
    switch(code) {
        case ASIC_EVT_PVR_OPAQUEDONE:
        case ASIC_EVT_PVR_TRANSDONE:
        case ASIC_EVT_PVR_OPAQUEMODDONE:
        case ASIC_EVT_PVR_TRANSMODDONE:
        case ASIC_EVT_PVR_PTDONE:
            state.lists_submitting = PVR_FALSE;
            break;
        case ASIC_EVT_PVR_RENDERDONE_TSP:
        case ASIC_EVT_PVR_VBLANK_BEGIN:
            break;
        case ASIC_EVT_PVR_ISP_OUTOFMEM:
        case ASIC_EVT_PVR_STRIP_HALT:
        case ASIC_EVT_PVR_OPB_OUTOFMEM:
        case ASIC_EVT_PVR_TA_INPUT_ERR:
        case ASIC_EVT_PVR_TA_INPUT_OVERFLOW:
        default:
            fprintf(stderr, "PVR Error: %d\n", code);
            break;
    }
}

static void ta_await_list_finish(pvr_poly_list_t list) {
    while(state.lists_submitting) {}
}

/* Called when we start targetting a new list. This will do one of several
 * things:
 *
 * 1. If the passed list is the same as the current list, this is a no-op
 * 2. If the new list is numerically after the current list, it will also do
 * nothing
 * 3. If the new list is numerically less than the current list, it will flush
 * the command queue, await the list processing to finish, and then submit a
 * list continuation
 */
static void set_active_pvr_list(pvr_poly_list_t list) {
    pvr_poly_list_t current_list = state.poly_conf.header.list_type;

    if(list == current_list) {
        return;
    }    

    if(list > current_list) {
        /* We can just move onto the list, we don't need to do
         * multi-pass */

    } else {
        // Flush the outstanding command buffer before
        // commencing the next list
        pvr_flush();

        ta_await_list_finish(current_list);

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

static void enable_interrupt_handlers() {
    asic_evt_set_handler(ASIC_EVT_PVR_OPAQUEDONE, pvr_interrupt_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_OPAQUEDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_OPAQUEMODDONE, pvr_interrupt_handler,
                         NULL);
    asic_evt_enable(ASIC_EVT_PVR_OPAQUEMODDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_TRANSDONE, pvr_interrupt_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_TRANSDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_TRANSMODDONE, pvr_interrupt_handler,
                         NULL);
    asic_evt_enable(ASIC_EVT_PVR_TRANSMODDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_PTDONE, pvr_interrupt_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_PTDONE, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_RENDERDONE_TSP, pvr_interrupt_handler,
                         NULL);
    asic_evt_enable(ASIC_EVT_PVR_RENDERDONE_TSP, ASIC_IRQ_DEFAULT);

    asic_evt_set_handler(ASIC_EVT_PVR_ISP_OUTOFMEM, pvr_interrupt_handler,
                         NULL);
    asic_evt_enable(ASIC_EVT_PVR_ISP_OUTOFMEM, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_STRIP_HALT, pvr_interrupt_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_STRIP_HALT, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_OPB_OUTOFMEM, pvr_interrupt_handler,
                         NULL);
    asic_evt_enable(ASIC_EVT_PVR_OPB_OUTOFMEM, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_TA_INPUT_ERR, pvr_interrupt_handler,
                         NULL);
    asic_evt_enable(ASIC_EVT_PVR_TA_INPUT_ERR, ASIC_IRQ_DEFAULT);
    asic_evt_set_handler(ASIC_EVT_PVR_TA_INPUT_OVERFLOW, pvr_interrupt_handler,
                         NULL);
    asic_evt_enable(ASIC_EVT_PVR_TA_INPUT_OVERFLOW, ASIC_IRQ_DEFAULT);
}

void pvr_api_init() {
    enable_interrupt_handlers();
    started = true;
}

void pvr_flush() {
    state.lists_submitting = true;

    for(size_t i = 0; i < state.command_counter; ++i) {}

    state.command_counter = 0;
}

void pvr_start(pvr_command_mode_t mode, void* context) {
    state.command_list = (command_t*)context;
    state.command_counter = 0;

    // Start the command list of with the header state
    memcpy(state.command_list[state.command_counter++], state.header,
           sizeof(state.header));

    pvr_wait_ready();

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
