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
#include "../../application.h"
#include "../../stats_recorder.h"

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
#include <cstring>

namespace smlt {

/* ========================================================================
 * PVR Vertex type: 32-byte packed-color format
 * Layout: flags, x, y, z, u, v, argb, oargb
 *
 * Colors stay in float through transform/clipping/lighting and are only
 * clamped to [0,1] and quantized to bytes here, immediately before
 * submission, minimizing TA input bandwidth and vertex-buffer footprint.
 * The corresponding polygon header must use PVR_CLRFMT_ARGBPACKED. See
 * pvr_build_poly_hdr.
 * ======================================================================== */
#ifdef __DREAMCAST__
typedef struct {
    uint32_t flags;     /* TA command (vertex flags) */
    float x, y, z;      /* Screen coordinates (x, y) and 1/w depth (z) */
    float u, v;         /* Texture coordinates */
    uint32_t argb;      /* Base color, packed 0xAARRGGBB */
    uint32_t oargb;     /* Offset (specular) color, packed 0xAARRGGBB */
} __attribute__((aligned(32))) pvr_vertex_packed_t;

/* Texture size (power-of-2, 8..1024) -> PVR size index 0..7 */
static inline uint32_t pvr_txr_size_idx(int sz) {
    return (uint32_t)(__builtin_ctz((unsigned)sz) - 3);
}

/* Clamp a color channel to [0,1] before packing to a byte. Deliberately not
 * shz_clampf: that lowers to fminf/fmaxf, whose NaN/signed-zero handling drags
 * in __fpclassifyf and made this the hottest part of vertex emission. NaN maps
 * to 0 here, which is a safe color. */
static inline float pvr_clamp01(float v) {
    return v > 0.0f ? (v < 1.0f ? v : 1.0f) : 0.0f;
}

/* Depth for orthographic projections.
 *
 * The PVR compares 1/w, larger = nearer. That breaks for orthographic
 * projections: w is always 1, so every vertex in the scene gets the same
 * depth and nothing can be depth-ordered (every test is a tie). As GLdc does,
 * use 1/(1 + z) instead. Ortho clip-space z runs from -1 (near) to 1 (far)
 * after near-plane clipping, so this stays positive (as shz_invf_fsrra
 * requires) and larger = nearer; the small extra avoids a divide by zero
 * exactly on the near plane.
 *
 * Whether to use it is decided once per draw from the MVP (pvr_is_ortho)
 * rather than per vertex, so it costs nothing for perspective draws: most
 * vertices are packed by pvr_pack_sh4, which always writes 1/w, and only
 * ortho draws then need their depths rewritten. */
static inline float pvr_ortho_depth(float z) {
    return shz_invf_fsrra(1.0001f + z);
}

/* True if the MVP's bottom row is (0, 0, 0, 1), so w is always 1 */
static inline bool pvr_is_ortho(const Mat4& mvp) {
    return mvp[3] == 0.0f && mvp[7] == 0.0f && mvp[11] == 0.0f && mvp[15] == 1.0f;
}

/* Build pvr_poly_hdr_t directly without going through pvr_poly_cxt_t.
 *
 * Fixed for all our draw calls:
 *   color format  = PVR_CLRFMT_ARGBPACKED (32-byte packed-color vertices)
 *   UV format     = PVR_UVFMT_32BIT  (= 0, contributes nothing)
 *   color clamp   = enabled
 *   txr_alpha     = PVR_TXRALPHA_ENABLE (= 0, contributes nothing)
 *   uv_flip/clamp = none  (= 0)
 *   mipmap_bias   = PVR_MIPBIAS_NORMAL (= 4)
 *   mipmap        = disabled (= 0)
 *   no modifier volumes, no user-clip
 *   specular (oargb) = enabled only when the pass has lighting on, so the
 *   TSP's extra per-pixel offset-color add is only paid for where it's
 *   actually used. */
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
    int specular,
    const PVRTextureObject* tex_obj)
{
    const int textured = (tex_obj != nullptr) ? 1 : 0;
    /* alpha enabled for translucent and punch-through lists */
    const int alpha = (list_type != PVR_LIST_OP_POLY) ? 1 : 0;

    /* CMD: base | specular/oargb-enable (bit 2) | texture-enable (bit 3)
     *          | list type (26:24) | color format (6:4) | shade mode (bit 1) */
    hdr->cmd = PVR_CMD_POLYHDR
             | ((uint32_t)specular   << PVR_TA_CMD_SPECULAR_SHIFT)
             | ((uint32_t)textured   << 3)
             | ((uint32_t)list_type  << PVR_TA_CMD_TYPE_SHIFT)
             | (PVR_CLRFMT_ARGBPACKED << PVR_TA_CMD_CLRFMT_SHIFT)
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

/* Maps a material pass to the PVR list its geometry belongs in. Shared by
 * change_material_pass and the start-of-traversal direct-list selection. */
static pvr_list_type_t list_type_for_pass(const MaterialPass* pass) {
    const bool to_modifier =
        (pass->polygon_list_target() == POLYGON_LIST_TARGET_MODIFIER);

    switch(pass->blend_func()) {
        case BLEND_NONE:
            return to_modifier ? PVR_LIST_OP_MOD : PVR_LIST_OP_POLY;
        case BLEND_MASK:
            return to_modifier ? PVR_LIST_OP_MOD : PVR_LIST_PT_POLY;
        default:
            return to_modifier ? PVR_LIST_OP_MOD : PVR_LIST_TR_POLY;
    }
}

void PVRRenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue,
                                             uint64_t frame_id,
                                             StageNode* stage_node) {
    _S_UNUSED(frame_id);

    /* New camera pass — discard any light state cached against the previous
     * view matrix. */
    light_cache_count_ = 0;

    /* Get ambient light from the stage if available */
    if(stage_node) {
        auto a = stage_node->scene->lighting->ambient_light();
        ambient_[0] = a.r;
        ambient_[1] = a.g;
        ambient_[2] = a.b;
    }

#ifdef __DREAMCAST__
    /* Choose the single list to stream directly to the TA for this frame.
     * Each list is weighted by the vertex/index work its renderables will
     * cost; the dominant one is opened once here and its geometry written
     * straight through the store queues. The remaining lists are RAM-buffered
     * and drained in on_post_render.
     *
     * A PVR list may only be opened once per scene, and start_traversal() runs
     * once per layer/pipeline, so the choice and the pvr_list_begin() happen
     * on the first traversal only. Later layers submit into the already-open
     * direct list, or buffer to the others. */
    if(!renderer_->direct_list_chosen_) {
        renderer_->direct_list_chosen_ = true;

        size_t weight[PVRRenderer::PVR_LIST_COUNT] = {0};

        const std::size_t n = queue.renderable_count();
        for(std::size_t i = 0; i < n; ++i) {
            const Renderable* r =
                const_cast<batcher::RenderQueue&>(queue).renderable(i);
            if(!r || !r->material) {
                continue;
            }

            std::size_t elems = r->index_element_count;
            if(!elems) {
                for(std::size_t v = 0; v < r->vertex_range_count; ++v) {
                    elems += r->vertex_ranges[v].count;
                }
            }
            if(!elems) {
                elems = 1;
            }

            const uint8_t passes = r->material->pass_count();
            for(uint8_t p = 0; p < passes; ++p) {
                const pvr_list_type_t lt =
                    list_type_for_pass(r->material->pass(p));
                weight[(size_t) lt] += elems;
            }
        }

        pvr_list_type_t chosen = (pvr_list_type_t) -1;
        size_t best = 0;
        for(size_t i = 0; i < PVRRenderer::PVR_LIST_COUNT; ++i) {
            if(weight[i] > best) {
                best = weight[i];
                chosen = (pvr_list_type_t) i;
            }
        }

        renderer_->direct_list_ = chosen;
        if(chosen != (pvr_list_type_t) -1) {
            pvr_list_begin(chosen);
            pvr_dr_init(&renderer_->dr_state_);
            renderer_->prev_list_type_ = chosen;
            renderer_->current_list_type_ = chosen;
        }
    }

#endif
}

void PVRRenderQueueVisitor::end_traversal(const batcher::RenderQueue& queue,
                                           StageNode* stage_node) {
    _S_UNUSED(queue);
    _S_UNUSED(stage_node);

    if(polygons_rendered_) {
        get_app()->stats->add_polygons_rendered(polygons_rendered_);
        polygons_rendered_ = 0;
    }
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

    /* Cache the base color map's UV transform as scalar coefficients (see
     * the field comment in the header for why this avoids xmtrx). Skipping
     * the multiply entirely in the (common) identity case keeps the no-op
     * cost at a single bool check per vertex. */
    const Mat4& uv_mat = next->base_color_map_matrix();
    uv_matrix_identity_ = (uv_mat == Mat4());
    if(!uv_matrix_identity_) {
        uv_matrix_[0] = uv_mat[0];
        uv_matrix_[1] = uv_mat[4];
        uv_matrix_[2] = uv_mat[12];
        uv_matrix_[3] = uv_mat[1];
        uv_matrix_[4] = uv_mat[5];
        uv_matrix_[5] = uv_mat[13];
    }

    /* Determine PVR list type based on blend mode, then redirect to the
     * modifier-volume list if this pass targets it. All modifier passes go to
     * PVR_LIST_OP_MOD regardless of blend mode. */
    auto blend = next->blend_func();
    const bool to_modifier =
        (next->polygon_list_target() == POLYGON_LIST_TARGET_MODIFIER);
    emitting_modifier_volume_ = to_modifier;

    renderer_->current_list_type_ = list_type_for_pass(next);

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

    /* Sync the PVR's global fog table/color registers (see the cache fields'
     * comment in pvr_renderer.h for why this is deduped and lives on the
     * renderer rather than being rebuilt unconditionally here). GL1x/GL2
     * reach the equivalent state via glFogf/glFogfv on every pass change;
     * on the PVR that would mean rebuilding a 129-entry table on every
     * material switch, so only touch it when the fog params actually
     * differ from what's already programmed. */
    if(fog_type == PVR_FOG_TABLE) {
        const Color& fc = next->fog_color();
        const float density = next->fog_density();
        const float start = next->fog_start();
        const float end = next->fog_end();
        const int32_t mode = (int32_t)next->fog_mode();

        auto& r = *renderer_;
        const bool changed =
            r.fog_mode_cache_ != mode ||
            r.fog_density_cache_ != density ||
            r.fog_start_cache_ != start ||
            r.fog_end_cache_ != end ||
            r.fog_color_cache_[0] != fc.r ||
            r.fog_color_cache_[1] != fc.g ||
            r.fog_color_cache_[2] != fc.b ||
            r.fog_color_cache_[3] != fc.a;

        if(changed) {
            switch(next->fog_mode()) {
                case FOG_MODE_EXP:  pvr_fog_table_exp(density);  break;
                case FOG_MODE_EXP2: pvr_fog_table_exp2(density); break;
                case FOG_MODE_LINEAR:
                default:             pvr_fog_table_linear(start, end); break;
            }
            pvr_fog_table_color(fc.a, fc.r, fc.g, fc.b);

            r.fog_mode_cache_ = mode;
            r.fog_density_cache_ = density;
            r.fog_start_cache_ = start;
            r.fog_end_cache_ = end;
            r.fog_color_cache_[0] = fc.r;
            r.fog_color_cache_[1] = fc.g;
            r.fog_color_cache_[2] = fc.b;
            r.fog_color_cache_[3] = fc.a;
        }
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
            next->is_lighting_enabled() ? PVR_SPECULAR_ENABLE : PVR_SPECULAR_DISABLE,
            tex_obj
        );
    }
#endif
}

void PVRRenderQueueVisitor::compute_light_state(LightPtr light,
                                                 VertexLightState& state) {
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
    state.inv_range = 1.0f / (state.range + 1e-8f);

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

const VertexLightState& PVRRenderQueueVisitor::get_cached_light_state(LightPtr light) {
    for(int i = 0; i < light_cache_count_; ++i) {
        if(light_cache_[i].light == light) {
            return light_cache_[i].state;
        }
    }

    /* Miss: compute once. Cache it if there's room, otherwise fall back to the
     * scratch slot (the eye-space transform is only skipped for repeats, so an
     * overflowing light simply doesn't benefit from caching). */
    VertexLightState* dst;
    if(light_cache_count_ < LIGHT_CACHE_SIZE) {
        LightCacheEntry& entry = light_cache_[light_cache_count_++];
        entry.light = light;
        dst = &entry.state;
    } else {
        dst = &light_scratch_;
    }

    compute_light_state(light, *dst);
    return *dst;
}

void PVRRenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {
    for(int i = 0; i < MAX_LIGHTS; i++) {
        lights_[i].enabled = false;
    }

    for(uint8_t i = 0; i < count && i < MAX_LIGHTS; i++) {
        if(!lights[i]) continue;

        /* The eye-space position/dir/colour are frame-constant per light, so
         * pull them from the per-frame cache instead of recomputing the
         * view-matrix product and normalise on every call. */
        lights_[i] = get_cached_light_state(lights[i]);
        lights_[i].enabled = true;
    }
}


/* ========================================================================
 * Near-plane clipping support
 * ======================================================================== */

#ifdef __DREAMCAST__

/* A vertex in clip space with all attributes needed for interpolation */
struct ClipVertex {
    float x, y, z, w;    /* Clip-space position */
    float u, v;          /* Texture coordinates */
    float r, g, b, a;    /* Base (diffuse+ambient) color */
    float sr, sg, sb;    /* Specular color — offloaded to the PVR's oargb
                          * offset-color unit, added post-texture-modulate
                          * and clamped by hardware (PVR_CLRCLAMP_ENABLE),
                          * so it never needs summing or clamping here. */
    bool ok;             /* False if the source position transformed to a
                          * a NaN or similar */
    /* Padded to exactly two cache lines, so pass 1 can claim a vertex's
     * lines with MOVCA.L instead of reading them from RAM first: the
     * work array is far bigger than the cache, so every line is cold. */
    uint8_t pad[11];
} __attribute__((aligned(32)));
static_assert(sizeof(ClipVertex) == 64, "ClipVertex should be two cache lines");

/* ========================================================================
 * Per-vertex PBR lighting
 * ======================================================================== */

/* A light, compacted for the per-vertex loop: disabled slots are removed and
 * all per-light constants are folded in up-front, so the loop never tests
 * `enabled` or multiplies colour by intensity. */
struct PackedLight {
    float v[3];      /* Eye-space position (point) or unit toward-light dir */
    float col[3];    /* colour × intensity */
    float inv_range;
    bool  point;
};

struct LightingParams {
    const PackedLight* lights;
    float ambient[3];
    float metallic;
    float nm;         /* 1 - metallic */
    float a2;         /* GGX alpha², from which bp_n is derived */
    float bp_n;       /* Blinn-Phong exponent equivalent to a2 */
    float bp_nm1;     /* bp_n - 1 */
    float bp_scale;   /* Blinn-Phong normalisation, (n + 2) / 8 */
};

/* Per-vertex lighting, split into passes that each fit the SH4's 16
 * single-precision FP registers.
 *
 * The previous single-loop kernel kept eye-space P and N, their reciprocal
 * lengths, N.V, both lights' constants, per-light weights and nine colour
 * accumulators live at once. That is far past 16 registers, so GCC reloaded
 * every loop-invariant value (literal-pool constants, LightingParams and
 * PackedLight fields) and spilled temporaries on every vertex: of ~400
 * instructions per vertex only ~90 were lighting math, and because fmov has
 * no displacement addressing each spill/reload also cost an address
 * computation.
 *
 * Here each batch is processed in windows of LIGHT_WINDOW vertices:
 *   1. geometry pass - two FTRVs per vertex, writes unit eye-space N, eye
 *      position, 1/|P| and N.V to a scratch window;
 *   2. one pass per light - only that light's constants are live, so they
 *      can sit in registers for the whole pass;
 *   3. combine pass - folds the accumulated terms into cv's colours.
 * A window is small enough to stay in the operand cache across all passes,
 * so revisiting it is cheap. Windowing is safe because a vertex's lighting
 * depends on nothing but that vertex.
 *
 * On entry XMTRX must hold the modelview. Reads base colour from cv.r/g/b,
 * writes lit diffuse (+ambient) to cv.r/g/b and specular to cv.sr/sg/sb. */
/* Scratch row per vertex, as a flat float array rather than a struct so
 * each pass addresses fields by constant offset. 16 floats = 64 bytes, so a
 * row is exactly two cache lines. The order of the first eight is set by the
 * geometry pass, which writes them back to front in the order it produces
 * them. */
enum : uint32_t {
    LV_NDOTP,               /* N.P / |P| = -N.V, for the N.H identity */
    LV_NX, LV_NY, LV_NZ,    /* unit eye-space normal */
    LV_INVV,                /* 1/|P| */
    LV_PX, LV_PY, LV_PZ,    /* eye-space position */
    LV_W,                   /* per light: diffuse weight, specular weight */
    LV_STRIDE = 16,
    /* Dielectric combine output, over the weights: lit (ambient + diffuse)
     * colour factor and specular colour. */
    LV_DR = LV_W, LV_DG, LV_DB,
    LV_SR = LV_W + 4, LV_SG, LV_SB
};
static_assert(LV_W + 2 * PVRRenderQueueVisitor::MAX_LIGHTS <= LV_STRIDE,
              "LitVertex row too small");
static_assert(LV_SB < LV_STRIDE, "LitVertex row too small");

/* Arguments for the geometry pass in pvr_lighting_sh4.s; layout is fixed by
 * the assembly. Passed by pointer to keep everything in r4-r7. */
struct PvrGeomArgs {
    const uint8_t* pos;
    const uint8_t* nrm;
    uint32_t stride;
    uint32_t n;
    float* out;
    float sqrt_eps;
};
static_assert(sizeof(PvrGeomArgs) == 24, "PvrGeomArgs layout is fixed by the asm");
extern "C" void pvr_light_geometry_sh4(const PvrGeomArgs* args);

/* One light over a window of scratch rows (pvr_lighting_sh4.s), directional
 * or point: each writes doubled diffuse and specular weights at w, w + 1 of
 * each row and uses slots 12-14 as scratch. The point light's attenuation is
 * 1 - distance / range, clamped at 0. pvr_light_point_sh4 clobbers XMTRX.
 * tools/sh4_asm/kbench/pointc.cpp keeps the C++ the point kernel replaced,
 * as its reference.
 *
 * Specular is normalised Blinn-Phong rather than GGX + Smith + Fresnel: the
 * microfacet model needs ~10 constants on top of the light's own and so can
 * never fit the SH4's 16 FP registers. Per-vertex lighting is
 * Gouraud-interpolated by the PVR anyway, so the difference in lobe shape is
 * small. N.H^n uses Schlick's rational approximation x / (n - (n-1)x): one
 * reciprocal, runtime exponent, no pow. Fresnel is applied at normal
 * incidence only, in the combine pass, which also makes the light passes
 * identical for dielectrics and metals. */
extern "C" void pvr_light_dir_sh4(float* rows, float* w, uint32_t n, const float* k);
extern "C" void pvr_light_point_sh4(float* rows, float* w, uint32_t n, const float* k);

/* Dielectric combine passes, for one, two and three lights
 * (pvr_lighting_sh4.s): replace each row's weights with LV_DR.. and LV_SR...
 * `w` is the first row's weights. A third light's weights sit at LV_SR,
 * LV_SR + 1, which the combine reads before it overwrites them. */
static_assert(LV_W == 8 && LV_DR == 8 && LV_SR == 12 && LV_STRIDE == 16,
              "Scratch row layout is fixed by the asm");
extern "C" void pvr_light_combine3_sh4(float* w, uint32_t n);
extern "C" void pvr_light_combine2_sh4(float* w, uint32_t n);
extern "C" void pvr_light_combine1_sh4(float* w, uint32_t n);


static const uint32_t LIGHT_WINDOW = 32;   /* = XFORM_WINDOW */



/* Bias that keeps the geometry pass's reciprocal square roots finite for a
 * zero-length N or P (at the eye): it is added squared, as 1e-18. */
static const float LIGHT_SQRT_EPS = 1e-9f;
/* A window's scratch: the lighting rows and the ClipVertex records pass 1
 * writes, together (4KB), so that the two can never evict each other in
 * the direct-mapped 16KB operand cache. (Separate arrays aliased every
 * several windows, and pass 1 and pack then stalled on the cache though
 * their data was small and nominally resident.) */
struct WindowScratch {
    float lit[LIGHT_WINDOW][LV_STRIDE];
    ClipVertex cv[LIGHT_WINDOW];
};
alignas(32) static WindowScratch window_scratch_;
static float (&lit_window_)[LIGHT_WINDOW][LV_STRIDE] = window_scratch_.lit;
static ClipVertex (&cv_window_)[LIGHT_WINDOW] = window_scratch_.cv;

template<bool Dielectric, int NumLights>
__attribute__((noinline))
static void light_vertices(const LightingParams& lp, ClipVertex* cv,
                           const uint8_t* row, uint32_t count, uint32_t stride,
                           uint32_t pos_offset, uint32_t normal_offset) {

    /* One window per call: the caller slices batches to at most
     * XFORM_WINDOW vertices, and the dielectric combine below leaves the
     * light-colour matrix in XMTRX, so a second geometry pass here would
     * transform by the wrong matrix. */
    assert(count <= LIGHT_WINDOW);
    {
        const uint32_t start = 0;
        const uint32_t n = count;

        /* 1. Geometry: eye-space P and unit N, 1/|P| and N.V per vertex
         * (pvr_lighting_sh4.s). XMTRX holds the modelview. */
        {
            PvrGeomArgs ga;
            ga.pos = row + stride * start + pos_offset;
            ga.nrm = row + stride * start + normal_offset;
            ga.stride = stride;
            ga.n = n;
            ga.out = lit_window_[0];
            ga.sqrt_eps = LIGHT_SQRT_EPS;
            pvr_light_geometry_sh4(&ga);
        }

        /* 2. One pass per light. */
#pragma GCC unroll 4
        for(int li = 0; li < NumLights; ++li) {
            const PackedLight& L = lp.lights[li];
            const float sqrt8 = 2.8284271f;
            if(L.point) {
                /* pvr_light_point_sh4: see its comment for k. XMTRX takes
                 * (1, P) to (sqrt_eps, P - Lpos); the geometry pass is done
                 * with the modelview, and the caller reloads MVP. */
                alignas(8) float k[22] = {
                    LIGHT_SQRT_EPS, -L.v[0], -L.v[1], -L.v[2],
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f,
                    1.0f + 4e-6f,
                    L.inv_range * 0.5f,
                    0.5f,
                    lp.bp_nm1 / sqrt8,
                    -lp.bp_n,
                    lp.bp_scale / sqrt8,
                };
                pvr_light_point_sh4(lit_window_[0], lit_window_[0] + LV_W + 2 * li, n, k);
            } else {
                /* pvr_light_dir_sh4: see its comment for k. */
                alignas(8) float k[8] = {
                    0.0f, -L.v[0], -L.v[1], -L.v[2],
                    1.0f + 4e-6f,
                    lp.bp_nm1 / sqrt8,
                    -lp.bp_n,
                    lp.bp_scale / sqrt8,
                };
                pvr_light_dir_sh4(lit_window_[0], lit_window_[0] + LV_W + 2 * li, n, k);
            }
        }

        /* 3. Combine: light colours, Fresnel at normal incidence. Dielectrics
         * write the lit colour factor and specular to the scratch rows, which
         * pass 1 then applies (see transform_window); metals fold straight
         * into cv. Diffuse (+ambient) and specular stay separate and go out
         * through the vertex's base/offset (oargb) colours; the PVR's TSP
         * adds and clamps them (PVR_CLRCLAMP_ENABLE), and applies specular
         * after texture modulation so it isn't tinted by the surface. */
        if(Dielectric) {
            /* XMTRX is free once the geometry pass is done, so it holds the
             * combine's constants: columns 0-2 = lights 0-2's colours (zero
             * for absent lights; all pre-scaled by the 0.96 diffuse share of
             * F0 = 0.04), column 3 = (ambient, 1). Then
             *   FTRV(wd0, wd1, wd2, 1) = ambient + 0.96 * sum(wd * colour)
             *   FTRV(ws0, ws1, ws2, 0) = 0.96 * sum(ws * colour)
             * and the specular weights were already scaled by 0.04 / 0.96 in
             * bp_scale, so the second is exactly the 4% specular. See
             * pvr_light_combine*_sh4 in pvr_lighting_sh4.s. The caller
             * reloads MVP afterwards. */
            alignas(32) static shz_mat4x4_t cm;
            float* m = cm.elem;
            for(int col = 0; col < 3; ++col) {
                const float* lc = (col < NumLights) ? lp.lights[col].col : nullptr;
                /* x 0.5: the light passes store doubled weights */
                m[col * 4 + 0] = lc ? lc[0] * 0.48f : 0.0f;
                m[col * 4 + 1] = lc ? lc[1] * 0.48f : 0.0f;
                m[col * 4 + 2] = lc ? lc[2] * 0.48f : 0.0f;
                m[col * 4 + 3] = 0.0f;
            }
            m[12] = lp.ambient[0]; m[13] = lp.ambient[1]; m[14] = lp.ambient[2];
            m[15] = 1.0f;
            shz_xmtrx_load_4x4(&cm);

            if(NumLights > 2) {
                pvr_light_combine3_sh4(lit_window_[0] + LV_W, n);
            } else if(NumLights > 1) {
                pvr_light_combine2_sh4(lit_window_[0] + LV_W, n);
            } else {
                pvr_light_combine1_sh4(lit_window_[0] + LV_W, n);
            }
        } else {
            const float ar = lp.ambient[0], ag = lp.ambient[1], ab = lp.ambient[2];
            ClipVertex* c = cv + start;
            for(uint32_t i = 0; i < n; ++i, ++c) {
                const float* w = lit_window_[i] + LV_W;
                float dr = 0.0f, dg = 0.0f, db = 0.0f;
                float sr = 0.0f, sg = 0.0f, sb = 0.0f;
#pragma GCC unroll 4
                for(int li = 0; li < NumLights; ++li) {
                    const float* col = lp.lights[li].col;
                    /* the light passes store doubled weights */
                    const float wd = 0.5f * *w++, ws = 0.5f * *w++;
                    dr += wd * col[0]; dg += wd * col[1]; db += wd * col[2];
                    sr += ws * col[0]; sg += ws * col[1]; sb += ws * col[2];
                }

                const float br = c->r, bg = c->g, bb = c->b;
                const float mt = lp.metallic, nm = lp.nm;
                const float F0r = 0.04f + (br - 0.04f) * mt;
                const float F0g = 0.04f + (bg - 0.04f) * mt;
                const float F0b = 0.04f + (bb - 0.04f) * mt;
                c->r = br * (ar + nm * (1.0f - F0r) * dr);
                c->g = bg * (ag + nm * (1.0f - F0g) * dg);
                c->b = bb * (ab + nm * (1.0f - F0b) * db);
                c->sr = F0r * sr; c->sg = F0g * sg; c->sb = F0b * sb;
            }
        }
    }
}

/* Check if a vertex is in front of the near plane (visible).
 * In clip space, the near plane is at z = -w for the standard projection. */
static inline bool is_vertex_visible(const ClipVertex& v) {
    return v.z >= -v.w;
}

/* NaN and Inf screen coordinates hard-lock the TA, and near-plane clipping
 * does *not* filter them out. A single bad source position therefore turns
 * into a lockup, but only on the frames where it happens to straddle the near plane.
 *
 * fabs (< LIMIT) rejects NaN (fabs(NaN) is NaN, which compares false) and Inf
 * with a single comparison per component instead of two. */
static inline bool is_clip_position_valid(float x, float y, float z, float w) {
    const float LIMIT = 1.0e18f;
    return shz_fabsf(x) < LIMIT &&
           shz_fabsf(y) < LIMIT &&
           shz_fabsf(z) < LIMIT &&
           shz_fabsf(w) < LIMIT;
}

/* Pass 1 of the vertex transform (see transform_window): clip-space
 * position, UVs and colour for a window of vertices.
 *
 * Everything the loop reads is passed in by value and specialised on, so the
 * loop keeps it in registers: as part of a lambda capturing by reference,
 * GCC reloaded stride, offsets, the colour format and the material colour
 * through pointers every vertex, and re-tested the format, at ~180
 * instructions a vertex. Missing attributes read a static default with a
 * zero stride instead of branching. */
enum Pass1Color { P1_COLOR_4F, P1_COLOR_3F, P1_COLOR_4UB_RGBA, P1_COLOR_4UB_BGRA };

struct Pass1Args {
    const uint8_t* pos; uint32_t pos_stride;
    const uint8_t* uv; uint32_t uv_stride;
    const uint8_t* col; uint32_t col_stride;
    float base[4];          /* material colour, or 1 if the vertex colour replaces it */
    const float* uvm;       /* affine UV matrix, if UVMat */
    const float* lit;       /* first scratch row, if Lit (see LV_DR / LV_SR) */
};

/* Pass 1 in pvr_lighting_sh4.s, for 4F or absent colour and no UV matrix;
 * layout fixed by the asm. Returns the number of vertices with a non-finite
 * clip position (ok = false), whose position the caller replaces. */
struct Pass1AsmArgs {
    const uint8_t* pos;
    const uint8_t* uv;
    const uint8_t* col;
    const float* lit;       /* D at +0, S at +16 */
    ClipVertex* out;
    uint32_t n;
    int32_t pos_step, uv_step, col_step, lit_step;  /* stride - bytes post-incremented */
    float material[4];
    float limit;
};
static_assert(sizeof(Pass1AsmArgs) == 60, "Pass1AsmArgs layout is fixed by the asm");
static_assert(offsetof(ClipVertex, ok) == 52 && sizeof(ClipVertex) == 64,
              "ClipVertex layout is fixed by the asm");
extern "C" uint32_t pvr_pass1_sh4(const Pass1AsmArgs* args);

/* A lit row for unlit vertices: D = 1, S = 0. */
alignas(32) static const float P1_UNLIT_ROW[8] = {1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

template<Pass1Color Color, bool Lit, bool UVMat>
__attribute__((noinline))
static void pass1_window(const Pass1Args& a, ClipVertex* cv, uint32_t count) {
    const uint8_t* pos = a.pos;
    const uint8_t* uv = a.uv;
    const uint8_t* col = a.col;
    const uint32_t ps = a.pos_stride, us = a.uv_stride, cs = a.col_stride;
    const float mr = a.base[0], mg = a.base[1], mb = a.base[2], ma = a.base[3];
    float m0 = 0, m1 = 0, m2 = 0, m3 = 0, m4 = 0, m5 = 0;
    if(UVMat) {
        m0 = a.uvm[0]; m1 = a.uvm[1]; m2 = a.uvm[2];
        m3 = a.uvm[3]; m4 = a.uvm[4]; m5 = a.uvm[5];
    }
    const float* lit = a.lit;

    for(uint32_t i = 0; i < count; ++i, ++cv) {
        /* Keep the sequential reads ahead of the cache (PREF never faults). */
        SHZ_PREFETCH(pos + ps * 4);

        const float* p = (const float*) pos;
        const shz_vec4_t clip = shz_xmtrx_transform_vec4(
            shz_vec4_init(p[0], p[1], p[2], 1.0f));

        /* Independent work while the FTRV completes. */
        const float* t = (const float*) uv;
        float u = t[0], v = t[1];
        if(UVMat) {
            const float u0 = u;
            u = m0 * u0 + m1 * v + m2;
            v = m3 * u0 + m4 * v + m5;
        }

        float r, g, b, al;
        if(Color == P1_COLOR_4F || Color == P1_COLOR_3F) {
            const float* c = (const float*) col;
            r = c[0]; g = c[1]; b = c[2];
            al = (Color == P1_COLOR_4F) ? c[3] : 1.0f;
        } else {
            const float k = 1.0f / 255.0f;
            const float c0 = col[0] * k, c1 = col[1] * k, c2 = col[2] * k;
            al = col[3] * k;
            if(Color == P1_COLOR_4UB_RGBA) { r = c0; g = c1; b = c2; }
            else { r = c2; g = c1; b = c0; }
        }
        r *= mr; g *= mg; b *= mb; al *= ma;

        float sr = 0.0f, sg = 0.0f, sb = 0.0f;
        if(Lit) {
            r *= lit[LV_DR]; g *= lit[LV_DG]; b *= lit[LV_DB];
            sr = lit[LV_SR]; sg = lit[LV_SG]; sb = lit[LV_SB];
            lit += LV_STRIDE;
        }

        cv->u = u; cv->v = v;
        cv->r = r; cv->g = g; cv->b = b; cv->a = al;
        cv->sr = sr; cv->sg = sg; cv->sb = sb;

        if(is_clip_position_valid(clip.x, clip.y, clip.z, clip.w)) {
            cv->x = clip.x; cv->y = clip.y; cv->z = clip.z; cv->w = clip.w;
            cv->ok = true;
        } else {
            /* Bad vertex position. Swap for a point that is guaranteed to
             * test as behind the near plane (is_vertex_visible: 0 >= 1 is
             * false), so it can never be emitted directly, and flag it so
             * the clip path drops primitives that touch it rather than
             * interpolating towards it. */
            cv->x = 0.0f; cv->y = 0.0f; cv->z = 0.0f; cv->w = -1.0f;
            cv->ok = false;
        }

        pos += ps; uv += us; col += cs;
    }
}

template<Pass1Color Color, bool Lit>
static void pass1_dispatch_uv(const Pass1Args& a, ClipVertex* cv, uint32_t count) {
    if(a.uvm) pass1_window<Color, Lit, true>(a, cv, count);
    else pass1_window<Color, Lit, false>(a, cv, count);
}

template<Pass1Color Color>
static void pass1_dispatch_lit(const Pass1Args& a, ClipVertex* cv, uint32_t count) {
    if(a.lit) pass1_dispatch_uv<Color, true>(a, cv, count);
    else pass1_dispatch_uv<Color, false>(a, cv, count);
}

/* After the transform a batch's vertices are packed for submission (see
 * pvr_pack_sh4), one 32-byte PVR vertex each, in work_ (see do_visit). The
 * clipping path, which also needs a vertex's clip-space position, works it
 * out again from the source vertex (see cv_at): it's needed for ~100
 * triangles a frame, where storing it cost 32 more bytes of cache traffic
 * per vertex, for every vertex. */

/* Copy a 32-byte packed vertex as four 64-bit FPU moves (FPSCR.SZ=1)
 * rather than eight 32-bit loads and stores: emission is bound by the
 * load/store unit, and this halves its slots. `dst` is a store queue or a
 * claimed cache line; both src and dst are 32-byte aligned. Uses fr0-fr7,
 * which are caller-saved, and leaves XMTRX (the batch MVP) alone. */
static SHZ_FORCE_INLINE void copy_vertex32(uint32_t* dst, const void* src) {
    const void* s = src;
    uint32_t* d = dst + 8;
    __asm__ volatile(
        "fschg\n\t"
        "fmov @%[s]+, dr0\n\t"
        "fmov @%[s]+, dr2\n\t"
        "fmov @%[s]+, dr4\n\t"
        "fmov @%[s]+, dr6\n\t"
        "fmov dr6, @-%[d]\n\t"
        "fmov dr4, @-%[d]\n\t"
        "fmov dr2, @-%[d]\n\t"
        "fmov dr0, @-%[d]\n\t"
        "fschg"
        : [s] "+r"(s), [d] "+r"(d)
        : "m"(*(const char(*)[32]) src)
        : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "memory");
}

/* Emit one packed vertex to the next store queue (the direct list) or to the
 * RAM staging cursor. The vertex's flags word is already PVR_CMD_VERTEX
 * (only vertices that passed pvr_pack_sh4's test are emitted from here), so
 * only the end of a strip needs its command word replacing. `q` is a local
 * copy of KOS's pvr_dr_addr (see pvr_dr_target), kept in a register across
 * the emission loops and written back by the caller. */
static SHZ_FORCE_INLINE void put_packed_sq(uint32_t& q, const pvr_vertex_packed_t* v,
                                           bool eol) {
    q ^= 32;
    uint32_t* d = reinterpret_cast<uint32_t*>(q);
    copy_vertex32(d, v);
    if(eol) d[0] = PVR_CMD_VERTEX_EOL;
    __asm__ volatile("pref @%0" :: "r"(d) : "memory");
}

static SHZ_FORCE_INLINE void put_packed_ram(uint8_t*& cur, const pvr_vertex_packed_t* v,
                                            bool eol) {
    uint32_t* d = reinterpret_cast<uint32_t*>(cur);
    /* A whole, cold cache line: claim it rather than read it. */
    __asm__ volatile("movca.l %1, @%0" :: "r"(d), "z"(0) : "memory");
    copy_vertex32(d, v);
    if(eol) d[0] = PVR_CMD_VERTEX_EOL;
    cur += 32;
}

/* Independent triangles from packed vertices. Everything the loop touches
 * is a local, and the packed vertices of a few triangles ahead are
 * prefetched: for a batch bigger than the cache they are long evicted by
 * the time the topology walk reaches them. A triangle with any vertex that
 * can't be submitted as-is (behind the near plane, or non-finite) goes to
 * `slow`, the clipping path, unless none of them can (nothing to draw); `grow` makes room in the staging buffer. Both
 * update `dr` / `cur` / `end` themselves, which is why those are
 * references. */
template<typename Index, typename Slow, typename Grow>
__attribute__((noinline))
static void emit_triangles(Index index, std::size_t ntris, const pvr_vertex_packed_t* pv,
                           bool direct, uint8_t*& cur, uint8_t*& end,
                           Slow slow, Grow grow) {
    const std::size_t AHEAD = 4;
    uint32_t q = pvr_dr_addr;
    for(std::size_t t = 0; t < ntris; ++t) {
        if(t + AHEAD < ntris) {
            const std::size_t k = (t + AHEAD) * 3;
            SHZ_PREFETCH(&pv[index(k + 0)]);
            SHZ_PREFETCH(&pv[index(k + 1)]);
            SHZ_PREFETCH(&pv[index(k + 2)]);
        }
        const uint32_t i0 = index(t * 3 + 0);
        const uint32_t i1 = index(t * 3 + 1);
        const uint32_t i2 = index(t * 3 + 2);
        const pvr_vertex_packed_t* a = &pv[i0];
        const pvr_vertex_packed_t* b = &pv[i1];
        const pvr_vertex_packed_t* c = &pv[i2];
        if(a->flags & b->flags & c->flags) {
            if(direct) {
                put_packed_sq(q, a, false);
                put_packed_sq(q, b, false);
                put_packed_sq(q, c, true);
            } else {
                if(end - cur < 96) grow();
                put_packed_ram(cur, a, false);
                put_packed_ram(cur, b, false);
                put_packed_ram(cur, c, true);
            }
        } else if(a->flags | b->flags | c->flags) {
            /* Partly visible: clip it. */
            pvr_dr_addr = q;
            slow(i0, i1, i2);
            q = pvr_dr_addr;
        } else {
        }
        /* Otherwise every vertex is behind the near plane (or poisoned,
         * which tests the same) and the clipping path would emit nothing:
         * skip it rather than unpacking three ClipVertex to find that out. */
    }
    pvr_dr_addr = q;
}

/* Pack a window of transformed vertices into ready-to-submit PVR vertices
 * (pvr_lighting_sh4.s): screen position (viewport + perspective divide), UV
 * and clamped, packed base and offset colours. Done once per vertex, while
 * the window's ClipVertex slots are still in the cache, rather than per
 * triangle corner at emission - indexed meshes reference each vertex
 * several times. The flags word is PVR_CMD_VERTEX if the vertex can be
 * submitted as-is (finite and in front of the near plane) and 0 if not;
 * emission replaces it with the vertex's real command word, and any
 * triangle with a 0 goes down the clipping path, which works from the
 * ClipVertex. Assumes non-negative colours (true of
 * every colour the transform produces from non-negative material and vertex
 * colours); a negative channel would spill into its neighbour's byte.
 * consts = {half width, half height, 127.5, -127.5}. */


extern "C" void pvr_pack_sh4(const ClipVertex* in, pvr_vertex_packed_t* out, uint32_t n,
                             const float* consts);
static_assert(sizeof(pvr_vertex_packed_t) == 32, "pvr_pack_sh4 writes 32-byte vertices");

/* A triangle strip from packed vertices, as sub-strips: runs of fully
 * visible triangles stream straight out, and each triangle that isn't
 * fully visible ends the current sub-strip and, if partly visible, goes to
 * `slow` (the clipping path) as an independent triangle. Winding: the PVR
 * flips it for every other triangle of a strip, so a sub-strip starting at
 * an odd position of the original strip gets a degenerate lead vertex to
 * restore the parity, and a clipped triangle at an odd position has its
 * first two vertices swapped. Inside a sub-strip only the newly added
 * vertex needs a visibility test - the other two were in the previous
 * (visible) triangle. `count` is the strip's vertex count. */
template<typename Index, typename Slow, typename Grow>
__attribute__((noinline))
static void emit_strip(Index index, std::size_t count, const pvr_vertex_packed_t* pv,
                       bool direct, uint8_t*& cur, uint8_t*& end,
                       Slow slow, Grow grow) {
    if(count < 3) return;

    uint32_t q = pvr_dr_addr;
    auto put = [&](uint32_t i, bool eol) {
        if(direct) {
            put_packed_sq(q, &pv[i], eol);
        } else {
            if(end - cur < 32) grow();
            put_packed_ram(cur, &pv[i], eol);
        }
    };

    bool in_strip = false;
    uint32_t pending = 0;   /* last vertex of the open sub-strip, not yet sent */
    uint32_t i0 = index(0), i1 = index(1);
    const std::size_t AHEAD = 8;
    for(std::size_t pos = 0; pos + 2 < count; ++pos) {
        if(pos + 2 + AHEAD < count) SHZ_PREFETCH(&pv[index(pos + 2 + AHEAD)]);
        const uint32_t i2 = index(pos + 2);
        if(in_strip && pv[i2].flags) {
            /* Extend: the PVR already has the previous two vertices. */
            put(pending, false);
            pending = i2;
        } else if(!in_strip && (pv[i0].flags & pv[i1].flags & pv[i2].flags)) {
            /* Start a sub-strip. */
            if(pos & 1) put(i0, false);
            put(i0, false);
            put(i1, false);
            pending = i2;
            in_strip = true;
        } else {
            /* Clip boundary or fully invisible: end any open sub-strip. */
            if(in_strip) {
                put(pending, true);
                in_strip = false;
            }
            /* Partly visible: an individual clipped triangle. Fully
             * invisible (all behind the near plane): nothing to draw. */
            if(pv[i0].flags | pv[i1].flags | pv[i2].flags) {
                pvr_dr_addr = q;
                if(pos & 1) slow(i1, i0, i2);
                else        slow(i0, i1, i2);
                q = pvr_dr_addr;
            }
        }
        i0 = i1;
        i1 = i2;
    }
    if(in_strip) put(pending, true);
    pvr_dr_addr = q;
}

static void pass1(const Pass1Args& a, Pass1Color color, ClipVertex* cv, uint32_t count) {
    switch(color) {
        case P1_COLOR_4F: pass1_dispatch_lit<P1_COLOR_4F>(a, cv, count); break;
        case P1_COLOR_3F: pass1_dispatch_lit<P1_COLOR_3F>(a, cv, count); break;
        case P1_COLOR_4UB_RGBA: pass1_dispatch_lit<P1_COLOR_4UB_RGBA>(a, cv, count); break;
        case P1_COLOR_4UB_BGRA: pass1_dispatch_lit<P1_COLOR_4UB_BGRA>(a, cv, count); break;
    }
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
    out.sr = shz_lerpf(v1.sr, v2.sr, t);
    out.sg = shz_lerpf(v1.sg, v2.sg, t);
    out.sb = shz_lerpf(v1.sb, v2.sb, t);
    /* Callers only interpolate between two usable vertices — process_triangle
     * drops any primitive touching a poisoned one first. */
    out.ok = true;
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

#ifdef __DREAMCAST__
/* The modifier-volume submission path (see the comment in do_visit). Out of
 * line and cold: it only runs for ShadowCaster's volumes, and inline it made
 * do_visit's body several KB bigger - the per-renderable path is what keeps
 * missing the SH4's 8KB instruction cache. */
__attribute__((noinline, cold))
void PVRRenderQueueVisitor::do_visit_modifier_volume(const Renderable* renderable) {
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
    const bool ortho = pvr_is_ortho(mvp);

    const float hw = 320.0f;
    const float hh = 240.0f;

    const auto& spec = vdata->vertex_specification();
    const auto stride = vdata->stride();
    const auto pos_offset = spec.position_offset(false);
    const uint8_t* raw_data = vdata->data();

    auto& buf = renderer_->buffer(renderer_->current_list_type_)
                    .buffers[renderer_->current_buffer_index_];

    shz_xmtrx_load_4x4((shz_mat4x4_t*) mvp._native());

    /* Position-only clip-space vertex. `ok` mirrors ClipVertex::ok. */
    struct ModVtx { float x, y, z, w; bool ok; };

    auto load_clip = [&](uint32_t vi) -> ModVtx {
        const float* p = (const float*)(raw_data + stride * vi + pos_offset);
        shz_vec4_t c = shz_xmtrx_transform_vec4(
            shz_vec4_init(p[0], p[1], p[2], 1.0f));
        if(!is_clip_position_valid(c.x, c.y, c.z, c.w)) {
            /* Same substitution as the polygon path — see the comment in
             * transform_batch. Always tests as behind the near plane, and
             * flagged so the switch below drops any triangle touching it. */
            return {0.0f, 0.0f, 0.0f, -1.0f, false};
        }
        return {c.x, c.y, c.z, c.w, true};
    };

    /* Linear interpolation in clip space. Callers only interpolate between
     * two usable vertices. */
    auto lerp_clip = [](const ModVtx& a, const ModVtx& b, float t) -> ModVtx {
        return {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t,
            true,
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
        float inv_w = shz_invf_fsrra(w); /* w >= FLT_EPSILON > 0 */
        sx = (v.x * hw + hw * w) * inv_w;
        sy = (-v.y * hh + hh * w) * inv_w;
        sz = ortho ? pvr_ortho_depth(v.z) : inv_w;
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

        /* See the matching guard in process_triangle: a poisoned vertex is
         * always invisible, so only the clipped masks can drag it into the
         * output via interpolation. */
        if(mask != 7 && mask != 0 && !(v0.ok && v1.ok && v2.ok)) {
            continue;
        }

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
}
#endif


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
        do_visit_modifier_volume(renderable);
        return;
    }

    const auto& model = *renderable->final_transformation;
    const auto& view = camera_->view_matrix();
    const auto& projection = camera_->projection_matrix();

    /* Build the modelview-projection matrix */
    Mat4 modelview = view * model;
    Mat4 mvp = projection * modelview;
    const bool ortho = pvr_is_ortho(mvp);

    /* The lighting geometry pass only uses the xyz of its transforms, so the
     * modelview's fourth row is free: (0, 0, 0, sqrt_eps) makes P.w equal
     * sqrt_eps, which folds FSRRA's bias into P.P (see
     * pvr_lighting_sh4.s). */
    alignas(8) shz_mat4x4_t light_modelview;
    std::memcpy(&light_modelview, modelview._native(), sizeof(light_modelview));
    light_modelview.elem[3] = light_modelview.elem[7] = light_modelview.elem[11] = 0.0f;
    light_modelview.elem[15] = LIGHT_SQRT_EPS;

    /* Build viewport transform matrix */
    float hw = 320.0f; /* Half-width */
    float hh = 240.0f; /* Half-height */

    /* Append bytes to the current list: either to the RAM staging buffer, or
     * (if this list was chosen as the direct list in start_traversal) straight
     * to the TA via the store queues. */
    /* While emitting into a RAM-staged list, vertices are written through a
     * cursor into space grown in large steps (see stage_open below), rather
     * than resizing the buffer per 32-byte vertex; the buffer is trimmed to
     * what was written when do_visit returns. */
    PVRRenderer::StagingBuffer* stage_buf = nullptr;
    uint8_t* stage_cur = nullptr;
    uint8_t* stage_end = nullptr;

    auto stage_grow = [&](size_t need) {
        uint8_t* base = reinterpret_cast<uint8_t*>(stage_buf->data());
        const size_t used = stage_cur - base;
        const size_t cap = stage_end - base;
        size_t extra = cap / 2 + 4096;
        if(extra < need) extra = need;
        stage_buf->resize((cap + extra + 31) & ~size_t(31));
        base = reinterpret_cast<uint8_t*>(stage_buf->data());
        stage_cur = base + used;
        stage_end = base + stage_buf->size();
    };

    /* The next 32 bytes of the staged list. Each is a whole cache line
     * (the buffer is 32-byte aligned and grows in 32-byte steps), claimed
     * with MOVCA.L so the cold staging buffer isn't read from RAM first. */
    auto stage_next = [&]() -> uint32_t* {
        if(stage_cur + 32 > stage_end) stage_grow(32);
        uint32_t* d = reinterpret_cast<uint32_t*>(stage_cur);
        __asm__ volatile("movca.l %1, @%0" :: "r"(d), "z"(0) : "memory");
        stage_cur += 32;
        return d;
    };

    auto submit_bytes = [&](const void* data, size_t size) {
        const pvr_list_type_t list = renderer_->current_list_type_;
        if(stage_buf) {
            /* Only ever 32-byte vertices here (see stage_open). */
            const uint32_t* s = reinterpret_cast<const uint32_t*>(data);
            uint32_t* d = stage_next();
            d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
            d[4] = s[4]; d[5] = s[5]; d[6] = s[6]; d[7] = s[7];
            _S_UNUSED(size);
        } else if(renderer_->is_list_direct(list)) {
            /* Stream straight to the TA. pvr_dr_target() hands back a store
             * queue window address, and shz_sq_memcpy32() is the purpose-built
             * SQ copy: it stores the 32 bytes and flushes the queue itself, so
             * no cache-line allocation or separate pvr_dr_commit() is needed. */
            const uint8_t* src = reinterpret_cast<const uint8_t*>(data);
            void* dest = pvr_dr_target(renderer_->dr_state_);
            shz_sq_memcpy32(dest, src, size);
        } else {
            auto& buf = renderer_->buffer(list)
                            .buffers[renderer_->current_buffer_index_];
            const uint8_t* src = reinterpret_cast<const uint8_t*>(data);
            std::size_t pos = buf.size();
            buf.resize(pos + size);
            /* Every submission is a 32-byte header or vertex from a
             * 32-byte-aligned source, and the staging buffer is a 32-byte
             * aligned_vector that only ever grows in 32-byte steps. */
            shz_memcpy32(&buf[pos], src, size);
        }
    };

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

    /* Skip resubmitting a header that's byte-identical to the one already
     * active for this list: the TA updates its internal poly state for every
     * header it sees, so resending an unchanged header for every renderable
     * that shares a material pass (and shadow-receive flag, which `hdr_src`
     * above folds in) is wasted work. The cache lives per-list on the
     * renderer and is invalidated once per frame, so the first poly submitted
     * to a list always sends its header. */
    bool& cache_valid = renderer_->last_header_valid_[renderer_->current_list_type_];
    pvr_poly_hdr_t& cache_bytes = renderer_->last_header_[renderer_->current_list_type_];
    const bool header_unchanged =
        cache_valid && std::memcmp(&cache_bytes, hdr_src, sizeof(pvr_poly_hdr_t)) == 0;

    if(!header_unchanged) {
        /* All polygon lists, including OP_POLY, go through submit_bytes: a
         * list is only opened (and thus switched to direct submission) once it
         * is promoted, and it must stay open from then on. Opening OP_POLY
         * eagerly in pre_render would mean a later promotion of another list
         * closes it permanently, corrupting any OP_POLY geometry that follows. */
        submit_bytes(hdr_src, sizeof(pvr_poly_hdr_t));

        cache_bytes = *hdr_src;
        cache_valid = true;
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

    const auto color_mat_mode = material_pass->color_material();
    /* Hoisted out of the per-vertex loop: a constant per material pass. */
    const bool color_replaces_base =
        (color_mat_mode == COLOR_MATERIAL_DIFFUSE ||
         color_mat_mode == COLOR_MATERIAL_AMBIENT_AND_DIFFUSE);
    bool lighting_enabled = material_pass->is_lighting_enabled() && normal_offset;

    /* ================================================================
     * Two-pass vertex transformation
     * Each pass below uses a narrow working set, so the SH4 register file
     * is largely sufficient.  Pass 1 needs the loaded MVP in xmtrx for the
     * FTRV position transform and folds in the UV / colour reads (no matrix);
     * the optional lighting pass uses xmtrx for modelview transforms and
     * computes lighting against a fixed-size light table.
     * ================================================================ */
    /* Fixed transform scratch. Every slot below `count` is written before it
     * is read and slots above are never read, so a plain array avoids the
     * growth/initialization bookkeeping of a vector. Primitive lists whose
     * vertices span more than this are drawn a chunk at a time (see
     * emit_list); meshes do get that big (one of the cave sample's OBJ
     * submeshes is a 5000-vertex range). */
    static const uint32_t WORK_VERTEX_CAPACITY = 4096;
    /* Per batch vertex, packed for submission (see pvr_pack_sh4). The
     * window's ClipVertex records only live in cv_window_ until they are
     * packed. */
    alignas(32) static pvr_vertex_packed_t work_[WORK_VERTEX_CAPACITY];

    /* Transform + light the slice [first, first + count) of the batch that
     * starts at source vertex `base`. See transform_batch below for why this
     * works a window at a time. */
    /* Pass 1's per-renderable constants (see pass1_window). */
    static const float P1_ZERO_UV[2] = {0.0f, 0.0f};
    static const float P1_ONE_COLOR[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    Pass1Color p1_color = P1_COLOR_4F;
    bool p1_has_color = false;
    if(color_offset) {
        switch(spec.color_attribute) {
            case VERTEX_ATTRIBUTE_4F: p1_color = P1_COLOR_4F; p1_has_color = true; break;
            case VERTEX_ATTRIBUTE_3F: p1_color = P1_COLOR_3F; p1_has_color = true; break;
            case VERTEX_ATTRIBUTE_4UB_RGBA:   /* == VERTEX_ATTRIBUTE_4UB */ p1_color = P1_COLOR_4UB_RGBA; p1_has_color = true; break;
            case VERTEX_ATTRIBUTE_4UB_BGRA: p1_color = P1_COLOR_4UB_BGRA; p1_has_color = true; break;
            default: break;   /* unsupported colour format: treated as absent */
        }
    }
    Pass1Args p1_args;
    p1_args.pos_stride = stride;
    p1_args.uv = (const uint8_t*) P1_ZERO_UV;
    p1_args.uv_stride = uv_offset ? stride : 0;
    p1_args.col = (const uint8_t*) P1_ONE_COLOR;
    p1_args.col_stride = p1_has_color ? stride : 0;
    for(int k = 0; k < 4; ++k) {
        p1_args.base[k] = (p1_has_color && color_replaces_base) ? 1.0f : mat_base_color_[k];
    }
    p1_args.uvm = (uv_offset && !uv_matrix_identity_) ? uv_matrix_ : nullptr;
    p1_args.lit = nullptr;

    /* The asm pass 1 covers 4F or absent colour without a UV matrix. */
    const bool p1_asm = (!p1_has_color || p1_color == P1_COLOR_4F) && !p1_args.uvm;
    Pass1AsmArgs p1_asm_args;
    p1_asm_args.uv = (const uint8_t*) P1_ZERO_UV;
    p1_asm_args.col = (const uint8_t*) P1_ONE_COLOR;
    p1_asm_args.lit = P1_UNLIT_ROW;
    p1_asm_args.pos_step = int32_t(stride) - 8;
    p1_asm_args.uv_step = int32_t(p1_args.uv_stride) - 4;
    p1_asm_args.col_step = int32_t(p1_args.col_stride) - 12;
    p1_asm_args.lit_step = -24;
    for(int k = 0; k < 4; ++k) p1_asm_args.material[k] = p1_args.base[k];
    p1_asm_args.limit = 1e36f;

    /* The next window's source rows, prefetched a slice at a time between
     * this window's kernels (see transform_batch): the passes' compute hides
     * the line fills, which otherwise stalled the geometry pass ~1.3 times
     * a vertex. PREF never faults, so the slices needn't stop exactly at the
     * end of the batch's data. */
    const uint8_t* prefetch_cur = nullptr;
    const uint8_t* prefetch_end = nullptr;
    auto prefetch_rows = [&](uint32_t lines) {
        for(; lines && prefetch_cur < prefetch_end; --lines, prefetch_cur += 32) {
            SHZ_PREFETCH(prefetch_cur);
        }
    };

    auto transform_window = [&](uint32_t base, uint32_t first, uint32_t count) {
        if(!count) return;

        /* Compact the enabled lights and fold per-light constants. */
        PackedLight packed[MAX_LIGHTS];
        int light_count = 0;
        if(lighting_enabled) {
            for(int li = 0; li < MAX_LIGHTS; ++li) {
                const VertexLightState& ls = lights_[li];
                if(!ls.enabled) continue;
                PackedLight& pl = packed[light_count++];
                pl.point = (ls.position[3] >= 0.5f);
                const float* v = pl.point ? ls.position : ls.dir;
                pl.v[0] = v[0]; pl.v[1] = v[1]; pl.v[2] = v[2];
                pl.col[0] = ls.color[0] * ls.intensity;
                pl.col[1] = ls.color[1] * ls.intensity;
                pl.col[2] = ls.color[2] * ls.intensity;
                pl.inv_range = ls.inv_range;
            }
        }

        /* ------------------------------------------------------------
         * Per-vertex PBR lighting (light_vertices). Loads the modelview
         * into xmtrx so the normal and eye-space position transforms can
         * also use FTRV, and restores MVP afterwards.
         * ------------------------------------------------------------ */
        auto light_window = [&]() {
            shz_xmtrx_load_4x4(&light_modelview);

            const float roughness_alpha = mat_roughness_ * mat_roughness_;
            LightingParams lp;
            lp.lights     = packed;
            lp.ambient[0] = ambient_[0];
            lp.ambient[1] = ambient_[1];
            lp.ambient[2] = ambient_[2];
            lp.metallic   = mat_metallic_;
            lp.nm         = 1.0f - mat_metallic_;
            lp.a2         = roughness_alpha * roughness_alpha;

            /* Blinn-Phong exponent matching the GGX alpha (Walter et al.
             * 2007: n = 2 / alpha^2 - 2). With the engine's pi-folded
             * convention the normalised lobe is (n + 2) / 8, which peaks at
             * 1 / a2 - the same as GGX's D at N.H = 1. */
            float bp_n = 2.0f / (lp.a2 + 1e-6f) - 2.0f;
            bp_n = (bp_n < 1.0f) ? 1.0f : ((bp_n > 2048.0f) ? 2048.0f : bp_n);
            lp.bp_n = bp_n;
            lp.bp_nm1 = bp_n - 1.0f;
            lp.bp_scale = (bp_n + 2.0f) * 0.125f;
            /* Dielectrics apply F0 = 0.04 by running the specular weights
             * through the same 0.96-scaled colour matrix as diffuse (see the
             * combine pass), so pre-scale by 0.04 / 0.96 here. */
            if(mat_metallic_ == 0.0f) {
                lp.bp_scale *= 0.04f / 0.96f;
            }

            /* Specialised on light count (so light slots unroll and dead
             * lanes drop out of the register allocation) and on dielectric
             * materials (metallic == 0, the common case), whose constant F0
             * of 0.04 makes Fresnel scalar per light. */
            static_assert(MAX_LIGHTS == 3, "Update the light_vertices dispatch");
            const uint8_t* row = raw_data + stride * (base + first);
            const bool dielectric = (mat_metallic_ == 0.0f);
            if(light_count == 1) {
                if(dielectric) {
                    light_vertices<true, 1>(lp, cv_window_, row, count,
                                            stride, pos_offset, normal_offset);
                } else {
                    light_vertices<false, 1>(lp, cv_window_, row, count,
                                             stride, pos_offset, normal_offset);
                }
            } else if(light_count == 2) {
                if(dielectric) {
                    light_vertices<true, 2>(lp, cv_window_, row, count,
                                            stride, pos_offset, normal_offset);
                } else {
                    light_vertices<false, 2>(lp, cv_window_, row, count,
                                             stride, pos_offset, normal_offset);
                }
            } else {
                if(dielectric) {
                    light_vertices<true, 3>(lp, cv_window_, row, count,
                                            stride, pos_offset, normal_offset);
                } else {
                    light_vertices<false, 3>(lp, cv_window_, row, count,
                                             stride, pos_offset, normal_offset);
                }
            }

            shz_xmtrx_load_4x4((shz_mat4x4_t*) mvp._native());
        };

        /* Dielectrics are lit before pass 1: their combine leaves the lit
         * colour factor and specular in the scratch rows, and pass 1 applies
         * them as it writes each ClipVertex, so no ClipVertex is written
         * twice or read back (reading them back missed the cache about once
         * a vertex). Metals need the base colour for Fresnel, so they are
         * lit afterwards, straight into cv. */
        const bool light_first = lighting_enabled && light_count > 0 && mat_metallic_ == 0.0f;
        if(light_first) {
            light_window();
        }
        prefetch_rows(16);

        /* ------------------------------------------------------------
         * Pass 1: position × MVP (FTRV), UVs, and base colour (material ×
         * per-vertex colour) in a single sweep over the vertex rows.
         *
         * Position transform uses the MVP already loaded in xmtrx; the UV /
         * colour work needs no matrix, so both fit in one loop. Merging them
         * halves the number of passes over the source vertex buffer — on the
         * SH4 the vertex data is the dominant memory-bandwidth cost, and for
         * a batch larger than the D-cache the rows would otherwise be re-read
         * from RAM on the second pass. For dielectrics the lighting above has
         * already run and the lit colour is applied here; otherwise the
         * *base* colour is stashed in cv.r/g/b and any lighting below reads
         * it back out and replaces it.
         * ------------------------------------------------------------ */
        {
            const uint8_t* row = raw_data + stride * (base + first);
            Pass1Args a = p1_args;
            a.pos = row + pos_offset;
            if(uv_offset) a.uv = row + uv_offset;
            if(p1_has_color) a.col = row + color_offset;
            a.lit = light_first ? lit_window_[0] : nullptr;
            ClipVertex* out = cv_window_;
            if(p1_asm) {
                Pass1AsmArgs aa = p1_asm_args;
                aa.pos = a.pos;
                if(uv_offset) aa.uv = a.uv;
                if(p1_has_color) aa.col = a.col;
                if(a.lit) {
                    aa.lit = a.lit + LV_DR;
                    aa.lit_step = LV_STRIDE * sizeof(float) - 24;
                }
                aa.out = out;
                aa.n = count;


                if(pvr_pass1_sh4(&aa)) {
                    for(uint32_t i = 0; i < count; ++i) {
                        if(!out[i].ok) {
                            out[i].x = 0.0f; out[i].y = 0.0f; out[i].z = 0.0f; out[i].w = -1.0f;
                        }
                    }
                }
            } else {
                pass1(a, p1_color, out, count);
            }
        }

        if(lighting_enabled) {
            if(light_count == 0) {
                /* Ambient only. Specular was already zeroed by pass 1, and
                 * xmtrx still holds MVP. */
                for(uint32_t i = 0; i < count; ++i) {
                    ClipVertex& cv = cv_window_[i];
                    cv.r *= ambient_[0];
                    cv.g *= ambient_[1];
                    cv.b *= ambient_[2];
                }
            } else if(!light_first) {
                light_window();
            }
        }

        prefetch_rows(16);

        alignas(8) static float pk_consts[4];
        pk_consts[0] = hw; pk_consts[1] = hh; pk_consts[2] = 127.5f; pk_consts[3] = -127.5f;
        pvr_pack_sh4(cv_window_, &work_[first], count, pk_consts);

        /* pvr_pack_sh4 always writes 1/w as the depth, which is the same
         * for every vertex of an ortho draw; replace it (see pvr_ortho_depth).
         * The window's ClipVertex slots are still in the cache. */
        if(ortho) {
            for(uint32_t i = 0; i < count; ++i) {
                work_[first + i].z = pvr_ortho_depth(cv_window_[i].z);
            }
        }
    };

    /* Pass 1 (clip-space position, UV, base colour) and the lighting pass
     * both walk the batch's vertices in order, so doing them a window at a
     * time is equivalent to doing each over the whole batch - but keeps the
     * window's source rows and ClipVertex slots in the operand cache between
     * the two. Over a whole large batch (a level chunk is ~1800 vertices,
     * ~100KB of ClipVertex alone) both were long evicted by the time the
     * lighting pass reached them, costing ~26% of the lighting kernel in
     * dcache stalls. The per-window setup it repeats is a few dozen
     * instructions per 64 vertices. */
    /* Transform a contiguous batch of `count` vertices starting at source
     * vertex `base`. On entry MVP must be loaded into xmtrx, and still is on
     * return. On exit work_[i] holds source vertex base + i, packed for
     * submission. */
    uint32_t work_base = 0;     /* source vertex of work_[0], for cv_at */
    /* When set, work_[i] holds source vertex work_gather[i] instead (see
     * emit_list's gather) */
    const uint32_t* work_gather = nullptr;
    /* always_inline: with the oversized-batch helpers' extra callers GCC
     * otherwise outlines this (and the transform_window inlined in it),
     * which measured ~13% slower over the benchmark's frame. */
    auto transform_batch = [&](uint32_t base, uint32_t count) __attribute__((always_inline)) {
        work_base = base;
        /* emit_list and friends never ask for more (bigger lists go in
         * chunks); clamp defensively rather than overrunning the array. */
        if(count > WORK_VERTEX_CAPACITY) {
            count = WORK_VERTEX_CAPACITY;
        }
        static const uint32_t XFORM_WINDOW = 32;
        for(uint32_t first = 0; first < count; first += XFORM_WINDOW) {
            const uint32_t n = (count - first < XFORM_WINDOW) ?
                (count - first) : XFORM_WINDOW;
            const uint32_t rest = count - first - n;
            const uint32_t next = (rest < XFORM_WINDOW) ? rest : XFORM_WINDOW;
            prefetch_cur = raw_data + stride * (base + first + n);
            prefetch_end = prefetch_cur + stride * next;
            transform_window(base, first, n);
            prefetch_rows(~0u);   /* whatever the slices didn't reach */
        }
    };

    /* MVP is loaded once here; transform_batch keeps it loaded across calls. */
    shz_xmtrx_load_4x4((shz_mat4x4_t*) mvp._native());

    /* Emit a single screen-space vertex (viewport transform + perspective
     * divide already applied) into the active PVR list.
     * For OP: submits directly via store queues (64-byte Type 5 format).
     * For PT/TR: appends the vertex to the deferred buffer.
     *
     * Kept separate from the clip-space path so the line expansion below can
     * build its width in screen space and emit through the same sink. */
    auto emit_vertex = [&](float sx, float sy, float sz,
                           float u, float v,
                           float r, float g, float b, float a,
                           float sr, float sg, float sb,
                           bool is_last) {
        pvr_vertex_packed_t vert;
        vert.flags = is_last ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
        vert.x = sx;
        vert.y = sy;
        vert.z = sz;
        vert.u = u;
        vert.v = v;

        /* Clamp before quantizing: the PVR's color clamp only applies to
         * float-color vertices, and a value >1 would wrap in the uint8
         * conversion (e.g. 1.2 -> 50) producing wildly wrong colors.
         * Lighting/specular routinely exceed 1.0. */
        vert.argb = PVR_PACK_COLOR(
            pvr_clamp01(a), pvr_clamp01(r), pvr_clamp01(g), pvr_clamp01(b));

        /* Offset alpha is ignored by the PVR's oargb unit — only rgb is
         * added to the post-texture-modulate color. */
        vert.oargb = PVR_PACK_COLOR(
            1.0f, pvr_clamp01(sr), pvr_clamp01(sg), pvr_clamp01(sb));

        /* All lists (including OP_POLY) share submit_bytes, so only a list
         * that has been promoted is ever opened mid-frame. See the promotion
         * comment above. */
        submit_bytes(&vert, sizeof(vert));
    };

    /* Viewport transform + perspective divide of a clip-space vertex.
     * Returns screen-space x/y and the 1/w depth the PVR expects. */
    auto clip_to_screen = [&](const ClipVertex& cv,
                              float& sx, float& sy, float& sz) {
        /* Apply viewport transform (done before perspective divide for PVR).
         * Factored to 1 add + 1 mul per axis rather than 2 muls + 1 add. */
        float vx = hw * (cv.x + cv.w);
        float vy = hh * (cv.w - cv.y);

        /* Clamp rather than only special-casing exactly zero: a substituted or
         * interpolated vertex can land on a small or negative w, and 1/w for
         * those produces the huge (or sign-flipped) screen coordinates the TA
         * chokes on. Matches the modifier path's to_screen(). */
        float w = cv.w;
        if(w < FLT_EPSILON) w = FLT_EPSILON;
        float inv_w = shz_invf_fsrra(w); /* w >= FLT_EPSILON > 0 */

        sx = vx * inv_w;
        sy = vy * inv_w;
        sz = ortho ? pvr_ortho_depth(cv.z) : inv_w;
    };

    /* Lambda to do perspective divide and emit a ClipVertex. */
    auto submit_clip_vertex = [&](const ClipVertex& cv, bool is_last) {
        float sx, sy, sz;
        clip_to_screen(cv, sx, sy, sz);
        emit_vertex(sx, sy, sz, cv.u, cv.v, cv.r, cv.g, cv.b, cv.a,
                    cv.sr, cv.sg, cv.sb, is_last);
    };

    const bool direct_list = renderer_->is_list_direct(renderer_->current_list_type_);

    /* Lambda to process a triangle with near-plane clipping */
    auto process_triangle = [&](const ClipVertex& v0, const ClipVertex& v1,
                                const ClipVertex& v2, bool is_last_tri) {
        /* Check visibility of each vertex (z >= -w means in front of near plane) */
        bool vis0 = is_vertex_visible(v0);
        bool vis1 = is_vertex_visible(v1);
        bool vis2 = is_vertex_visible(v2);
        int visible_mask = (vis0 ? 1 : 0) | (vis1 ? 2 : 0) | (vis2 ? 4 : 0);

        /* A poisoned vertex always tests as invisible, so it can only reach the
         * TA via a clip interpolation — drop the whole primitive instead. Only
         * the partially-visible masks need checking: mask 7 can't contain one,
         * and mask 0 emits nothing, which keeps the fully-visible fast path and
         * the fully-culled path free of the test. emit_strip, process_quad and
         * process_line all funnel their clipped cases through here, so this is
         * the only place the check is needed. */
        if(visible_mask != 7 && visible_mask != 0 &&
           !(v0.ok && v1.ok && v2.ok)) {
            return;
        }

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

    /* Lambda to render a quad (4 corners in GL_QUADS perimeter order) as a
     * single 4-vertex triangle strip.  A strip [v0,v1,v3,v2] draws the two
     * triangles (v0,v1,v3) and (v1,v2,v3) — a valid triangulation of the quad
     * across the v1–v3 diagonal, with the PVR flipping the second triangle's
     * winding automatically so no per-triangle winding fixup is needed.  When
     * any corner is behind the near plane we fall back to two independently
     * clipped triangles matching GL_QUADS' (v0,v1,v2)+(v0,v2,v3) split. */
    auto process_quad = [&](const ClipVertex& v0, const ClipVertex& v1,
                            const ClipVertex& v2, const ClipVertex& v3) {
        if(is_vertex_visible(v0) && is_vertex_visible(v1) &&
           is_vertex_visible(v2) && is_vertex_visible(v3)) {
            submit_clip_vertex(v0, false);
            submit_clip_vertex(v1, false);
            submit_clip_vertex(v3, false);
            submit_clip_vertex(v2, true);
        } else {
            process_triangle(v0, v1, v2, true);
            process_triangle(v0, v2, v3, true);
        }
    };

    /* Lambda to render a line segment as a screen-space-aligned quad (a
     * 4-vertex triangle strip).  The PVR has no line primitive, so each
     * segment is expanded to a constant-pixel-width strip: the perpendicular
     * is computed in screen space after the perspective divide, giving the
     * same uniform width GL's glLineWidth would.  Segments touching the near
     * plane are dropped — lines are thin outlines and full near-plane line
     * clipping isn't worth the hot-path cost. */
    const float line_half_width = material_pass->line_width() * 0.5f;
    auto process_line = [&](const ClipVertex& c0, const ClipVertex& c1) {
        if(!is_vertex_visible(c0) || !is_vertex_visible(c1)) {
            return;
        }

        float sx0, sy0, sz0, sx1, sy1, sz1;
        clip_to_screen(c0, sx0, sy0, sz0);
        clip_to_screen(c1, sx1, sy1, sz1);

        float dx = sx1 - sx0;
        float dy = sy1 - sy0;
        float len_sq = dx * dx + dy * dy;
        if(len_sq < 1e-8f) {
            return; /* degenerate zero-length segment */
        }
        /* Unit perpendicular in screen space, scaled to the half width. */
        float inv_len = shz_inv_sqrtf_fsrra(len_sq);
        float nx = -dy * inv_len * line_half_width;
        float ny =  dx * inv_len * line_half_width;

        /* Lines never carry specular — flat vertex colour only. */
        emit_vertex(sx0 + nx, sy0 + ny, sz0, c0.u, c0.v,
                    c0.r, c0.g, c0.b, c0.a, 0.0f, 0.0f, 0.0f, false);
        emit_vertex(sx0 - nx, sy0 - ny, sz0, c0.u, c0.v,
                    c0.r, c0.g, c0.b, c0.a, 0.0f, 0.0f, 0.0f, false);
        emit_vertex(sx1 + nx, sy1 + ny, sz1, c1.u, c1.v,
                    c1.r, c1.g, c1.b, c1.a, 0.0f, 0.0f, 0.0f, false);
        emit_vertex(sx1 - nx, sy1 - ny, sz1, c1.u, c1.v,
                    c1.r, c1.g, c1.b, c1.a, 0.0f, 0.0f, 0.0f, true);
    };

    /* A batch vertex as a ClipVertex, for the clipping path: clip position
     * worked out again from the source vertex (as pass 1 does, including
     * its treatment of non-finite positions), UV and colours from the packed
     * vertex (so already clamped to [0, 1], which interpolating and
     * re-packing is unaffected by). */
    auto cv_at = [&](uint32_t i) __attribute__((noinline, cold)) -> ClipVertex {
        const uint32_t src = work_gather ? work_gather[i] : work_base + i;
        const float* sp = (const float*)(raw_data + stride * src + pos_offset);
        const Vec4 c = mvp * Vec4(sp[0], sp[1], sp[2], 1.0f);
        const pvr_vertex_packed_t& p = work_[i];
        const float k = 1.0f / 255.0f;
        ClipVertex v;
        v.ok = is_clip_position_valid(c.x, c.y, c.z, c.w);
        if(v.ok) {
            v.x = c.x; v.y = c.y; v.z = c.z; v.w = c.w;
        } else {
            v.x = 0.0f; v.y = 0.0f; v.z = 0.0f; v.w = -1.0f;
        }
        v.u = p.u; v.v = p.v;
        v.a = float((p.argb >> 24) & 255) * k;
        v.r = float((p.argb >> 16) & 255) * k;
        v.g = float((p.argb >> 8) & 255) * k;
        v.b = float(p.argb & 255) * k;
        v.sr = float((p.oargb >> 16) & 255) * k;
        v.sg = float((p.oargb >> 8) & 255) * k;
        v.sb = float(p.oargb & 255) * k;
        return v;
    };

    /* emit_triangles' / emit_strip's clipping path and staging growth. */
    auto emit_slow_triangle = [&](uint32_t i0, uint32_t i1, uint32_t i2) {
        process_triangle(cv_at(i0), cv_at(i1), cv_at(i2), true);
    };
    auto emit_grow = [&]() { stage_grow(96); };

    /* Fans, quads, lines and line strips: all through the (ClipVertex)
     * clipping path, which is fine for how rarely they're used. Out of line
     * and cold so they don't bulk out do_visit, whose common path has to
     * share the SH4's 8KB instruction cache with the rest of the frame.
     * `index(k)` is the work-array index of the k-th of `n` vertices. */
    auto emit_other_arrangement = [&](auto index, std::size_t n) __attribute__((noinline, cold)) {
        switch(renderable->arrangement) {
            case MESH_ARRANGEMENT_TRIANGLE_FAN:
                if(n >= 3) {
                    const ClipVertex v0 = cv_at(index(0));
                    for(std::size_t i = 1; i + 1 < n; i++) {
                        process_triangle(v0, cv_at(index(i)), cv_at(index(i + 1)), true);
                    }
                }
                break;
            case MESH_ARRANGEMENT_QUADS:
                for(std::size_t i = 0; i + 3 < n; i += 4) {
                    process_quad(cv_at(index(i + 0)), cv_at(index(i + 1)),
                                 cv_at(index(i + 2)), cv_at(index(i + 3)));
                }
                break;
            case MESH_ARRANGEMENT_LINES:
                for(std::size_t i = 0; i + 1 < n; i += 2) {
                    process_line(cv_at(index(i + 0)), cv_at(index(i + 1)));
                }
                break;
            case MESH_ARRANGEMENT_LINE_STRIP:
                for(std::size_t i = 1; i < n; i++) {
                    process_line(cv_at(index(i - 1)), cv_at(index(i)));
                }
                break;
            default:
                break;
        }
    };

    /*
     * Submit geometry with near-plane clipping.
     *
     * Both paths first run transform_batch over the source vertex range
     * that they need, then walk the topology referencing the packed
     * vertices in work_.
     */

    /* stage_open: a RAM-staged list gets room for the expected vertex count
     * up front (stage_next grows it if clipping produces more), and is
     * trimmed back to what was written on the way out. */
    struct StageClose {
        PVRRenderer::StagingBuffer*& buf;
        uint8_t*& cur;
        ~StageClose() {
            if(buf) buf->resize(cur - reinterpret_cast<uint8_t*>(buf->data()));
        }
    } _stage_close{stage_buf, stage_cur};

    if(!direct_list) {
        std::size_t prim_verts = 0;
        if(renderable->index_element_count > 0 && renderable->index_data) {
            prim_verts = renderable->index_element_count;
        } else {
            for(std::size_t ri = 0; ri < renderable->vertex_range_count; ++ri) {
                prim_verts += renderable->vertex_ranges[ri].count;
            }
        }
        std::size_t vfactor;
        switch(renderable->arrangement) {
            case MESH_ARRANGEMENT_LINES:
            case MESH_ARRANGEMENT_LINE_STRIP:    vfactor = 4; break;
            case MESH_ARRANGEMENT_TRIANGLE_FAN:  vfactor = 6; break;
            case MESH_ARRANGEMENT_QUADS:         vfactor = 3; break;
            default:                             vfactor = 2; break;
        }
        auto& buf = renderer_->buffer(renderer_->current_list_type_)
                        .buffers[renderer_->current_buffer_index_];
        const std::size_t used = buf.size();
        buf.resize(used + (vfactor * prim_verts + 8) * sizeof(pvr_vertex_packed_t));
        stage_buf = &buf;
        uint8_t* base = reinterpret_cast<uint8_t*>(buf.data());
        stage_cur = base + used;
        stage_end = base + buf.size();
    }

    /* A batch is transformed into work_, which holds WORK_VERTEX_CAPACITY
     * vertices, so a primitive list whose vertices span more than that is
     * drawn a chunk at a time, each chunk's span transformed on its own -
     * by these, out of line and cold so the common path below stays as it
     * was. `src(k)` is the source vertex of the list's k-th element. */

    /* Triangles [0, ntris) of a triangle list. A chunk whose vertices span
     * too much is halved until it fits; a single triangle whose vertices
     * are too far apart for any batch has them gathered into work_[0..2]
     * one at a time. */
    auto emit_list = [&](auto src, std::size_t ntris) __attribute__((noinline, cold)) {
        std::size_t t = 0, chunk = ntris;
        while(t < ntris) {
            const std::size_t n = (chunk < ntris - t) ? chunk : ntris - t;
            uint32_t lo = 0xFFFFFFFFu, hi = 0;
            for(std::size_t k = t * 3; k < (t + n) * 3; ++k) {
                const uint32_t v = src(k);
                if(v < lo) lo = v;
                if(v > hi) hi = v;
            }
            if(hi - lo < WORK_VERTEX_CAPACITY) {
                transform_batch(lo, hi - lo + 1);
                const std::size_t k0 = t * 3;
                emit_triangles([&](std::size_t k) -> uint32_t { return src(k0 + k) - lo; },
                               n, work_, direct_list,
                               stage_cur, stage_end, emit_slow_triangle, emit_grow);
                t += n;
            } else if(n > 1) {
                chunk = n / 2;
            } else {
                /* Each vertex transformed on its own (into work_[0]) and
                 * set aside, through transform_batch so that transform_window
                 * keeps its single caller. */
                uint32_t gathered[3];
                pvr_vertex_packed_t packed[3];
                for(uint32_t c = 0; c < 3; ++c) {
                    gathered[c] = src(t * 3 + c);
                    transform_batch(gathered[c], 1);
                    packed[c] = work_[0];
                }
                for(uint32_t c = 0; c < 3; ++c) work_[c] = packed[c];
                work_gather = gathered;
                emit_triangles([](std::size_t k) -> uint32_t { return uint32_t(k); },
                               1, work_, direct_list,
                               stage_cur, stage_end, emit_slow_triangle, emit_grow);
                work_gather = nullptr;
                t += 1;
            }
        }
    };

    /* A triangle strip of `count` elements, as chunks that overlap by the two
     * vertices the next triangle shares. emit_strip winds each chunk as a
     * strip starting at an even position, so chunks start at even ones; a
     * triangle that would leave the next start odd goes out on its own, with
     * the odd-position winding (its first two vertices swapped). */
    auto emit_strip_list = [&](auto src, std::size_t count) __attribute__((noinline, cold)) {
        if(count < 3) return;
        std::size_t p = 0, chunk = count;
        while(p + 2 < count) {
            if(p & 1) {
                const uint32_t tri[3] = {src(p + 1), src(p), src(p + 2)};
                emit_list([&tri](std::size_t k) -> uint32_t { return tri[k]; }, 1);
                p += 1;
                continue;
            }
            std::size_t n = (chunk < count - p) ? chunk : count - p;
            if(p + n < count && (n & 1)) --n;      /* the next chunk starts even */
            if(n < 3) n = 3;
            uint32_t lo = 0xFFFFFFFFu, hi = 0;
            for(std::size_t k = p; k < p + n; ++k) {
                const uint32_t v = src(k);
                if(v < lo) lo = v;
                if(v > hi) hi = v;
            }
            if(hi - lo < WORK_VERTEX_CAPACITY) {
                transform_batch(lo, hi - lo + 1);
                const std::size_t k0 = p;
                emit_strip([&](std::size_t k) -> uint32_t { return src(k0 + k) - lo; },
                           n, work_, direct_list,
                           stage_cur, stage_end, emit_slow_triangle, emit_grow);
                p += n - 2;
            } else if(n > 3) {
                chunk = n / 2;
            } else {
                const uint32_t tri[3] = {src(p), src(p + 1), src(p + 2)};
                emit_list([&tri](std::size_t k) -> uint32_t { return tri[k]; }, 1);
                p += 1;
            }
        }
    };

    /* Fans, quads and lines aren't chunked: a span over the capacity is cut
     * short rather than read past work_. */
    auto emit_other_clamped = [&](auto src, std::size_t n, uint32_t lo, uint32_t span) __attribute__((noinline, cold)) {
        if(span > WORK_VERTEX_CAPACITY) {
            S_WARN_ONCE("PVR: a fan/quad/line batch spans {0} vertices, more than {1}; "
                        "drawing only part of it", span, WORK_VERTEX_CAPACITY);
            span = WORK_VERTEX_CAPACITY;
        }
        transform_batch(lo, span);
        std::size_t m = n;
        while(m && src(m - 1) - lo >= span) --m;
        emit_other_arrangement([&](std::size_t k) -> uint32_t {
            const uint32_t v = src(k) - lo;
            return v < span ? v : 0;
        }, m);
    };

    if(renderable->index_element_count > 0 && renderable->index_data) {
        /* Indexed rendering. */
        const auto* idata = renderable->index_data;
        auto itype = idata->index_type();
        auto icount = renderable->index_element_count;
        const uint8_t* index_ptr = idata->data();

        /* Fast path for the common case: 16-bit independent triangles (the
         * level and most meshes). Resolving the index width once here avoids
         * branching on the index type for every index fetched below. */
        if(itype == INDEX_TYPE_16_BIT &&
           renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
            const uint16_t* idx = (const uint16_t*) index_ptr;
            if(icount == 0) return;

            uint32_t base = 0xFFFFFFFFu, high = 0;
            for(std::size_t i = 0; i < icount; ++i) {
                const uint32_t v = idx[i];
                if(v < base) base = v;
                if(v > high) high = v;
            }
            if(high - base >= WORK_VERTEX_CAPACITY) {
                emit_list([idx](std::size_t k) -> uint32_t { return idx[k]; }, icount / 3);
            } else {
                transform_batch(base, high - base + 1);
                emit_triangles([idx, base](std::size_t k) -> uint32_t { return idx[k] - base; },
                               icount / 3, work_, direct_list,
                               stage_cur, stage_end, emit_slow_triangle, emit_grow);
            }
        } else {
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

        if(high - base >= WORK_VERTEX_CAPACITY) {
            if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
                emit_list(get_index, icount / 3);
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
                emit_strip_list(get_index, icount);
            } else {
                emit_other_clamped(get_index, icount, base, high - base + 1);
            }
        } else {
        transform_batch(base, high - base + 1);

        if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
            emit_triangles([&](std::size_t k) -> uint32_t { return get_index(k) - base; },
                           icount / 3, work_, direct_list,
                           stage_cur, stage_end, emit_slow_triangle, emit_grow);
        } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
            if(icount >= 3) {
                emit_strip([&](std::size_t k) -> uint32_t { return get_index(k) - base; },
                           icount, work_, direct_list,
                           stage_cur, stage_end, emit_slow_triangle, emit_grow);
            }
        } else {
            emit_other_arrangement([&](std::size_t k) -> uint32_t { return get_index(k) - base; },
                                   icount);
        }
        } /* end batch within capacity */
        } /* end generic indexed arrangements */
    } else {
        /* Non-indexed range-based rendering. Ranges are independent
         * primitives, but they're often many small, adjacent ones (a particle
         * system submits a 4-vertex strip per particle): when they pack
         * densely into one span that fits the work array, transform the span
         * once and emit each range from it, rather than paying the transform
         * kernels' per-call setup for every range. Otherwise each range is
         * its own batch. */
        const VertexRange* ranges = renderable->vertex_ranges;
        std::size_t range_count = renderable->vertex_range_count;

        uint32_t lo = 0xFFFFFFFFu, hi = 0, used = 0;
        for(std::size_t ri = 0; ri < range_count; ++ri) {
            const uint32_t count = ranges[ri].count;
            if(!count) continue;
            const uint32_t start = ranges[ri].start;
            if(start < lo) lo = start;
            if(start + count > hi) hi = start + count;
            used += count;
        }
        const bool one_batch = used && range_count > 1 &&
                               hi - lo <= WORK_VERTEX_CAPACITY &&
                               used * 2 >= hi - lo;
        if(one_batch) {
            transform_batch(lo, hi - lo);
        }

        for(std::size_t ri = 0; ri < range_count; ++ri) {
            uint32_t start = ranges[ri].start;
            uint32_t count = ranges[ri].count;
            if(!count) continue;

            if(count > WORK_VERTEX_CAPACITY) {
                auto src = [start](std::size_t k) -> uint32_t { return start + uint32_t(k); };
                if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
                    emit_list(src, count / 3);
                } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
                    emit_strip_list(src, count);   /* each range is its own strip */
                } else {
                    emit_other_clamped(src, count, start, count);
                }
                continue;
            }

            uint32_t first = 0;   /* work_ index of the range's first vertex */
            if(one_batch) {
                first = start - lo;
            } else {
                transform_batch(start, count);
            }

            if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
                emit_triangles([first](std::size_t k) -> uint32_t { return first + uint32_t(k); },
                               count / 3, work_, direct_list,
                               stage_cur, stage_end, emit_slow_triangle, emit_grow);
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
                if(count >= 3) {
                    /* Each range is an independent strip. */
                    emit_strip([first](std::size_t k) -> uint32_t { return first + uint32_t(k); },
                               count, work_, direct_list,
                               stage_cur, stage_end, emit_slow_triangle, emit_grow);
                }
            } else {
                emit_other_arrangement([first](std::size_t k) -> uint32_t { return first + uint32_t(k); },
                                       count);
            }
        }
    }

    /* Report polygon throughput for the stats panel / profiling, matching the
     * GL renderers. Uses the same element/range accounting: for indexed
     * geometry the index count, otherwise the summed vertex-range counts. */
    {
        uint32_t elements = 0;
        if(renderable->index_element_count) {
            elements = (uint32_t) renderable->index_element_count;
        } else {
            for(std::size_t i = 0; i < renderable->vertex_range_count; ++i) {
                elements += renderable->vertex_ranges[i].count;
            }
        }
        if(elements) {
            polygons_rendered_ += StatsRecorder::polygon_count(
                renderable->arrangement, elements);
        }
    }

#else
    _S_UNUSED(material_pass);
#endif
}

} // namespace smlt
