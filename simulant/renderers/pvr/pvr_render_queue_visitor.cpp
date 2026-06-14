#include "pvr_render_queue_visitor.h"
#include "pvr_renderer.h"
#include "pvr_texture_manager.h"

#include "../../meshes/submesh.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../scenes/scene.h"
#include "../../types.h"
#include "../../vertex_data.h"
#include "../../assets/material.h"
#include "../../core/aligned_vector.h"
#include "../../logging.h"

#ifdef __DREAMCAST__
#include <kos.h>
#include <dc/pvr.h>
#include <dc/matrix.h>
#include <dc/fmath.h>
#include "../../deps/sh4zam/shz_sh4zam.h"
#else
/* Provide fallback definitions for non-Dreamcast builds (stub compilation) */
#define PVR_LIST_OP_POLY 0
#define PVR_LIST_OP_MOD  1
#define PVR_LIST_TR_POLY 2
#define PVR_LIST_TR_MOD  3
#define PVR_LIST_PT_POLY 4
#endif

#include <cmath>
#include <cfloat>

namespace smlt {

/* ========================================================================
 * PVR Vertex type: 64-byte floating color format (Type 5)
 * Layout: flags, x, y, z, u, v, (padding), base_a/r/g/b, offset_a/r/g/b
 * ======================================================================== */
#ifdef __DREAMCAST__
typedef struct {
    uint32_t flags;     /* TA command (vertex flags) */
    float x, y, z;      /* Screen coordinates (x, y) and 1/w depth (z) */
    float u, v;         /* Texture coordinates */
    uint32_t _pad0;     /* Padding to align to 32 bytes */
    uint32_t _pad1;     /* Padding */
    float base_a;       /* Base color alpha (0.0-1.0) */
    float base_r;       /* Base color red */
    float base_g;       /* Base color green */
    float base_b;       /* Base color blue */
    float offset_a;     /* Offset color alpha */
    float offset_r;     /* Offset color red */
    float offset_g;     /* Offset color green */
    float offset_b;     /* Offset color blue */
} __attribute__((aligned(32))) pvr_vertex_type5_t;

/* Texture size (power-of-2, 8..1024) -> PVR size index 0..7 */
static inline uint32_t pvr_txr_size_idx(int sz) {
    return (uint32_t)(__builtin_ctz((unsigned)sz) - 3);
}

/* Build pvr_poly_hdr_t directly without going through pvr_poly_cxt_t.
 *
 * Fixed for all our draw calls:
 *   color format  = PVR_CLRFMT_4FLOATS
 *   UV format     = PVR_UVFMT_32BIT  (= 0, contributes nothing)
 *   color clamp   = enabled
 *   txr_alpha     = PVR_TXRALPHA_ENABLE (= 0, contributes nothing)
 *   uv_flip/clamp = none  (= 0)
 *   mipmap_bias   = PVR_MIPBIAS_NORMAL (= 4)
 *   mipmap        = disabled (= 0)
 *   no modifier volumes, no user-clip, no specular */
static inline void pvr_build_poly_hdr(
    pvr_poly_hdr_t* hdr,
    int list_type,
    int shade_mode,
    int depth_func,
    int depth_write,
    int cull_mode,
    int blend_src,
    int blend_dst,
    int fog_type,
    const PVRTextureObject* tex_obj)
{
    const int textured = (tex_obj != nullptr) ? 1 : 0;
    /* alpha enabled for translucent and punch-through lists */
    const int alpha = (list_type != PVR_LIST_OP_POLY) ? 1 : 0;

    /* CMD: base | texture-enable (bit 3) | list type (26:24)
     *          | color format (6:4) | shade mode (bit 1) */
    hdr->cmd = PVR_CMD_POLYHDR
             | ((uint32_t)textured   << 3)
             | ((uint32_t)list_type  << PVR_TA_CMD_TYPE_SHIFT)
             | (PVR_CLRFMT_4FLOATS   << PVR_TA_CMD_CLRFMT_SHIFT)
             | ((uint32_t)shade_mode << PVR_TA_CMD_SHADE_SHIFT);

    /* mode1: depth compare (31:29) | cull (28:27)
     *        | depth write (26) | texture enable (25) */
    hdr->mode1 = ((uint32_t)depth_func  << PVR_TA_PM1_DEPTHCMP_SHIFT)
               | ((uint32_t)cull_mode   << PVR_TA_PM1_CULLING_SHIFT)
               | ((uint32_t)depth_write << PVR_TA_PM1_DEPTHWRITE_SHIFT)
               | ((uint32_t)textured    << PVR_TA_PM1_TXRENABLE_SHIFT);

    /* mode2: src blend (31:29) | dst blend (28:26) | fog (23:22)
     *        | color clamp (21) | alpha (20) */
    hdr->mode2 = ((uint32_t)blend_src  << PVR_TA_PM2_SRCBLEND_SHIFT)
               | ((uint32_t)blend_dst  << PVR_TA_PM2_DSTBLEND_SHIFT)
               | ((uint32_t)fog_type   << PVR_TA_PM2_FOG_SHIFT)
               | (PVR_CLRCLAMP_ENABLE  << PVR_TA_PM2_CLAMP_SHIFT)
               | ((uint32_t)alpha      << PVR_TA_PM2_ALPHA_SHIFT);

    hdr->mode3 = 0;

    if(textured) {
        /* env: MODULATEALPHA for alpha lists, MODULATE for opaque */
        const int env    = alpha ? PVR_TXRENV_MODULATEALPHA : PVR_TXRENV_MODULATE;
        const int filter = (tex_obj->filter == TEXTURE_FILTER_BILINEAR)
                         ? PVR_FILTER_BILINEAR : PVR_FILTER_NONE;
        const uint32_t u        = pvr_txr_size_idx(tex_obj->width);
        const uint32_t v        = pvr_txr_size_idx(tex_obj->height);
        const uint32_t txr_base = ((uint32_t)tex_obj->texture_vram & 0x00fffff8u) >> 3;

        hdr->mode2 |= (PVR_MIPBIAS_NORMAL  << PVR_TA_PM2_MIPBIAS_SHIFT)
                    | ((uint32_t)env        << PVR_TA_PM2_TXRENV_SHIFT)
                    | (u                    << PVR_TA_PM2_USIZE_SHIFT)
                    | (v                    << PVR_TA_PM2_VSIZE_SHIFT)
                    | ((uint32_t)filter     << __builtin_ctz(PVR_TA_PM2_FILTER));

        /* mode3: format bits are pre-encoded; OR in the VRAM address >> 3 */
        hdr->mode3 = (uint32_t)tex_obj->format | txr_base;
    }
    /* bytes 16-31 are unused for non-modifier polygon headers;
     * they were zeroed at construction and we leave them as-is */
}
#endif

/* ========================================================================
 * Constructor
 * ======================================================================== */

PVRRenderQueueVisitor::PVRRenderQueueVisitor(PVRRenderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera) {
#ifdef __DREAMCAST__
    memset(&poly_hdr_, 0, sizeof(poly_hdr_));
#endif
}

/* ========================================================================
 * Traversal start/end - manage scene and list lifecycle
 * ======================================================================== */

void PVRRenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue,
                                             uint64_t frame_id,
                                             StageNode* stage_node) {
    _S_UNUSED(queue);
    _S_UNUSED(frame_id);
    _S_UNUSED(stage_node);

    /* Get ambient light from the stage if available */
    if(stage_node) {
        auto a = stage_node->scene->lighting->ambient_light();
        ambient_[0] = a.r;
        ambient_[1] = a.g;
        ambient_[2] = a.b;
    }
}

void PVRRenderQueueVisitor::end_traversal(const batcher::RenderQueue& queue,
                                           StageNode* stage_node) {
    _S_UNUSED(queue);
    _S_UNUSED(stage_node);
}

/* ========================================================================
 * Render group / material pass changes
 * ======================================================================== */

void PVRRenderQueueVisitor::change_render_group(const batcher::RenderGroup* prev,
                                                 const batcher::RenderGroup* next) {
    _S_UNUSED(prev);
    _S_UNUSED(next);
}

void PVRRenderQueueVisitor::change_material_pass(const MaterialPass* prev,
                                                  const MaterialPass* next) {
    pass_ = next;
    if(!next) return;

    /* Nothing changed — poly_hdr_ and material properties are still valid */
    if(prev == next) return;

    /* Store PBR material properties directly */
    const Color& bc = next->base_color();
    mat_base_color_[0] = bc.r;
    mat_base_color_[1] = bc.g;
    mat_base_color_[2] = bc.b;
    mat_base_color_[3] = bc.a;
    mat_metallic_  = next->metallic();
    mat_roughness_ = next->roughness();

    /* Determine PVR list type based on blend mode, then redirect to the
     * matching modifier-volume list if this pass targets it. Punch-through has
     * no modifier list, so it falls back to the opaque modifier list. */
    auto blend = next->blend_func();
    const bool to_modifier =
        (next->polygon_list_target() == POLYGON_LIST_TARGET_MODIFIER);
    emitting_modifier_volume_ = to_modifier;

    if(blend == BLEND_NONE) {
        renderer_->current_list_type_ =
            to_modifier ? PVR_LIST_OP_MOD : PVR_LIST_OP_POLY;
    } else if(blend == BLEND_MASK) {
        renderer_->current_list_type_ =
            to_modifier ? PVR_LIST_OP_MOD : PVR_LIST_PT_POLY;
    } else {
        renderer_->current_list_type_ =
            to_modifier ? PVR_LIST_TR_MOD : PVR_LIST_TR_POLY;
    }

#ifdef __DREAMCAST__
    /* Map blend modes to PVR blend factors */
    int blend_src, blend_dst;
    switch(blend) {
        case BLEND_NONE:
            blend_src = PVR_BLEND_ONE;
            blend_dst = PVR_BLEND_ZERO;
            break;
        case BLEND_ADD:
            blend_src = PVR_BLEND_SRCALPHA;
            blend_dst = PVR_BLEND_ONE;
            break;
        case BLEND_ALPHA:
            blend_src = PVR_BLEND_SRCALPHA;
            blend_dst = PVR_BLEND_INVSRCALPHA;
            break;
        case BLEND_COLOR:
        case BLEND_MODULATE:
            blend_src = PVR_BLEND_DESTCOLOR;
            blend_dst = PVR_BLEND_ZERO;
            break;
        case BLEND_ONE_ONE_MINUS_ALPHA:
            blend_src = PVR_BLEND_ONE;
            blend_dst = PVR_BLEND_INVSRCALPHA;
            break;
        default:
            blend_src = PVR_BLEND_ONE;
            blend_dst = PVR_BLEND_ZERO;
            break;
    }

    /* Map depth function - INVERTED because PVR uses 1/w for depth
     * where larger values are closer, opposite to OpenGL Z convention */
    int depth_func;
    switch(next->depth_func()) {
        case DEPTH_FUNC_NEVER:   depth_func = PVR_DEPTHCMP_NEVER;   break;
        case DEPTH_FUNC_LESS:    depth_func = PVR_DEPTHCMP_GREATER;  break;
        case DEPTH_FUNC_LEQUAL:  depth_func = PVR_DEPTHCMP_GEQUAL;  break;
        case DEPTH_FUNC_EQUAL:   depth_func = PVR_DEPTHCMP_EQUAL;   break;
        case DEPTH_FUNC_GEQUAL:  depth_func = PVR_DEPTHCMP_LEQUAL;  break;
        case DEPTH_FUNC_GREATER: depth_func = PVR_DEPTHCMP_LESS;    break;
        case DEPTH_FUNC_ALWAYS:  depth_func = PVR_DEPTHCMP_ALWAYS;  break;
        default:                 depth_func = PVR_DEPTHCMP_GEQUAL;  break;
    }

    /* Map cull mode */
    int cull_mode;
    switch(next->cull_mode()) {
        case CULL_MODE_NONE:               cull_mode = PVR_CULLING_NONE;  break;
        case CULL_MODE_BACK_FACE:          cull_mode = PVR_CULLING_CW;    break;
        case CULL_MODE_FRONT_FACE:         cull_mode = PVR_CULLING_CCW;   break;
        case CULL_MODE_FRONT_AND_BACK_FACE:cull_mode = PVR_CULLING_SMALL; break;
        default:                           cull_mode = PVR_CULLING_CW;    break;
    }

    const int shade_mode = (next->shade_model() == SHADE_MODEL_FLAT)
                         ? PVR_SHADE_FLAT : PVR_SHADE_GOURAUD;

    int fog_type;
    switch(next->fog_mode()) {
        case FOG_MODE_LINEAR:
        case FOG_MODE_EXP:
        case FOG_MODE_EXP2: fog_type = PVR_FOG_TABLE;   break;
        default:            fog_type = PVR_FOG_DISABLE;  break;
    }

    /* Resolve texture — bind_texture uploads if needed and returns VRAM object */
    PVRTextureObject* tex_obj = nullptr;
    if((next->textures_enabled() & BASE_COLOR_MAP_ENABLED) != 0) {
        auto tex = next->base_color_map();
        if(tex) {
            tex_obj = renderer_->texture_manager().bind_texture(tex->_renderer_specific_id());
            if(tex_obj && !tex_obj->texture_vram)
                tex_obj = nullptr;  /* upload failed or not ready */
        }
    }

    if(emitting_modifier_volume_) {
        /* Modifier-volume passes use a completely different header format
         * (pvr_mod_hdr_t). Compile both variants up-front: do_visit emits
         * mod_hdr_other_ before all but the last triangle of the volume, and
         * mod_hdr_include_ before the last one (which is what marks an
         * inclusion modifier volume — pixels inside get darkened). */
        pvr_mod_compile(
            &mod_hdr_other_,
            renderer_->current_list_type_,
            PVR_MODIFIER_OTHER_POLY,
            PVR_CULLING_NONE
        );
        pvr_mod_compile(
            &mod_hdr_include_,
            renderer_->current_list_type_,
            PVR_MODIFIER_INCLUDE_LAST_POLY,
            PVR_CULLING_NONE
        );
    } else {
        pvr_build_poly_hdr(
            &poly_hdr_,
            renderer_->current_list_type_,
            shade_mode,
            next->is_depth_test_enabled() ? depth_func : PVR_DEPTHCMP_ALWAYS,
            next->is_depth_write_enabled() ? PVR_DEPTHWRITE_ENABLE : PVR_DEPTHWRITE_DISABLE,
            cull_mode,
            blend_src,
            blend_dst,
            fog_type,
            tex_obj
        );
    }
#endif
}

void PVRRenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {
    for(int i = 0; i < MAX_LIGHTS; i++) {
        lights_[i].enabled = false;
    }

    for(uint8_t i = 0; i < count && i < MAX_LIGHTS; i++) {
        if(!lights[i]) continue;

        VertexLightState& state = lights_[i];
        state.enabled = true;

        auto light = lights[i];
        const float w = (light->light_type() == smlt::LIGHT_TYPE_DIRECTIONAL) ? 0.0f : 1.0f;
        auto lp = light->transform->position();
        /* Use w=1 for point lights so the view translation is included;
         * w=0 for directional lights treats it as a direction vector. */
        auto pos4 = camera_->view_matrix() * smlt::Vec4(lp.x, lp.y, lp.z, w);
        state.position[0] = pos4.x;
        state.position[1] = pos4.y;
        state.position[2] = pos4.z;
        state.position[3] = w;

        state.color[0] = light->color().r;
        state.color[1] = light->color().g;
        state.color[2] = light->color().b;

        state.intensity = light->intensity();
        state.range = light->range();

        /* Pre-normalised toward-light direction for directional lights.
         * position already stores -pointing_dir (set_direction negates on write),
         * so pos4.xyz = view_rot * (-pointing_dir) = toward_light in eye space.
         * No further negation needed. */
        if(state.position[3] < 0.5f) {
#ifdef __DREAMCAST__
            shz_vec3_t d = shz_vec3_normalize_safe(
                shz_vec3_init(pos4.x, pos4.y, pos4.z));
            state.dir[0] = d.x; state.dir[1] = d.y; state.dir[2] = d.z;
#else
            float len = std::sqrt(pos4.x*pos4.x + pos4.y*pos4.y + pos4.z*pos4.z);
            if(len > 1e-8f) {
                state.dir[0] = pos4.x/len;
                state.dir[1] = pos4.y/len;
                state.dir[2] = pos4.z/len;
            }
#endif
        }
    }
}


/* ========================================================================
 * Near-plane clipping support
 * ======================================================================== */

#ifdef __DREAMCAST__

/* A vertex in clip space with all attributes needed for interpolation */
struct ClipVertex {
    float x, y, z, w;  /* Clip-space position */
    float u, v;         /* Texture coordinates */
    float r, g, b, a;   /* Color */
};

/* Check if a vertex is in front of the near plane (visible).
 * In clip space, the near plane is at z = -w for the standard projection. */
static inline bool is_vertex_visible(const ClipVertex& v) {
    return v.z >= -v.w;
}

/* Interpolate between two vertices at the near plane intersection.
 * Returns the t value (0..1) along the edge from v1 to v2 where it
 * intersects the near plane (z = -w). */
static inline float clip_edge_t(const ClipVertex& v1, const ClipVertex& v2) {
    /* Near plane equation: z + w = 0, so z = -w
     * We need to find t where: v1.z + t*(v2.z - v1.z) = -(v1.w + t*(v2.w - v1.w))
     * Rearranging: v1.z + v1.w + t*(v2.z - v1.z + v2.w - v1.w) = 0
     * t = -(v1.z + v1.w) / ((v2.z - v1.z) + (v2.w - v1.w))
     */
    float d1 = v1.z + v1.w;  /* Distance from v1 to near plane (negative = behind) */
    float d2 = v2.z + v2.w;  /* Distance from v2 to near plane */
    float denom = d2 - d1;
    if(fabsf(denom) < 1e-7f) {
        return 0.5f;  /* Parallel to plane, shouldn't happen but handle gracefully */
    }
    float t = -d1 / denom;
    /* Clamp to valid range */
    if(t < 0.0f) t = 0.0f;
    if(t > 1.0f) t = 1.0f;
    return t;
}

/* Each shz_lerpf compiles to a single FMAC instruction on SH4. */
static inline ClipVertex lerp_vertex(const ClipVertex& v1, const ClipVertex& v2, float t) {
    ClipVertex out;
    out.x = shz_lerpf(v1.x, v2.x, t);
    out.y = shz_lerpf(v1.y, v2.y, t);
    out.z = shz_lerpf(v1.z, v2.z, t);
    out.w = shz_lerpf(v1.w, v2.w, t);
    out.u = shz_lerpf(v1.u, v2.u, t);
    out.v = shz_lerpf(v1.v, v2.v, t);
    out.r = shz_lerpf(v1.r, v2.r, t);
    out.g = shz_lerpf(v1.g, v2.g, t);
    out.b = shz_lerpf(v1.b, v2.b, t);
    out.a = shz_lerpf(v1.a, v2.a, t);
    return out;
}

#endif

/* ========================================================================
 * visit - main draw call
 * ======================================================================== */

void PVRRenderQueueVisitor::visit(const Renderable* renderable,
                                   const MaterialPass* pass,
                                   batcher::Iteration iteration) {
    /* Invisible renderables are still in the queue so consumers like
     * ShadowCaster can read them back to generate shadow geometry from a
     * hidden low-poly proxy. Skip the actual draw here. */
    if(renderable && !renderable->is_visible) {
        return;
    }
    /* The PVR has no stencil hardware; stencil-enabled passes exist only for
     * the GL stencil-shadow-volume technique. Skip them here. */
    if(pass && pass->is_stencil_test_enabled()) {
        return;
    }
    do_visit(renderable, pass, iteration);
}

void PVRRenderQueueVisitor::do_visit(const Renderable* renderable,
                                      const MaterialPass* material_pass,
                                      batcher::Iteration iteration) {
    _S_UNUSED(iteration);

    if(!renderable || !material_pass) return;

    renderer_->prepare_to_render(renderable);

#ifdef __DREAMCAST__
    /* ================================================================
     * Modifier-volume submission path
     *
     * When the active material pass targets the modifier list, the geometry
     * is submitted as pvr_modifier_vol_t triangles (each carrying its three
     * world-space-screen-coords) preceded by a pvr_mod_hdr_t. The pattern is:
     *   [OTHER hdr][vol_0]...[vol_{N-2}][INCLUDE_LAST hdr][vol_{N-1}]
     * which makes the whole batch one inclusion volume (pixels inside are
     * affected — what cheap-shadow uses to darken receivers).
     * ================================================================ */
    if(emitting_modifier_volume_) {
        const auto* vdata = renderable->vertex_data;
        const auto* idata = renderable->index_data;
        if(!vdata || !idata) return;
        if(renderable->arrangement != MESH_ARRANGEMENT_TRIANGLES) {
            /* TODO: support strip/fan; for now only TRIANGLES (what
             * ShadowCaster emits). */
            return;
        }

        std::size_t index_count = renderable->index_element_count;
        if(index_count == 0) index_count = idata->count();
        const std::size_t tri_count = index_count / 3;
        if(tri_count == 0) return;

        /* MVP for transforming the volume vertices to clip space. */
        const auto& model = *renderable->final_transformation;
        const auto& view = camera_->view_matrix();
        const auto& projection = camera_->projection_matrix();
        Mat4 mvp = projection * (view * model);

        const float hw = 320.0f;
        const float hh = 240.0f;

        const auto& spec = vdata->vertex_specification();
        const auto stride = vdata->stride();
        const auto pos_offset = spec.position_offset(false);
        const uint8_t* raw_data = vdata->data();

        auto& buf = renderer_->buffer(renderer_->current_list_type_)
                        .buffers[renderer_->current_buffer_index_];

        shz_xmtrx_load_4x4(mvp.native());

        /* Position-only clip-space vertex. */
        struct ModVtx { float x, y, z, w; };

        auto load_clip = [&](uint32_t vi) -> ModVtx {
            const float* p = (const float*)(raw_data + stride * vi + pos_offset);
            shz_vec4_t c = shz_xmtrx_transform_vec4(
                shz_vec4_init(p[0], p[1], p[2], 1.0f));
            return {c.x, c.y, c.z, c.w};
        };

        /* Linear interpolation in clip space. */
        auto lerp_clip = [](const ModVtx& a, const ModVtx& b, float t) -> ModVtx {
            return {
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t,
            };
        };

        /* Find the t (0..1) along edge a→b where it crosses the near plane
         * (z + w = 0 in clip space). Callers only invoke this when the edge
         * actually crosses, so the denominator is non-zero. */
        auto near_clip_t = [](const ModVtx& a, const ModVtx& b) -> float {
            float da = a.z + a.w;
            float db = b.z + b.w;
            float denom = db - da;
            if(fabsf(denom) < 1e-7f) return 0.5f;
            float t = -da / denom;
            if(t < 0.0f) t = 0.0f;
            if(t > 1.0f) t = 1.0f;
            return t;
        };

        /* Perspective divide + viewport transform → screen-space + 1/w. */
        auto to_screen = [&](const ModVtx& v, float& sx, float& sy, float& sz) {
            float w = v.w;
            if(w < FLT_EPSILON) w = FLT_EPSILON; /* defensive; post-clip should be > 0 */
            float inv_w = shz_invf(w);
            sx = (v.x * hw + hw * w) * inv_w;
            sy = (-v.y * hh + hh * w) * inv_w;
            sz = inv_w;
        };

        auto append_hdr = [&](const pvr_mod_hdr_t& hdr) {
            std::size_t pos = buf.size();
            buf.resize(pos + sizeof(pvr_mod_hdr_t));
            shz_memcpy32(&buf[pos], &hdr, sizeof(pvr_mod_hdr_t));
        };

        /* Build a pvr_modifier_vol_t from three clip-space vertices and
         * stage it through the deferred emission mechanism below. Near-plane
         * clipping may produce a variable number of output triangles per
         * input triangle (0, 1, or 2), so we don't know which one is "last"
         * until we run out of input — emit each pending triangle as OTHER
         * once we see another after it.
         *
         * The actual InsideLastPolygon OP is a SEPARATE synthetic triangle
         * appended after staging (see the closure block below). Tagging one
         * of the volume's own triangles wouldn't work: on the PVR the
         * InsideLast OP is only registered in its bbox tiles, but the
         * volume's parity is set by every side quad / cap triangle, so any
         * tile reached by the volume but missed by the chosen closer would
         * leak orphan STENCIL_VOLPAR into AREA1 (and contaminate the next
         * volume in the list). */
        float sb_min_x = FLT_MAX, sb_min_y = FLT_MAX;
        float sb_max_x = -FLT_MAX, sb_max_y = -FLT_MAX;

        bool has_prev = false;
        bool emitted_other_hdr = false;
        alignas(32) pvr_modifier_vol_t prev_vol;

        auto stage_triangle = [&](const ModVtx& a, const ModVtx& b, const ModVtx& c) {
            alignas(32) pvr_modifier_vol_t vol;
            vol.flags = PVR_CMD_VERTEX_EOL;
            to_screen(a, vol.ax, vol.ay, vol.az);
            to_screen(b, vol.bx, vol.by, vol.bz);
            to_screen(c, vol.cx, vol.cy, vol.cz);
            vol.d1 = vol.d2 = vol.d3 = vol.d4 = vol.d5 = vol.d6 = 0;

            /* Expand the screen-space bbox covering every tile the synthetic
             * closer must reach. */
            if(vol.ax < sb_min_x) sb_min_x = vol.ax;
            if(vol.bx < sb_min_x) sb_min_x = vol.bx;
            if(vol.cx < sb_min_x) sb_min_x = vol.cx;
            if(vol.ax > sb_max_x) sb_max_x = vol.ax;
            if(vol.bx > sb_max_x) sb_max_x = vol.bx;
            if(vol.cx > sb_max_x) sb_max_x = vol.cx;
            if(vol.ay < sb_min_y) sb_min_y = vol.ay;
            if(vol.by < sb_min_y) sb_min_y = vol.by;
            if(vol.cy < sb_min_y) sb_min_y = vol.cy;
            if(vol.ay > sb_max_y) sb_max_y = vol.ay;
            if(vol.by > sb_max_y) sb_max_y = vol.by;
            if(vol.cy > sb_max_y) sb_max_y = vol.cy;

            if(has_prev) {
                /* The previously-staged triangle is now known not to be last:
                 * emit it as OTHER. */
                if(!emitted_other_hdr) {
                    append_hdr(mod_hdr_other_);
                    emitted_other_hdr = true;
                }
                std::size_t pos = buf.size();
                buf.resize(pos + sizeof(pvr_modifier_vol_t));
                shz_memcpy32(&buf[pos], &prev_vol, sizeof(pvr_modifier_vol_t));
            }
            prev_vol = vol;
            has_prev = true;
        };

        for(std::size_t t = 0; t < tri_count; ++t) {
            const std::size_t base = renderable->first_index + t * 3;
            const ModVtx v0 = load_clip(idata->at((uint32_t)(base + 0)));
            const ModVtx v1 = load_clip(idata->at((uint32_t)(base + 1)));
            const ModVtx v2 = load_clip(idata->at((uint32_t)(base + 2)));

            const bool vis0 = (v0.z >= -v0.w);
            const bool vis1 = (v1.z >= -v1.w);
            const bool vis2 = (v2.z >= -v2.w);
            const int mask = (vis0 ? 1 : 0) | (vis1 ? 2 : 0) | (vis2 ? 4 : 0);

            switch(mask) {
                case 0: /* All behind the near plane: discard. */
                    break;
                case 7: /* All in front: emit as-is. */
                    stage_triangle(v0, v1, v2);
                    break;
                case 1: { /* Only v0 in front. */
                    ModVtx a = lerp_clip(v0, v1, near_clip_t(v0, v1));
                    ModVtx b = lerp_clip(v0, v2, near_clip_t(v0, v2));
                    stage_triangle(v0, a, b);
                    break;
                }
                case 2: { /* Only v1 in front. */
                    ModVtx a = lerp_clip(v1, v0, near_clip_t(v1, v0));
                    ModVtx b = lerp_clip(v1, v2, near_clip_t(v1, v2));
                    stage_triangle(a, v1, b);
                    break;
                }
                case 4: { /* Only v2 in front. */
                    ModVtx a = lerp_clip(v2, v1, near_clip_t(v2, v1));
                    ModVtx b = lerp_clip(v2, v0, near_clip_t(v2, v0));
                    stage_triangle(a, v2, b);
                    break;
                }
                case 3: { /* v0, v1 in front; v2 behind — produces a quad. */
                    ModVtx a = lerp_clip(v1, v2, near_clip_t(v1, v2));
                    ModVtx b = lerp_clip(v0, v2, near_clip_t(v0, v2));
                    stage_triangle(v0, v1, a);
                    stage_triangle(v0, a, b);
                    break;
                }
                case 5: { /* v0, v2 in front; v1 behind — produces a quad. */
                    ModVtx a = lerp_clip(v0, v1, near_clip_t(v0, v1));
                    ModVtx b = lerp_clip(v2, v1, near_clip_t(v2, v1));
                    stage_triangle(v0, a, b);
                    stage_triangle(v0, b, v2);
                    break;
                }
                case 6: { /* v1, v2 in front; v0 behind — produces a quad. */
                    ModVtx a = lerp_clip(v1, v0, near_clip_t(v1, v0));
                    ModVtx b = lerp_clip(v2, v0, near_clip_t(v2, v0));
                    stage_triangle(a, v1, v2);
                    stage_triangle(a, v2, b);
                    break;
                }
            }
        }

        /* Close the volume.
         *
         * Flush the still-pending staged triangle as OTHER (it stayed in
         * prev_vol so we could potentially have used it as the closer; we
         * deliberately don't, see the bbox comment above). Then append a
         * synthetic closure triangle whose 3 vertices span the bbox of every
         * staged triangle, so its OP is registered in every tile the volume
         * touches and combine_modifier_volume fires there.
         *
         * The closer's z is fixed at a tiny positive value, well below KOS's
         * default ISP_BACKGND_D (0.0001f) and any opaque polygon's stored
         * 1/W. rasterize_modifier_triangle only flips STENCIL_VOLPAR where
         * z >= depth_buf, so this triangle never alters parity — it exists
         * purely to drive the per-tile fold. If clipping discarded every
         * input triangle (has_prev == false), the volume is empty and we
         * emit nothing. */
        if(has_prev) {
            if(!emitted_other_hdr) {
                append_hdr(mod_hdr_other_);
                emitted_other_hdr = true;
            }
            {
                std::size_t pos = buf.size();
                buf.resize(pos + sizeof(pvr_modifier_vol_t));
                shz_memcpy32(&buf[pos], &prev_vol, sizeof(pvr_modifier_vol_t));
            }

            alignas(32) pvr_modifier_vol_t closer;
            closer.flags = PVR_CMD_VERTEX_EOL;
            const float close_z = 1.0e-10f;
            closer.ax = sb_min_x; closer.ay = sb_min_y; closer.az = close_z;
            closer.bx = sb_max_x; closer.by = sb_min_y; closer.bz = close_z;
            closer.cx = sb_min_x; closer.cy = sb_max_y; closer.cz = close_z;
            closer.d1 = closer.d2 = closer.d3 = closer.d4 = closer.d5 = closer.d6 = 0;

            append_hdr(mod_hdr_include_);
            {
                std::size_t pos = buf.size();
                buf.resize(pos + sizeof(pvr_modifier_vol_t));
                shz_memcpy32(&buf[pos], &closer, sizeof(pvr_modifier_vol_t));
            }
        }

        return;
    }

    const auto& model = *renderable->final_transformation;
    const auto& view = camera_->view_matrix();
    const auto& projection = camera_->projection_matrix();

    /* Build the modelview-projection matrix */
    Mat4 modelview = view * model;
    Mat4 mvp = projection * modelview;

    /* Build viewport transform matrix */
    float hw = 320.0f; /* Half-width */
    float hh = 240.0f; /* Half-height */

    /* ================================================================
     * Submit or buffer the pre-compiled polygon header.
     *
     * If this renderable receives shadows (and is rendering as a normal
     * polygon, not a modifier volume), patch the header's modifier-enable
     * bit (PVR_TA_CMD_MODIFIER = bit 7) so cheap-shadow modifier volumes
     * darken its pixels. Cheap-shadow mode is selected by leaving
     * PVR_TA_CMD_MODIFIERMODE (bit 6) clear, which pvr_build_poly_hdr
     * already does. KOS keeps the header at 32 bytes in this mode.
     * ================================================================ */
    const bool patch_receiver =
        (renderable->flags & RENDERABLE_FLAG_RECEIVES_SHADOWS) &&
        (material_pass->polygon_list_target() == POLYGON_LIST_TARGET_NONE);

    pvr_poly_hdr_t hdr_local;
    const pvr_poly_hdr_t* hdr_src = &poly_hdr_;
    if(patch_receiver) {
        hdr_local = poly_hdr_;
        hdr_local.cmd |= (1u << 7); /* PVR_TA_CMD_MODIFIER */
        hdr_src = &hdr_local;
    }

    if(renderer_->current_list_type_ == PVR_LIST_OP_POLY) {
        pvr_vertex_t* hdr_dest = static_cast<pvr_vertex_t*>(pvr_dr_target(renderer_->dr_state_));
        shz_memcpy32(hdr_dest, hdr_src, sizeof(pvr_poly_hdr_t));
        pvr_dr_commit(hdr_dest);
    } else {
        auto& buf = renderer_->buffer(renderer_->current_list_type_).buffers[renderer_->current_buffer_index_];
        const uint8_t* hdr_bytes = reinterpret_cast<const uint8_t*>(hdr_src);
        auto size = buf.size();
        buf.resize(size + sizeof(pvr_poly_hdr_t));
        shz_memcpy32(&buf[size], hdr_bytes, sizeof(pvr_poly_hdr_t));
    }

    /* ================================================================
     * Read vertex data and transform
     * ================================================================ */
    const auto* vdata = renderable->vertex_data;
    if(!vdata) return;

    const auto& spec = vdata->vertex_specification();
    const auto stride = vdata->stride();
    const uint8_t* raw_data = vdata->data();

    auto pos_offset = spec.position_offset(false);
    auto uv_offset = spec.texcoord0_offset(false);
    auto color_offset = spec.color_offset(false);
    auto normal_offset = spec.normal_offset(false);

    auto color_mat_mode = material_pass->color_material();
    bool lighting_enabled = material_pass->is_lighting_enabled() && normal_offset;

    /* ================================================================
     * Three-pass vertex transformation
     * Each pass below uses a narrow working set, so the SH4 register file
     * is largely sufficient.  Pass 1 only needs the loaded MVP in xmtrx
     * and the FTRV inputs; pass 2 does no floating-point work besides a
     * handful of multiplies; pass 3 uses xmtrx for modelview transforms
     * and computes lighting against a fixed-size light table.
     * ================================================================ */
    static aligned_vector<ClipVertex, 32> work_vertices_;

    /* Transform a contiguous batch of `count` vertices starting at source
     * vertex `base`.  On entry MVP must be loaded into xmtrx.  On exit:
     *   - work_vertices_[i] corresponds to source vertex (base + i)
     *   - x,y,z,w = clip-space position
     *   - u,v     = texture coordinates
     *   - r,g,b   = lit colour (lighting on) or base colour (lighting off)
     *   - a       = final alpha
     * If lighting was applied xmtrx is restored to MVP before return. */
    auto transform_batch = [&](uint32_t base, uint32_t count) {
        if(!count) return;
        work_vertices_.resize(count);

        /* ------------------------------------------------------------
         * Pass 1: position × MVP via FTRV.
         * ------------------------------------------------------------ */
        {
            const uint8_t* row = raw_data + stride * base;
            for(uint32_t i = 0; i < count; ++i) {
                const float* p = (const float*)(row + pos_offset);
                shz_vec4_t clip = shz_xmtrx_transform_vec4(
                    shz_vec4_init(p[0], p[1], p[2], 1.0f));
                ClipVertex& cv = work_vertices_[i];
                cv.x = clip.x; cv.y = clip.y; cv.z = clip.z; cv.w = clip.w;
                row += stride;
            }
        }

        /* ------------------------------------------------------------
         * Pass 2: UVs and base colour (material × per-vertex colour).
         * Stores the *base* colour in cv.r/g/b — pass 3 reads it back
         * out and replaces it with the lit colour if lighting is on.
         * ------------------------------------------------------------ */
        {
            const VertexAttribute color_attr = spec.color_attribute;
            const uint8_t* row = raw_data + stride * base;
            for(uint32_t i = 0; i < count; ++i) {
                ClipVertex& cv = work_vertices_[i];

                if(uv_offset) {
                    const float* t = (const float*)(row + uv_offset);
                    cv.u = t[0];
                    cv.v = t[1];
                } else {
                    cv.u = 0.0f; cv.v = 0.0f;
                }

                float br = mat_base_color_[0];
                float bg = mat_base_color_[1];
                float bb = mat_base_color_[2];
                float ba = mat_base_color_[3];

                if(color_offset) {
                    float vc_r = 1.0f, vc_g = 1.0f, vc_b = 1.0f, vc_a = 1.0f;
                    if(color_attr == VERTEX_ATTRIBUTE_4F) {
                        const float* c = (const float*)(row + color_offset);
                        vc_r = c[0]; vc_g = c[1]; vc_b = c[2]; vc_a = c[3];
                    } else if(color_attr == VERTEX_ATTRIBUTE_3F) {
                        const float* c = (const float*)(row + color_offset);
                        vc_r = c[0]; vc_g = c[1]; vc_b = c[2];
                    } else if(color_attr == VERTEX_ATTRIBUTE_4UB_RGBA ||
                              color_attr == VERTEX_ATTRIBUTE_4UB) {
                        const uint8_t* c = row + color_offset;
                        vc_r = c[0]/255.0f; vc_g = c[1]/255.0f;
                        vc_b = c[2]/255.0f; vc_a = c[3]/255.0f;
                    } else if(color_attr == VERTEX_ATTRIBUTE_4UB_BGRA) {
                        const uint8_t* c = row + color_offset;
                        vc_b = c[0]/255.0f; vc_g = c[1]/255.0f;
                        vc_r = c[2]/255.0f; vc_a = c[3]/255.0f;
                    }
                    switch(color_mat_mode) {
                        case COLOR_MATERIAL_DIFFUSE:
                        case COLOR_MATERIAL_AMBIENT_AND_DIFFUSE:
                            br = vc_r; bg = vc_g; bb = vc_b; ba = vc_a;
                            break;
                        default:
                            br *= vc_r; bg *= vc_g; bb *= vc_b; ba *= vc_a;
                            break;
                    }
                }

                cv.r = br;
                cv.g = bg;
                cv.b = bb;
                cv.a = ba;
                row += stride;
            }
        }

        /* ------------------------------------------------------------
         * Pass 3: per-vertex PBR lighting.
         * Loads modelview into xmtrx so the normal and eye-space
         * position transforms can also use FTRV.
         * ------------------------------------------------------------ */
        if(lighting_enabled) {
            shz_xmtrx_load_4x4(modelview.native());

            const float roughness_alpha   = mat_roughness_ * mat_roughness_;
            const float roughness_alphaSq = roughness_alpha * roughness_alpha;
            const float k                 = roughness_alpha * 0.5f;
            const float nm                = 1.0f - mat_metallic_;

            const uint8_t* row = raw_data + stride * base;
            for(uint32_t i = 0; i < count; ++i) {
                ClipVertex& cv = work_vertices_[i];

                /* Recover base colour stashed by pass 2. */
                const float br = cv.r;
                const float bg = cv.g;
                const float bb = cv.b;

                const float* p     = (const float*)(row + pos_offset);
                const float* n_ptr = (const float*)(row + normal_offset);

                /* Normal × modelview (w=0 drops translation). */
                shz_vec4_t nv = shz_xmtrx_transform_vec4(
                    shz_vec4_init(n_ptr[0], n_ptr[1], n_ptr[2], 0.0f));
                float Nx = nv.x, Ny = nv.y, Nz = nv.z;
                float n_sq = shz_mag_sqr3f(Nx, Ny, Nz);
                if(n_sq > 1e-8f) {
                    float invn = shz_inv_sqrtf_fsrra(n_sq);
                    Nx *= invn; Ny *= invn; Nz *= invn;
                }

                /* Position × modelview → eye-space. */
                shz_vec4_t ev = shz_xmtrx_transform_vec4(
                    shz_vec4_init(p[0], p[1], p[2], 1.0f));
                float pos_ex = ev.x, pos_ey = ev.y, pos_ez = ev.z;

                /* View direction (eye at origin in view space). */
                float Vx = -pos_ex, Vy = -pos_ey, Vz = -pos_ez;
                float v_mag_sq = shz_mag_sqr3f(Vx, Vy, Vz);
                if(v_mag_sq > 1e-8f) {
                    float invv = shz_inv_sqrtf_fsrra(v_mag_sq);
                    Vx *= invv; Vy *= invv; Vz *= invv;
                }
                float NdotV = Nx*Vx + Ny*Vy + Nz*Vz;
                if(NdotV < 0.0001f) NdotV = 0.0001f;

                const float F0_r = 0.04f + (br - 0.04f) * mat_metallic_;
                const float F0_g = 0.04f + (bg - 0.04f) * mat_metallic_;
                const float F0_b = 0.04f + (bb - 0.04f) * mat_metallic_;

                float total_r = br * ambient_[0];
                float total_g = bg * ambient_[1];
                float total_b = bb * ambient_[2];

                for(int li = 0; li < MAX_LIGHTS; ++li) {
                    if(!lights_[li].enabled) continue;

                    float Lx, Ly, Lz;
                    float att = 1.0f;

                    if(lights_[li].position[3] < 0.5f) {
                        Lx = lights_[li].dir[0];
                        Ly = lights_[li].dir[1];
                        Lz = lights_[li].dir[2];
                    } else {
                        Lx = lights_[li].position[0] - pos_ex;
                        Ly = lights_[li].position[1] - pos_ey;
                        Lz = lights_[li].position[2] - pos_ez;
                        float l_sq = shz_mag_sqr3f(Lx, Ly, Lz);
                        if(l_sq > 1e-8f) {
                            float inv = shz_inv_sqrtf_fsrra(l_sq);
                            float dist = l_sq * inv;
                            Lx *= inv; Ly *= inv; Lz *= inv;
                            att = 1.0f - dist / (lights_[li].range + 1e-8f);
                            if(att < 0.0f) att = 0.0f;
                        }
                    }

                    float NdotL = shz_dot6f(Nx, Ny, Nz, Lx, Ly, Lz);
                    if(NdotL <= 0.0f) continue;

                    float Hx = Lx + Vx, Hy = Ly + Vy, Hz = Lz + Vz;
                    float h_sq = shz_mag_sqr3f(Hx, Hy, Hz);
                    if(h_sq > 1e-8f) {
                        float inv = shz_inv_sqrtf_fsrra(h_sq);
                        Hx *= inv; Hy *= inv; Hz *= inv;
                    }
                    float NdotH = Nx*Hx + Ny*Hy + Nz*Hz;
                    if(NdotH < 0.0f) NdotH = 0.0f;
                    float HdotV = Hx*Vx + Hy*Vy + Hz*Vz;
                    if(HdotV < 0.0f) HdotV = 0.0f;

                    float omHdotV = 1.0f - HdotV;
                    float pow5 = omHdotV * omHdotV; pow5 *= pow5; pow5 *= omHdotV;
                    float Fr = F0_r + (1.0f - F0_r) * pow5;
                    float Fg = F0_g + (1.0f - F0_g) * pow5;
                    float Fb = F0_b + (1.0f - F0_b) * pow5;

                    float kD_r = (1.0f - Fr) * nm;
                    float kD_g = (1.0f - Fg) * nm;
                    float kD_b = (1.0f - Fb) * nm;

                    float d = NdotH * NdotH * (roughness_alphaSq - 1.0f) + 1.0f;
                    float D = roughness_alphaSq / (d * d + 1e-7f);

                    float GV = NdotV / (NdotV * (1.0f - k) + k);
                    float GL = NdotL / (NdotL * (1.0f - k) + k + 1e-7f);
                    float G  = GV * GL;

                    float denom = 4.0f * NdotV * NdotL;
                    if(denom < 0.0001f) denom = 0.0001f;
                    float spec = D * G / denom;

                    float scale = NdotL * lights_[li].intensity * att;
                    total_r += (kD_r * br + spec * Fr) * scale * lights_[li].color[0];
                    total_g += (kD_g * bg + spec * Fg) * scale * lights_[li].color[1];
                    total_b += (kD_b * bb + spec * Fb) * scale * lights_[li].color[2];
                }

                cv.r = total_r > 1.0f ? 1.0f : total_r;
                cv.g = total_g > 1.0f ? 1.0f : total_g;
                cv.b = total_b > 1.0f ? 1.0f : total_b;
                row += stride;
            }

            /* Restore MVP for any subsequent batch. */
            shz_xmtrx_load_4x4(mvp.native());
        }
    };

    /* MVP is loaded once here; transform_batch keeps it loaded across calls. */
    shz_xmtrx_load_4x4(mvp.native());

    /* Lambda to do perspective divide and emit a ClipVertex.
     * For OP: submits directly via store queues (64-byte Type 5 format).
     * For PT/TR: appends the vertex to the deferred buffer. */
    auto submit_clip_vertex = [&](const ClipVertex& cv, bool is_last) {
        /* Apply viewport transform (done before perspective divide for PVR) */
        float vx = cv.x * hw + hw * cv.w;
        float vy = -cv.y * hh + hh * cv.w;

        /* Perspective divide */
        float w = cv.w;
        if(w == 0.0f) w = FLT_EPSILON;
        float inv_w = shz_invf(w);

        float sx = vx * inv_w;
        float sy = vy * inv_w;
        float sz = inv_w;  /* PVR uses 1/w for depth */

        pvr_vertex_type5_t vert;
        vert.flags = is_last ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
        vert.x = sx;
        vert.y = sy;
        vert.z = sz;
        vert.u = cv.u;
        vert.v = cv.v;
        vert._pad0 = 0;
        vert._pad1 = 0;
        vert.base_a = cv.a;
        vert.base_r = cv.r;
        vert.base_g = cv.g;
        vert.base_b = cv.b;
        vert.offset_a = 0.0f;
        vert.offset_r = 0.0f;
        vert.offset_g = 0.0f;
        vert.offset_b = 0.0f;

        if(renderer_->current_list_type_ == PVR_LIST_OP_POLY) {
            /* Submit 64-byte Type 5 vertex via direct rendering (two 32-byte writes) */
            pvr_vertex_t* dest1 = static_cast<pvr_vertex_t*>(pvr_dr_target(renderer_->dr_state_));
            *((uint32_t*)dest1 + 0) = vert.flags;
            *((float*)dest1 + 1) = vert.x;
            *((float*)dest1 + 2) = vert.y;
            *((float*)dest1 + 3) = vert.z;
            *((float*)dest1 + 4) = vert.u;
            *((float*)dest1 + 5) = vert.v;
            *((uint32_t*)dest1 + 6) = 0;  /* padding */
            *((uint32_t*)dest1 + 7) = 0;  /* padding */
            pvr_dr_commit(dest1);

            pvr_vertex_t* dest2 = static_cast<pvr_vertex_t*>(pvr_dr_target(renderer_->dr_state_));
            *((float*)dest2 + 0) = vert.base_a;
            *((float*)dest2 + 1) = vert.base_r;
            *((float*)dest2 + 2) = vert.base_g;
            *((float*)dest2 + 3) = vert.base_b;
            *((float*)dest2 + 4) = vert.offset_a;
            *((float*)dest2 + 5) = vert.offset_r;
            *((float*)dest2 + 6) = vert.offset_g;
            *((float*)dest2 + 7) = vert.offset_b;
            pvr_dr_commit(dest2);
        } else {
            auto& buf = renderer_->buffer(renderer_->current_list_type_).buffers[renderer_->current_buffer_index_];
            const uint8_t* vert_bytes = reinterpret_cast<const uint8_t*>(&vert);
            buf.insert(buf.end(), vert_bytes, vert_bytes + sizeof(pvr_vertex_type5_t));
        }
    };

    /* Lambda to process a triangle with near-plane clipping */
    auto process_triangle = [&](const ClipVertex& v0, const ClipVertex& v1,
                                const ClipVertex& v2, bool is_last_tri) {
        /* Check visibility of each vertex (z >= -w means in front of near plane) */
        bool vis0 = is_vertex_visible(v0);
        bool vis1 = is_vertex_visible(v1);
        bool vis2 = is_vertex_visible(v2);
        int visible_mask = (vis0 ? 1 : 0) | (vis1 ? 2 : 0) | (vis2 ? 4 : 0);

        switch(visible_mask) {
            case 0:  /* All behind - skip */
                break;

            case 7:  /* All visible - submit as-is */
                submit_clip_vertex(v0, false);
                submit_clip_vertex(v1, false);
                submit_clip_vertex(v2, true);  /* Always EOL for triangle end */
                break;

            case 1: {  /* Only v0 visible */
                float t01 = clip_edge_t(v0, v1);
                float t02 = clip_edge_t(v0, v2);
                ClipVertex c01 = lerp_vertex(v0, v1, t01);
                ClipVertex c02 = lerp_vertex(v0, v2, t02);
                submit_clip_vertex(v0, false);
                submit_clip_vertex(c01, false);
                submit_clip_vertex(c02, true);
                break;
            }

            case 2: {  /* Only v1 visible */
                float t10 = clip_edge_t(v1, v0);
                float t12 = clip_edge_t(v1, v2);
                ClipVertex c10 = lerp_vertex(v1, v0, t10);
                ClipVertex c12 = lerp_vertex(v1, v2, t12);
                submit_clip_vertex(c10, false);
                submit_clip_vertex(v1, false);
                submit_clip_vertex(c12, true);
                break;
            }

            case 4: {  /* Only v2 visible */
                float t20 = clip_edge_t(v2, v0);
                float t21 = clip_edge_t(v2, v1);
                ClipVertex c20 = lerp_vertex(v2, v0, t20);
                ClipVertex c21 = lerp_vertex(v2, v1, t21);
                submit_clip_vertex(c20, false);
                submit_clip_vertex(c21, false);
                submit_clip_vertex(v2, true);
                break;
            }

            case 3: {  /* v0, v1 visible (v2 behind) - produces quad (2 triangles) */
                float t02 = clip_edge_t(v0, v2);
                float t12 = clip_edge_t(v1, v2);
                ClipVertex c02 = lerp_vertex(v0, v2, t02);
                ClipVertex c12 = lerp_vertex(v1, v2, t12);
                /* Triangle 1: v0, v1, c02 */
                submit_clip_vertex(v0, false);
                submit_clip_vertex(v1, false);
                submit_clip_vertex(c02, true);
                /* Triangle 2: v1, c12, c02 */
                submit_clip_vertex(v1, false);
                submit_clip_vertex(c12, false);
                submit_clip_vertex(c02, true);
                break;
            }

            case 5: {  /* v0, v2 visible (v1 behind) - produces quad (2 triangles) */
                float t01 = clip_edge_t(v0, v1);
                float t21 = clip_edge_t(v2, v1);
                ClipVertex c01 = lerp_vertex(v0, v1, t01);
                ClipVertex c21 = lerp_vertex(v2, v1, t21);
                /* Triangle 1: v0, c01, v2 */
                submit_clip_vertex(v0, false);
                submit_clip_vertex(c01, false);
                submit_clip_vertex(v2, true);
                /* Triangle 2: c01, c21, v2 */
                submit_clip_vertex(c01, false);
                submit_clip_vertex(c21, false);
                submit_clip_vertex(v2, true);
                break;
            }

            case 6: {  /* v1, v2 visible (v0 behind) - produces quad (2 triangles) */
                float t10 = clip_edge_t(v1, v0);
                float t20 = clip_edge_t(v2, v0);
                ClipVertex c10 = lerp_vertex(v1, v0, t10);
                ClipVertex c20 = lerp_vertex(v2, v0, t20);
                /* Triangle 1: c10, v1, c20 */
                submit_clip_vertex(c10, false);
                submit_clip_vertex(v1, false);
                submit_clip_vertex(c20, true);
                /* Triangle 2: v1, v2, c20 */
                submit_clip_vertex(v1, false);
                submit_clip_vertex(v2, false);
                submit_clip_vertex(c20, true);
                break;
            }
        }
    };

    /* ================================================================
     * Strip submission state (shared by indexed and non-indexed paths)
     *
     * The PVR assembles triangle strips natively: only the last vertex of
     * an entire strip needs PVR_CMD_VERTEX_EOL.  Submitting N+2 vertices is
     * therefore cheaper than N×3 vertices.  We maintain the following state:
     *
     *   sp_pending  – last vertex not yet committed (its EOL flag is unknown
     *                 until we see what the next triangle looks like)
     *   sp_in_strip – whether the pending vertex is part of an active sub-strip
     *
     * Winding parity: the PVR automatically flips winding for every other
     * triangle in a strip.  When a clip boundary forces a sub-strip restart at
     * an ODD original strip position, we prepend a single degenerate vertex
     * (cv0 submitted twice) so the hardware's strip counter reaches the right
     * parity.  The degenerate triangle is zero-area and produces no pixels.
     * ================================================================ */
    ClipVertex sp_pending;
    bool sp_has_pending = false;
    bool sp_in_strip    = false;

    auto sp_flush = [&](bool eol) {
        if(sp_has_pending) {
            submit_clip_vertex(sp_pending, eol);
            sp_has_pending = false;
        }
    };

    auto sp_step = [&](const ClipVertex& cv0, const ClipVertex& cv1,
                       const ClipVertex& cv2, bool is_last, std::size_t pos) {
        bool vis0 = is_vertex_visible(cv0);
        bool vis1 = is_vertex_visible(cv1);
        bool vis2 = is_vertex_visible(cv2);
        int mask = (vis0 ? 1 : 0) | (vis1 ? 2 : 0) | (vis2 ? 4 : 0);

        if(mask == 7) {
            /* All visible. */
            if(!sp_in_strip) {
                /* Start a new sub-strip.  At an odd original position the PVR
                 * strip counter is in "even" mode after EOL; prepend cv0 twice
                 * so the first real triangle is processed with odd winding. */
                if(pos & 1) submit_clip_vertex(cv0, false);
                submit_clip_vertex(cv0, false);
                submit_clip_vertex(cv1, false);
                sp_pending    = cv2;
                sp_has_pending = true;
                sp_in_strip   = true;
            } else {
                /* Extend the strip.  cv0 and cv1 are already in the PVR strip
                 * register from previous submissions; only cv2 is new. */
                sp_flush(false);
                sp_pending    = cv2;
                sp_has_pending = true;
            }
            if(is_last) {
                sp_flush(true);
                sp_in_strip = false;
            }
        } else {
            /* Clip boundary or fully invisible: end any open sub-strip. */
            if(sp_in_strip) {
                sp_flush(true);
                sp_in_strip = false;
            }
            if(mask != 0) {
                /* Partially visible: fall back to an individual clipped
                 * triangle.  Odd strip positions swap cv0/cv1 for winding. */
                if(pos & 1) process_triangle(cv1, cv0, cv2, true);
                else        process_triangle(cv0, cv1, cv2, true);
            }
        }
    };

    /* ================================================================
     * Submit geometry with near-plane clipping.
     *
     * Both paths first run transform_batch over the source vertex range
     * that they need, then walk the topology referencing the cached
     * ClipVertex slots in work_vertices_.
     * ================================================================ */
    if(renderable->index_element_count > 0 && renderable->index_data) {
        /* Indexed rendering. */
        const auto* idata = renderable->index_data;
        auto itype = idata->index_type();
        auto icount = renderable->index_element_count;
        const uint8_t* index_ptr = idata->data();

        auto get_index = [&](std::size_t i) -> uint32_t {
            switch(itype) {
                case INDEX_TYPE_8_BIT: return index_ptr[i];
                case INDEX_TYPE_16_BIT: return ((const uint16_t*)index_ptr)[i];
                case INDEX_TYPE_32_BIT: return ((const uint32_t*)index_ptr)[i];
                default: return 0;
            }
        };

        /* Scan once to find the tight [min, max] range of indices.  We
         * transform exactly that contiguous slice, so each vertex hits
         * FTRV / lighting once even when the strip cache would have
         * otherwise re-fetched the same source vertex repeatedly. */
        if(icount == 0) return;
        uint32_t base = 0xFFFFFFFFu;
        uint32_t high = 0;
        for(std::size_t i = 0; i < icount; ++i) {
            uint32_t v = get_index(i);
            if(v < base) base = v;
            if(v > high) high = v;
        }
        transform_batch(base, high - base + 1);

        if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
            for(std::size_t i = 0; i + 2 < icount; i += 3) {
                const ClipVertex& v0 = work_vertices_[get_index(i + 0) - base];
                const ClipVertex& v1 = work_vertices_[get_index(i + 1) - base];
                const ClipVertex& v2 = work_vertices_[get_index(i + 2) - base];
                bool is_last = (i + 5 >= icount);
                process_triangle(v0, v1, v2, is_last);
            }
        } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
            if(icount >= 3) {
                for(std::size_t i = 0; i + 2 < icount; i++) {
                    const ClipVertex& cv0 = work_vertices_[get_index(i + 0) - base];
                    const ClipVertex& cv1 = work_vertices_[get_index(i + 1) - base];
                    const ClipVertex& cv2 = work_vertices_[get_index(i + 2) - base];
                    sp_step(cv0, cv1, cv2, (i + 3 >= icount), i);
                }
                sp_flush(true);
            }
        } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) {
            if(icount >= 3) {
                const ClipVertex& v0 = work_vertices_[get_index(0) - base];
                for(std::size_t i = 1; i + 1 < icount; i++) {
                    const ClipVertex& v1 = work_vertices_[get_index(i) - base];
                    const ClipVertex& v2 = work_vertices_[get_index(i + 1) - base];
                    bool is_last = (i + 2 >= icount);
                    process_triangle(v0, v1, v2, is_last);
                }
            }
        }
    } else {
        /* Non-indexed range-based rendering: each range is independent so
         * we transform it as its own batch. */
        const VertexRange* ranges = renderable->vertex_ranges;
        std::size_t range_count = renderable->vertex_range_count;
        for(std::size_t ri = 0; ri < range_count; ++ri) {
            uint32_t start = ranges[ri].start;
            uint32_t count = ranges[ri].count;
            if(!count) continue;

            transform_batch(start, count);

            if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
                for(uint32_t i = 0; i + 2 < count; i += 3) {
                    const ClipVertex& v0 = work_vertices_[i + 0];
                    const ClipVertex& v1 = work_vertices_[i + 1];
                    const ClipVertex& v2 = work_vertices_[i + 2];
                    bool is_last = (i + 5 >= count) && (ri + 1 >= range_count);
                    process_triangle(v0, v1, v2, is_last);
                }
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
                if(count >= 3) {
                    /* Each range is an independent strip; reset state. */
                    sp_has_pending = false;
                    sp_in_strip    = false;
                    for(uint32_t i = 0; i + 2 < count; i++) {
                        const ClipVertex& cv0 = work_vertices_[i + 0];
                        const ClipVertex& cv1 = work_vertices_[i + 1];
                        const ClipVertex& cv2 = work_vertices_[i + 2];
                        bool is_last = (i + 3 >= count) && (ri + 1 >= range_count);
                        sp_step(cv0, cv1, cv2, is_last, i);
                    }
                    sp_flush(true);
                }
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) {
                if(count >= 3) {
                    const ClipVertex& v0 = work_vertices_[0];
                    for(uint32_t i = 1; i + 1 < count; i++) {
                        const ClipVertex& v1 = work_vertices_[i];
                        const ClipVertex& v2 = work_vertices_[i + 1];
                        bool is_last = (i + 2 >= count) && (ri + 1 >= range_count);
                        process_triangle(v0, v1, v2, is_last);
                    }
                }
            }
        }
    }

#else
    _S_UNUSED(material_pass);
#endif
}

} // namespace smlt
