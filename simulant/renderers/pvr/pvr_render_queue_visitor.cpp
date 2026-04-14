#include "pvr_render_queue_visitor.h"
#include "pvr_renderer.h"

#include "pvr_texture_manager.h"

#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../types.h"
#include "../../vertex_data.h"
#include "../../assets/material.h"
#include "../../utils/pbr.h"

#ifdef __DREAMCAST__
#include <kos.h>
#include <dc/pvr.h>
#include <dc/matrix.h>
#include <dc/fmath.h>
#endif

#include <cmath>
#include <cfloat>

namespace smlt {

/* ========================================================================
 * PVR Vertex type: 64-byte Floating Color format (Polygon Type 5)
 * See Dreamcast Dev.Box System Architecture §3 for details.
 * The TA accepts float ARGB and converts to packed color internally.
 * ======================================================================== */

struct alignas(32) PVRVertex {
    uint32_t flags;
    float x, y, z;   /* screen x, y; z = 1/w */
    float u, v;
    uint32_t _pad0;   /* ignored by TA */
    uint32_t _pad1;   /* ignored by TA */
    float base_a, base_r, base_g, base_b;
    float offset_a, offset_r, offset_g, offset_b;
};

static_assert(sizeof(PVRVertex) == 64, "PVRVertex must be 64 bytes");

/* ========================================================================
 * Near-Z clipping support
 * ======================================================================== */

struct ClipVertex {
    float x, y, z, w;
    float u, v;
    float r, g, b, a;
};

static inline void clip_edge(const ClipVertex& v1, const ClipVertex& v2, ClipVertex& out) {
    float d0 = v1.w + v1.z;
    float d1 = v2.w + v2.z;
    float t = fabsf(d0) / (fabsf(d0) + fabsf(d1));
    float invt = 1.0f - t;

    out.x = invt * v1.x + t * v2.x;
    out.y = invt * v1.y + t * v2.y;
    out.z = invt * v1.z + t * v2.z;
    out.w = invt * v1.w + t * v2.w;
    out.u = invt * v1.u + t * v2.u;
    out.v = invt * v1.v + t * v2.v;
    out.r = invt * v1.r + t * v2.r;
    out.g = invt * v1.g + t * v2.g;
    out.b = invt * v1.b + t * v2.b;
    out.a = invt * v1.a + t * v2.a;

    /* Ensure z doesn't go to zero (would cause div-by-zero in perspective divide) */
    if(out.z < FLT_EPSILON && out.w < FLT_EPSILON) {
        out.z = FLT_EPSILON;
        out.w = FLT_EPSILON;
    }
}

static inline bool vertex_visible(const ClipVertex& v) {
    return v.z >= -v.w;
}

static inline void perspective_divide(ClipVertex& v, PVRVertex& out) {
    if(v.w == 0.0f) v.w = FLT_EPSILON;
    float inv_w = 1.0f / v.w;

    out.x = v.x * inv_w;
    out.y = v.y * inv_w;

    /* PVR uses 1/w for depth */
    if(v.w == 1.0f) {
        /* Orthographic projection */
        out.z = 1.0f / (1.0001f + v.z);
    } else {
        out.z = inv_w;
    }

    out.u = v.u;
    out.v = v.v;
    out._pad0 = 0;
    out._pad1 = 0;
    out.base_a = v.a;
    out.base_r = v.r;
    out.base_g = v.g;
    out.base_b = v.b;
    out.offset_a = 0.0f;
    out.offset_r = 0.0f;
    out.offset_g = 0.0f;
    out.offset_b = 0.0f;
}

/* ========================================================================
 * Constructor
 * ======================================================================== */

PVRRenderQueueVisitor::PVRRenderQueueVisitor(PVRRenderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera) {
}

/* ========================================================================
 * Traversal start/end
 * ======================================================================== */

void PVRRenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue,
                                             uint64_t frame_id,
                                             StageNode* stage_node) {
    _S_UNUSED(queue);
    _S_UNUSED(frame_id);

    /* Reset list tracking */
    for(int i = 0; i < 5; i++) list_opened_[i] = false;
    any_list_opened_ = false;

    /* Get ambient light from the stage if available */
    if(stage_node) {
        /* Default ambient */
        ambient_[0] = 0.2f;
        ambient_[1] = 0.2f;
        ambient_[2] = 0.2f;
        ambient_[3] = 1.0f;
    }
}

void PVRRenderQueueVisitor::end_traversal(const batcher::RenderQueue& queue,
                                           StageNode* stage_node) {
    _S_UNUSED(queue);
    _S_UNUSED(stage_node);

#ifdef __DREAMCAST__
    /* Close any opened lists */
    /* Note: lists are finished implicitly when the next one begins,
     * or when pvr_scene_finish() is called */
#endif
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
    _S_UNUSED(prev);
    pass_ = next;
    if(!next) return;

    /* Convert PBR to traditional material values */
    auto values = pbr_to_traditional(
        next->base_color(),
        next->metallic(),
        next->roughness(),
        next->specular_color(),
        next->specular()
    );

    mat_diffuse_[0] = values.diffuse.r;
    mat_diffuse_[1] = values.diffuse.g;
    mat_diffuse_[2] = values.diffuse.b;
    mat_diffuse_[3] = values.diffuse.a;

    mat_ambient_[0] = values.ambient.r;
    mat_ambient_[1] = values.ambient.g;
    mat_ambient_[2] = values.ambient.b;
    mat_ambient_[3] = values.ambient.a;

    mat_specular_[0] = values.specular.r;
    mat_specular_[1] = values.specular.g;
    mat_specular_[2] = values.specular.b;
    mat_specular_[3] = values.specular.a;

    mat_shininess_ = values.shininess;

    /* Determine PVR list type based on blend mode */
    auto blend = next->blend_func();
    if(blend == BLEND_NONE) {
        /* Check for alpha test (punch-through) */
        if(next->alpha_func() != ALPHA_FUNC_NONE) {
            current_list_type_ = 4; /* PVR_LIST_PT_POLY */
        } else {
            current_list_type_ = 0; /* PVR_LIST_OP_POLY */
        }
    } else {
        current_list_type_ = 2; /* PVR_LIST_TR_POLY */
    }

    /* Cache material state for polygon header construction */
    texturing_enabled_ = (next->textures_enabled() & BASE_COLOR_MAP_ENABLED) != 0;
    depth_test_enabled_ = next->is_depth_test_enabled();
    depth_write_enabled_ = next->is_depth_write_enabled();

#ifdef __DREAMCAST__
    /* Map blend modes to PVR blend factors */
    switch(blend) {
        case BLEND_NONE:
            blend_src_ = PVR_BLEND_ONE;
            blend_dst_ = PVR_BLEND_ZERO;
            break;
        case BLEND_ADD:
            blend_src_ = PVR_BLEND_SRCALPHA;
            blend_dst_ = PVR_BLEND_ONE;
            break;
        case BLEND_ALPHA:
            blend_src_ = PVR_BLEND_SRCALPHA;
            blend_dst_ = PVR_BLEND_INVSRCALPHA;
            break;
        case BLEND_COLOR:
            blend_src_ = PVR_BLEND_DESTCOLOR;
            blend_dst_ = PVR_BLEND_ZERO;
            break;
        case BLEND_MODULATE:
            blend_src_ = PVR_BLEND_DESTCOLOR;
            blend_dst_ = PVR_BLEND_ZERO;
            break;
        case BLEND_ONE_ONE_MINUS_ALPHA:
            blend_src_ = PVR_BLEND_ONE;
            blend_dst_ = PVR_BLEND_INVSRCALPHA;
            break;
        default:
            blend_src_ = PVR_BLEND_ONE;
            blend_dst_ = PVR_BLEND_ZERO;
            break;
    }

    /* Map depth function */
    switch(next->depth_func()) {
        case DEPTH_FUNC_NEVER: depth_func_ = PVR_DEPTHCMP_NEVER; break;
        case DEPTH_FUNC_LESS: depth_func_ = PVR_DEPTHCMP_LESS; break;
        case DEPTH_FUNC_LEQUAL: depth_func_ = PVR_DEPTHCMP_LEQUAL; break;
        case DEPTH_FUNC_EQUAL: depth_func_ = PVR_DEPTHCMP_EQUAL; break;
        case DEPTH_FUNC_GEQUAL: depth_func_ = PVR_DEPTHCMP_GEQUAL; break;
        case DEPTH_FUNC_GREATER: depth_func_ = PVR_DEPTHCMP_GREATER; break;
        case DEPTH_FUNC_ALWAYS: depth_func_ = PVR_DEPTHCMP_ALWAYS; break;
        default: depth_func_ = PVR_DEPTHCMP_GEQUAL; break;
    }

    /* Map cull mode - note PVR culling is in screen space (CW/CCW after transform) */
    switch(next->cull_mode()) {
        case CULL_MODE_NONE: cull_mode_ = PVR_CULLING_NONE; break;
        case CULL_MODE_BACK_FACE: cull_mode_ = PVR_CULLING_CW; break;
        case CULL_MODE_FRONT_FACE: cull_mode_ = PVR_CULLING_CCW; break;
        case CULL_MODE_FRONT_AND_BACK_FACE: cull_mode_ = PVR_CULLING_SMALL; break;
        default: cull_mode_ = PVR_CULLING_CW; break;
    }

    /* Shade model */
    shade_mode_ = (next->shade_model() == SHADE_MODEL_FLAT) ?
        PVR_SHADE_FLAT : PVR_SHADE_GOURAUD;

    /* Fog */
    switch(next->fog_mode()) {
        case FOG_MODE_NONE: fog_type_ = PVR_FOG_DISABLE; break;
        case FOG_MODE_LINEAR: fog_type_ = PVR_FOG_TABLE; break;
        case FOG_MODE_EXP: fog_type_ = PVR_FOG_TABLE; break;
        case FOG_MODE_EXP2: fog_type_ = PVR_FOG_TABLE; break;
        default: fog_type_ = PVR_FOG_DISABLE; break;
    }
#endif
}

void PVRRenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {
    /* Reset all lights */
    for(int i = 0; i < MAX_LIGHTS; i++) {
        lights_[i].enabled = false;
    }

    for(uint8_t i = 0; i < count && i < MAX_LIGHTS; i++) {
        if(!lights[i]) continue;

        auto& state = lights_[i];
        state.enabled = true;

        auto light = lights[i];
        /* Store light position in view space */
        auto pos = camera_->view_matrix() * light->transform->position();
        state.position[0] = pos.x;
        state.position[1] = pos.y;
        state.position[2] = pos.z;
        state.position[3] = (light->light_type() == smlt::LIGHT_TYPE_DIRECTIONAL) ? 0.0f : 1.0f;

        state.color[0] = light->color().r;
        state.color[1] = light->color().g;
        state.color[2] = light->color().b;
        state.color[3] = 1.0f;

        state.intensity = light->intensity();
        state.range = light->range();
    }
}

/* ========================================================================
 * ensure_list_opened - open a PVR list if not already opened
 * ======================================================================== */

void PVRRenderQueueVisitor::ensure_list_opened(int list_type) {
#ifdef __DREAMCAST__
    if(!list_opened_[list_type]) {
        pvr_list_begin(list_type);
        list_opened_[list_type] = true;
        any_list_opened_ = true;
    }
#else
    _S_UNUSED(list_type);
#endif
}

/* ========================================================================
 * visit - main draw call
 * ======================================================================== */

void PVRRenderQueueVisitor::visit(const Renderable* renderable,
                                   const MaterialPass* pass,
                                   batcher::Iteration iteration) {
    do_visit(renderable, pass, iteration);
}

void PVRRenderQueueVisitor::do_visit(const Renderable* renderable,
                                      const MaterialPass* material_pass,
                                      batcher::Iteration iteration) {
    _S_UNUSED(iteration);
    if(!renderable || !material_pass) return;

    renderer_->prepare_to_render(renderable);

#ifdef __DREAMCAST__
    const auto& model = renderable->final_transformation;
    const auto& view = camera_->view_matrix();
    const auto& projection = camera_->projection_matrix();

    /* Build the modelview-projection matrix */
    Mat4 modelview = view * model;
    Mat4 mvp = projection * modelview;

    /* Build viewport transform matrix.
     * PVR expects screen-space coordinates. We bake viewport into the transform. */
    float hw = 320.0f; /* Half-width */
    float hh = 240.0f; /* Half-height */

    /* ================================================================
     * Build polygon header
     * ================================================================ */
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;

    PVRTextureObject* tex_obj = nullptr;
    if(texturing_enabled_ && material_pass->base_color_map()) {
        auto tex_id = material_pass->base_color_map()->_renderer_specific_id();
        if(tex_id) {
            tex_obj = renderer_->texture_manager().bind_texture(tex_id);
        }
    }

    if(tex_obj && tex_obj->in_vram && tex_obj->texture_vram) {
        int filter;
        switch(tex_obj->filter) {
            case TEXTURE_FILTER_BILINEAR:
                filter = PVR_FILTER_BILINEAR;
                break;
            case TEXTURE_FILTER_TRILINEAR:
                filter = PVR_FILTER_TRILINEAR1; /* Best we can do */
                break;
            default:
                filter = PVR_FILTER_NONE;
                break;
        }

        pvr_poly_cxt_txr(&cxt, current_list_type_,
                          tex_obj->format,
                          tex_obj->width, tex_obj->height,
                          (pvr_ptr_t)tex_obj->texture_vram,
                          filter);
    } else {
        pvr_poly_cxt_col(&cxt, current_list_type_);
    }

    cxt.gen.shading = shade_mode_;
    cxt.gen.culling = cull_mode_;
    cxt.gen.fog_type = fog_type_;

    cxt.depth.comparison = depth_func_;
    cxt.depth.write = depth_write_enabled_ ? PVR_DEPTHWRITE_ENABLE : PVR_DEPTHWRITE_DISABLE;

    cxt.blend.src = blend_src_;
    cxt.blend.dst = blend_dst_;

    if(current_list_type_ == 2) { /* TR list needs alpha enabled */
        cxt.gen.alpha = PVR_ALPHA_ENABLE;
        if(tex_obj) {
            cxt.txr.alpha = PVR_TXRALPHA_ENABLE;
        }
    }

    pvr_poly_compile(&hdr, &cxt);

    /* Set Col_Type to Floating Color (1) in bits 5-4 of the Parameter Control Word.
     * KOS doesn't natively support this format, but the TA accepts it and will
     * convert to packed color internally. This allows us to submit 64-byte vertices
     * with float ARGB colors instead of packed 32-bit colors. */
    hdr.cmd = (hdr.cmd & ~(3 << 4)) | (1 << 4);

    ensure_list_opened(current_list_type_);

    /* Submit polygon header */
    pvr_prim(&hdr, sizeof(hdr));

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

    /* Color material mode */
    auto color_mat_mode = material_pass->color_material();

    /* Lambda to read a vertex from the interleaved buffer */
    auto read_vertex = [&](uint32_t index, ClipVertex& cv) {
        const uint8_t* ptr = raw_data + (stride * index);

        /* Position */
        {
            const float* p = (const float*)(ptr + pos_offset);
            float px = p[0], py = p[1], pz = p[2];

            /* Transform by MVP, including viewport baked in */
            cv.x = mvp[0] * px + mvp[4] * py + mvp[8]  * pz + mvp[12];
            cv.y = mvp[1] * px + mvp[5] * py + mvp[9]  * pz + mvp[13];
            cv.z = mvp[2] * px + mvp[6] * py + mvp[10] * pz + mvp[14];
            cv.w = mvp[3] * px + mvp[7] * py + mvp[11] * pz + mvp[15];

            /* Apply viewport transform to x, y (pre-perspective divide) */
            cv.x = cv.x * hw + hw * cv.w;
            cv.y = -cv.y * hh + hh * cv.w; /* Flip Y */
        }

        /* UV */
        if(uv_offset) {
            const float* t = (const float*)(ptr + uv_offset);
            cv.u = t[0];
            cv.v = t[1];
        } else {
            cv.u = 0.0f;
            cv.v = 0.0f;
        }

        /* Color - start with material diffuse */
        cv.r = mat_diffuse_[0];
        cv.g = mat_diffuse_[1];
        cv.b = mat_diffuse_[2];
        cv.a = mat_diffuse_[3];

        /* Read vertex color if present */
        float vert_r = 1.0f, vert_g = 1.0f, vert_b = 1.0f, vert_a = 1.0f;
        if(color_offset) {
            auto attr = spec.color_attribute.get();
            if(attr == VERTEX_ATTRIBUTE_4F) {
                const float* c = (const float*)(ptr + color_offset);
                vert_r = c[0]; vert_g = c[1]; vert_b = c[2]; vert_a = c[3];
            } else if(attr == VERTEX_ATTRIBUTE_3F) {
                const float* c = (const float*)(ptr + color_offset);
                vert_r = c[0]; vert_g = c[1]; vert_b = c[2]; vert_a = 1.0f;
            } else if(attr == VERTEX_ATTRIBUTE_4UB_RGBA || attr == VERTEX_ATTRIBUTE_4UB) {
                const uint8_t* c = ptr + color_offset;
                vert_r = c[0] / 255.0f; vert_g = c[1] / 255.0f;
                vert_b = c[2] / 255.0f; vert_a = c[3] / 255.0f;
            } else if(attr == VERTEX_ATTRIBUTE_4UB_BGRA) {
                const uint8_t* c = ptr + color_offset;
                vert_b = c[0] / 255.0f; vert_g = c[1] / 255.0f;
                vert_r = c[2] / 255.0f; vert_a = c[3] / 255.0f;
            }
        }

        /* Apply color material mode */
        switch(color_mat_mode) {
            case COLOR_MATERIAL_DIFFUSE:
                cv.r = vert_r; cv.g = vert_g; cv.b = vert_b; cv.a = vert_a;
                break;
            case COLOR_MATERIAL_AMBIENT:
                /* Vertex color replaces ambient only; diffuse stays as material */
                break;
            case COLOR_MATERIAL_AMBIENT_AND_DIFFUSE:
                cv.r = vert_r; cv.g = vert_g; cv.b = vert_b; cv.a = vert_a;
                break;
            case COLOR_MATERIAL_NONE:
            default:
                /* Use material color (already set) */
                break;
        }

        /* Simple per-vertex directional lighting if lighting is enabled */
        if(material_pass->is_lighting_enabled() && normal_offset) {
            const float* n_ptr = (const float*)(ptr + normal_offset);
            /* Transform normal to view space */
            float nx = n_ptr[0], ny = n_ptr[1], nz = n_ptr[2];

            float mvn_x = modelview[0]*nx + modelview[4]*ny + modelview[8]*nz;
            float mvn_y = modelview[1]*nx + modelview[5]*ny + modelview[9]*nz;
            float mvn_z = modelview[2]*nx + modelview[6]*ny + modelview[10]*nz;

            /* Normalize */
            float len = sqrtf(mvn_x*mvn_x + mvn_y*mvn_y + mvn_z*mvn_z);
            if(len > 0.0001f) {
                float inv_len = 1.0f / len;
                mvn_x *= inv_len; mvn_y *= inv_len; mvn_z *= inv_len;
            }

            float total_r = mat_ambient_[0] * ambient_[0];
            float total_g = mat_ambient_[1] * ambient_[1];
            float total_b = mat_ambient_[2] * ambient_[2];

            for(int li = 0; li < MAX_LIGHTS; li++) {
                if(!lights_[li].enabled) continue;

                float lx, ly, lz;
                float atten = 1.0f;

                if(lights_[li].position[3] == 0.0f) {
                    /* Directional light */
                    lx = -lights_[li].position[0];
                    ly = -lights_[li].position[1];
                    lz = -lights_[li].position[2];
                    float l_len = sqrtf(lx*lx + ly*ly + lz*lz);
                    if(l_len > 0.0001f) {
                        float inv = 1.0f / l_len;
                        lx *= inv; ly *= inv; lz *= inv;
                    }
                } else {
                    /* Point light - would need vertex position in view space.
                     * For performance, we approximate with the transformed origin. */
                    float vx = modelview[12], vy = modelview[13], vz = modelview[14];
                    lx = lights_[li].position[0] - vx;
                    ly = lights_[li].position[1] - vy;
                    lz = lights_[li].position[2] - vz;
                    float dist = sqrtf(lx*lx + ly*ly + lz*lz);
                    if(dist > 0.0001f) {
                        float inv = 1.0f / dist;
                        lx *= inv; ly *= inv; lz *= inv;
                    }
                    /* Simple attenuation */
                    float range = lights_[li].range;
                    atten = 1.0f - (dist / range);
                    if(atten < 0.0f) atten = 0.0f;
                }

                float ndotl = mvn_x * lx + mvn_y * ly + mvn_z * lz;
                if(ndotl < 0.0f) ndotl = 0.0f;

                float intensity = lights_[li].intensity * atten;
                total_r += cv.r * lights_[li].color[0] * ndotl * intensity;
                total_g += cv.g * lights_[li].color[1] * ndotl * intensity;
                total_b += cv.b * lights_[li].color[2] * ndotl * intensity;
            }

            cv.r = total_r;
            cv.g = total_g;
            cv.b = total_b;

            /* Clamp */
            if(cv.r > 1.0f) cv.r = 1.0f;
            if(cv.g > 1.0f) cv.g = 1.0f;
            if(cv.b > 1.0f) cv.b = 1.0f;
        }
    };

    /* Lambda to submit a single triangle (3 ClipVertices) with near-Z clipping */
    auto submit_triangle = [&](ClipVertex& v0, ClipVertex& v1, ClipVertex& v2) {
        int vis = (vertex_visible(v0) ? 1 : 0) |
                  (vertex_visible(v1) ? 2 : 0) |
                  (vertex_visible(v2) ? 4 : 0);

        if(vis == 0) return; /* All behind near plane */

        PVRVertex pv[4]; /* Max 4 vertices after clipping (quad from 1 triangle) */
        int count = 0;

        if(vis == 7) {
            /* All visible - no clipping needed */
            perspective_divide(v0, pv[0]); pv[0].flags = PVR_CMD_VERTEX;
            perspective_divide(v1, pv[1]); pv[1].flags = PVR_CMD_VERTEX;
            perspective_divide(v2, pv[2]); pv[2].flags = PVR_CMD_VERTEX_EOL;
            count = 3;
        } else if(vis == 1) {
            /* Only v0 visible */
            ClipVertex a, b;
            clip_edge(v0, v1, a);
            clip_edge(v0, v2, b);
            perspective_divide(v0, pv[0]); pv[0].flags = PVR_CMD_VERTEX;
            perspective_divide(a,  pv[1]); pv[1].flags = PVR_CMD_VERTEX;
            perspective_divide(b,  pv[2]); pv[2].flags = PVR_CMD_VERTEX_EOL;
            count = 3;
        } else if(vis == 2) {
            /* Only v1 visible */
            ClipVertex a, b;
            clip_edge(v1, v0, a);
            clip_edge(v1, v2, b);
            perspective_divide(a,  pv[0]); pv[0].flags = PVR_CMD_VERTEX;
            perspective_divide(v1, pv[1]); pv[1].flags = PVR_CMD_VERTEX;
            perspective_divide(b,  pv[2]); pv[2].flags = PVR_CMD_VERTEX_EOL;
            count = 3;
        } else if(vis == 4) {
            /* Only v2 visible */
            ClipVertex a, b;
            clip_edge(v2, v0, a);
            clip_edge(v2, v1, b);
            perspective_divide(a,  pv[0]); pv[0].flags = PVR_CMD_VERTEX;
            perspective_divide(b,  pv[1]); pv[1].flags = PVR_CMD_VERTEX;
            perspective_divide(v2, pv[2]); pv[2].flags = PVR_CMD_VERTEX_EOL;
            count = 3;
        } else if(vis == 3) {
            /* v0, v1 visible; v2 clipped -> produces a quad (4 verts as tristrip) */
            ClipVertex a, b;
            clip_edge(v0, v2, a);
            clip_edge(v1, v2, b);
            perspective_divide(v0, pv[0]); pv[0].flags = PVR_CMD_VERTEX;
            perspective_divide(v1, pv[1]); pv[1].flags = PVR_CMD_VERTEX;
            perspective_divide(a,  pv[2]); pv[2].flags = PVR_CMD_VERTEX;
            perspective_divide(b,  pv[3]); pv[3].flags = PVR_CMD_VERTEX_EOL;
            count = 4;
        } else if(vis == 5) {
            /* v0, v2 visible; v1 clipped -> quad */
            ClipVertex a, b;
            clip_edge(v0, v1, a);
            clip_edge(v2, v1, b);
            perspective_divide(v0, pv[0]); pv[0].flags = PVR_CMD_VERTEX;
            perspective_divide(a,  pv[1]); pv[1].flags = PVR_CMD_VERTEX;
            perspective_divide(v2, pv[2]); pv[2].flags = PVR_CMD_VERTEX;
            perspective_divide(b,  pv[3]); pv[3].flags = PVR_CMD_VERTEX_EOL;
            count = 4;
        } else if(vis == 6) {
            /* v1, v2 visible; v0 clipped -> quad */
            ClipVertex a, b;
            clip_edge(v1, v0, a);
            clip_edge(v2, v0, b);
            perspective_divide(a,  pv[0]); pv[0].flags = PVR_CMD_VERTEX;
            perspective_divide(v1, pv[1]); pv[1].flags = PVR_CMD_VERTEX;
            perspective_divide(b,  pv[2]); pv[2].flags = PVR_CMD_VERTEX;
            perspective_divide(v2, pv[3]); pv[3].flags = PVR_CMD_VERTEX_EOL;
            count = 4;
        }

        /* Submit clipped vertices to PVR */
        for(int i = 0; i < count; i++) {
            pvr_prim(&pv[i], sizeof(PVRVertex));
        }
    };

    /* ================================================================
     * Submit geometry
     * ================================================================ */
    if(renderable->index_element_count > 0 && renderable->index_data) {
        /* Indexed rendering */
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

        if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
            for(std::size_t i = 0; i + 2 < icount; i += 3) {
                ClipVertex v0, v1, v2;
                read_vertex(get_index(i + 0), v0);
                read_vertex(get_index(i + 1), v1);
                read_vertex(get_index(i + 2), v2);
                submit_triangle(v0, v1, v2);
            }
        } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
            for(std::size_t i = 0; i + 2 < icount; i++) {
                ClipVertex v0, v1, v2;
                read_vertex(get_index(i + 0), v0);
                read_vertex(get_index(i + 1), v1);
                read_vertex(get_index(i + 2), v2);
                /* Swap winding for even triangles */
                if(i & 1) {
                    submit_triangle(v0, v2, v1);
                } else {
                    submit_triangle(v0, v1, v2);
                }
            }
        } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) {
            if(icount >= 3) {
                ClipVertex v0;
                read_vertex(get_index(0), v0);
                for(std::size_t i = 1; i + 1 < icount; i++) {
                    ClipVertex v1, v2;
                    read_vertex(get_index(i), v1);
                    read_vertex(get_index(i + 1), v2);
                    submit_triangle(v0, v1, v2);
                }
            }
        }
    } else {
        /* Non-indexed range-based rendering */
        auto range = renderable->vertex_ranges;
        for(std::size_t ri = 0; ri < renderable->vertex_range_count; ++ri, ++range) {
            uint32_t start = range->start;
            uint32_t count = range->count;

            if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
                for(uint32_t i = 0; i + 2 < count; i += 3) {
                    ClipVertex v0, v1, v2;
                    read_vertex(start + i + 0, v0);
                    read_vertex(start + i + 1, v1);
                    read_vertex(start + i + 2, v2);
                    submit_triangle(v0, v1, v2);
                }
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
                for(uint32_t i = 0; i + 2 < count; i++) {
                    ClipVertex v0, v1, v2;
                    read_vertex(start + i + 0, v0);
                    read_vertex(start + i + 1, v1);
                    read_vertex(start + i + 2, v2);
                    if(i & 1) {
                        submit_triangle(v0, v2, v1);
                    } else {
                        submit_triangle(v0, v1, v2);
                    }
                }
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) {
                if(count >= 3) {
                    ClipVertex v0;
                    read_vertex(start, v0);
                    for(uint32_t i = 1; i + 1 < count; i++) {
                        ClipVertex v1, v2;
                        read_vertex(start + i, v1);
                        read_vertex(start + i + 1, v2);
                        submit_triangle(v0, v1, v2);
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
