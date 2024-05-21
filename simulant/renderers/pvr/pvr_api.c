#include <dc/pvr.h>
#include <stdio.h>
#include <string.h>

#include "pvr_api.h"
#include "pvr_private.h"

static struct {
    pvr_poly_header_t poly_conf;

    /* We use 'work commands' which have room for
     * our own extra data, and are 8-byte aligned */
    pvr_work_command_t* command_list;
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

static inline bool is_vertex(pvr_command_t* command) {
    return command->header.para_type == 7;
}

static inline bool is_last_vertex(pvr_command_t* command) {
    return command->header.end_of_strip;
}

static inline float fast_invert(float x) {
    return (1.f / __builtin_sqrtf(x * x));
}

static inline void perspective_divide(pvr_vertex3_t* vertex, const float h) {
    TRACE();

    const float f = fast_invert(vertex->w);

    /* Convert to NDC and apply viewport */
    vertex->xyz[0] = (vertex->xyz[0] * f * 320) + 320;
    vertex->xyz[1] = (vertex->xyz[1] * f * -240) + 240;

    /* Orthographic projections need to use invZ otherwise we lose
    the depth information. As w == 1, and clip-space range is -w to +w
    we add 1.0 to the Z to bring it into range. We add a little extra to
    avoid a divide by zero.
    */
    if(vertex->w == 1.0f) {
        vertex->xyz[2] = fast_invert(1.0001f + vertex->xyz[2]);
    } else {
        vertex->xyz[2] = f;
    }
}

static inline void prepare_store_queues() {
    //Set PVR DMA registers
    *PVR_LMMODE0 = 0;
    *PVR_LMMODE1 = 0;

    //Set QACR registers
    QACR[1] = QACR[0] = 0x11;
}

static inline void await_store_queues() {
    /* Wait for both store queues to complete */
    sq = (uint32_t*) 0xe0000000;
    sq[0] = sq[8] = 0;
}

static inline void ta_submit(pvr_command_t* v)  {
    uint32_t* s = (uint32_t*) v;
    sq[0] = *(s++);
    sq[1] = *(s++);
    sq[2] = *(s++);
    sq[3] = *(s++);
    sq[4] = *(s++);
    sq[5] = *(s++);
    sq[6] = *(s++);
    sq[7] = *(s++);
    __asm__("pref @%0" : : "r"(sq));
    sq += 8;
}

static inline void clip_edge(const pvr_vertex3_t* const v1, const pvr_vertex3_t* const v2, pvr_vertex3_t* vout) {
    const float o = 0.003921569f;  // 1 / 255
    const float d0 = v1->w + v1->xyz[2];
    const float d1 = v2->w + v2->xyz[2];
    const float t = (fabs(d0) * (1.0f / sqrtf((d1 - d0) * (d1 - d0))));
    const float invt = 1.0f - t;

    vout->xyz[0] = invt * v1->xyz[0] + t * v2->xyz[0];
    vout->xyz[1] = invt * v1->xyz[1] + t * v2->xyz[1];
    vout->xyz[2] = invt * v1->xyz[2] + t * v2->xyz[2];
    vout->xyz[2] = (vout->xyz[2] < FLT_EPSILON) ? FLT_EPSILON : vout->xyz[2];

    vout->uv[0] = invt * v1->uv[0] + t * v2->uv[0];
    vout->uv[1] = invt * v1->uv[1] + t * v2->uv[1];

    vout->w = invt * v1->w + t * v2->w;

    const float m = 255 * t;
    const float n = 255 - m;

    vout->bgra[0] = (v1->bgra[0] * n + v2->bgra[0] * m) * o;
    vout->bgra[1] = (v1->bgra[1] * n + v2->bgra[1] * m) * o;
    vout->bgra[2] = (v1->bgra[2] * n + v2->bgra[2] * m) * o;
    vout->bgra[3] = (v1->bgra[3] * n + v2->bgra[3] * m) * o;
}

typedef enum visible {
    NONE_VISIBLE = 0,
    FIRST_VISIBLE = 1,
    SECOND_VISIBLE = 2,
    THIRD_VISIBLE = 4,
    FIRST_AND_SECOND_VISIBLE = FIRST_VISIBLE | SECOND_VISIBLE,
    SECOND_AND_THIRD_VISIBLE = SECOND_VISIBLE | THIRD_VISIBLE,
    FIRST_AND_THIRD_VISIBLE = FIRST_VISIBLE | THIRD_VISIBLE,
    ALL_VISIBLE = 7
} visible_t;


void process_list(pvr_vertex3_t* vertices, size_t n) {
    prepare_store_queues();

    /* This is a bit cumbersome - in some cases (particularly case 2)
       we finish the vertex submission with a duplicated final vertex so
       that the tri-strip can be continued. However, if the next triangle in the
       strip is not visible then the duplicated vertex would've been sent without
       the EOL flag. We won't know if we need the EOL flag or not when processing
       case 2. To workaround this we may queue a vertex temporarily here, in the normal
       case it will be submitted by the next iteration with the same flags it had, but
       in the invisible case it will be overridden to submit with EOL */
    static pvr_vertex3_t __attribute__((aligned(32))) qv;
    pvr_vertex3_t* queued_vertex = NULL;

#define QUEUE_VERTEX(v) \
    do { queued_vertex = &qv; *queued_vertex = *(v); } while(0)

#define SUBMIT_QUEUED_VERTEX(end_of_strip) \
    do { if(queued_vertex) { queued_vertex->header.end_of_strip = (end_of_strip); ta_submit(queued_vertex); queued_vertex = NULL; } } while(0)

    uint32_t visible_mask = 0;

    pvr_work_command_t* it = vertices;

    for(int i = 0; i < n - 1; ++i, ++it) {
        pvr_vertex3_t* v0 = &it->v;

        if(is_header(v0)) {
            ta_submit(v0);
            visible_mask = 0;
            continue;
        }

        pvr_vertex3_t* v1 = &(it + 1)->v;
        pvr_vertex3_t* v2 = (i < n - 2) ? &(it + 2)->v : NULL;

        assert(!is_header(v1));

        // We are trailing if we're on the penultimate vertex, or the next but one vertex is
        // an EOL, or v1 is an EOL (FIXME: possibly unnecessary and coverted by the other case?)
        bool is_trailing = is_last_vertex(v1) || ((v2) ? is_header(v2) : true);

        if(is_trailing) {
            // OK so we've hit a new context header
            // we need to finalize this strip and move on

            // If the last triangle was all visible, we need
            // to submit the last two vertices, any clipped triangles
            // would've
            if(visible_mask == ALL_VISIBLE) {
                SUBMIT_QUEUED_VERTEX(qv.header.end_of_strip);

                perspective_divide(v0, h);
                ta_submit(v0);

                v1.header.end_of_strip = true;

                perspective_divide(v1, h);
                ta_submit(v1);
            } else {
                // If the previous triangle wasn't all visible, and we
                // queued a vertex - we force it to be EOL and submit
                SUBMIT_QUEUED_VERTEX(true);
            }

            i++;
            v0++;
            visible_mask = 0;
            continue;
        }

        visible_mask = (
            (v0->xyz[2] >= -v0->w) << 0 |
            (v1->xyz[2] >= -v1->w) << 1 |
            (v2->xyz[2] >= -v2->w) << 2
            );

        /* If we've gone behind the plane, we finish the strip
        otherwise we submit however it was */
        if(visible_mask == NONE_VISIBLE) {
            SUBMIT_QUEUED_VERTEX(true);
        } else {
            SUBMIT_QUEUED_VERTEX(qv.header.end_of_strip);
        }

        pvr_vertex3_t __attribute__((aligned(32))) scratch[4];
        pvr_vertex3_t a = &scratch[0], *b = &scratch[1], *c = &scratch[2], *d = &scratch[3];

        switch(visible_mask) {
        case ALL_VISIBLE:
            perspective_divide(v0, h);
            QUEUE_VERTEX(v0);
            break;
        case NONE_VISIBLE:
            break;
            break;
        case FIRST_VISIBLE:
            clip_edge(v0, v1, a);
            a.header.end_of_strip = false;

            clip_edge(v2, v0, b);
            b->header.end_of_strip = false;

            perspective_divide(v0, h);
            ta_submit(v0);

            perspective_divide(a, h);
            ta_submit(a);

            perspective_divide(b, h);
            ta_submit(b);

            QUEUE_VERTEX(b);
            break;
        case SECOND_VISIBLE:
            memcpy_vertex(c, v1);

            clip_edge(v0, v1, a);
            a.header.end_of_strip = false;

            clip_edge(v1, v2, b);
            b->header.end_of_strip = v2.header.end_of_strip;

            perspective_divide(a, h);
            ta_submit(a);

            perspective_divide(c, h);
            ta_submit(c);

            perspective_divide(b, h);
            QUEUE_VERTEX(b);
            break;
        case THIRD_VISIBLE:
            memcpy_vertex(c, v2);

            clip_edge(v2, v0, a);
            a.header.end_of_strip = false;

            clip_edge(v1, v2, b);
            b->header.end_of_strip = false;

            perspective_divide(a, h);
            //ta_submit(a);
            ta_submit(a);

            perspective_divide(b, h);
            ta_submit(b);

            perspective_divide(c, h);
            QUEUE_VERTEX(c);
            break;
        case FIRST_AND_SECOND_VISIBLE:
            memcpy_vertex(c, v1);

            clip_edge(v2, v0, b);
            b->header.end_of_strip = false;

            perspective_divide(v0, h);
            ta_submit(v0);

            clip_edge(v1, v2, a);
            a.header.end_of_strip = v2.header.end_of_strip;

            perspective_divide(c, h);
            ta_submit(c);

            perspective_divide(b, h);
            ta_submit(b);

            perspective_divide(a, h);
            ta_submit(c);

            QUEUE_VERTEX(a);
            break;
        case SECOND_AND_THIRD_VISIBLE:
            memcpy_vertex(c, v1);
            memcpy_vertex(d, v2);

            clip_edge(v0, v1, a);
            a.header.end_of_strip = false;

            clip_edge(v2, v0, b);
            b->header.end_of_strip = false;

            perspective_divide(a, h);
            ta_submit(a);

            perspective_divide(c, h);
            ta_submit(c);

            perspective_divide(b, h);
            ta_submit(b);
            ta_submit(c);

            perspective_divide(d, h);
            QUEUE_VERTEX(d);
            break;
        case FIRST_AND_THIRD_VISIBLE:
            memcpy_vertex(c, v2);
            c->flags = GPU_CMD_VERTEX;
            c->header.end_of_strip = false;

            clip_edge(v0, v1, a);
            a->header.end_of_strip = false;

            clip_edge(v1, v2, b);
            b->header.end_of_strip = false;

            perspective_divide(v0, h);
            ta_submit(v0);

            perspective_divide(a, h);
            ta_submit(a);

            perspective_divide(c, h);
            ta_submit(c);
            perspective_divide(b, h);
            ta_submit(b);
            QUEUE_VERTEX(c);
            break;
        default:
            break;
        }
    }

    SUBMIT_QUEUED_VERTEX(true);

    await_store_queues();
}


void pvr_flush() {
    state.lists_submitting = true;

    process_list(state.command_list, state.command_counter);

    state.command_counter = 0;
}

void pvr_start(pvr_command_mode_t mode, void* context) {
    state.command_list = (pvr_work_command_t*) context;
    state.command_counter = 0;

    // Start the command list of with the header state
    memcpy(state.command_list[state.command_counter++], state.header,
           sizeof(state.header));

    pvr_wait_ready();

    // Reset everything to the opaque list
    state.poly_conf.header.list_type = PVR_POLY_LIST_OPAQUE;
    ta_list_init();
}

void pvr_finish() {
    pvr_flush();
}

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

void pvr_enable(pvr_state_t s) {}

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
