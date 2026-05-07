#include "pvr_render_queue_visitor.h"
#include "pvr_renderer.h"
#include "pvr_texture_manager.h"

#include "../../meshes/submesh.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../types.h"
#include "../../vertex_data.h"
#include "../../assets/material.h"
#include "../../utils/pbr.h"
#include "../../logging.h"

#ifdef __DREAMCAST__
#include <kos.h>
#include <dc/pvr.h>
#include <dc/matrix.h>
#include <dc/fmath.h>
#else
/* Provide fallback definitions for non-Dreamcast builds (stub compilation) */
#define PVR_LIST_OP_POLY 0
#define PVR_LIST_PT_POLY 4
#define PVR_LIST_TR_POLY 2
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
        ambient_[0] = 0.2f;
        ambient_[1] = 0.2f;
        ambient_[2] = 0.2f;
        ambient_[3] = 1.0f;
    }

    S_VERBOSE("Init DR state");
    pvr_dr_init(&renderer_->dr_state_);
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
        renderer_->current_list_type_ = PVR_LIST_OP_POLY;
    } else if(blend == BLEND_MASK) {
        renderer_->current_list_type_ = PVR_LIST_PT_POLY;
    } else {
        renderer_->current_list_type_ = PVR_LIST_TR_POLY;
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
            blend_src = PVR_BLEND_DESTCOLOR;
            blend_dst = PVR_BLEND_ZERO;
            break;
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
        case DEPTH_FUNC_NEVER: depth_func = PVR_DEPTHCMP_NEVER; break;
        case DEPTH_FUNC_LESS: depth_func = PVR_DEPTHCMP_GREATER; break;
        case DEPTH_FUNC_LEQUAL: depth_func = PVR_DEPTHCMP_GEQUAL; break;
        case DEPTH_FUNC_EQUAL: depth_func = PVR_DEPTHCMP_EQUAL; break;
        case DEPTH_FUNC_GEQUAL: depth_func = PVR_DEPTHCMP_LEQUAL; break;
        case DEPTH_FUNC_GREATER: depth_func = PVR_DEPTHCMP_LESS; break;
        case DEPTH_FUNC_ALWAYS: depth_func = PVR_DEPTHCMP_ALWAYS; break;
        default: depth_func = PVR_DEPTHCMP_GEQUAL; break;
    }

    /* Map cull mode */
    int cull_mode;
    switch(next->cull_mode()) {
        case CULL_MODE_NONE: cull_mode = PVR_CULLING_NONE; break;
        case CULL_MODE_BACK_FACE: cull_mode = PVR_CULLING_CW; break;
        case CULL_MODE_FRONT_FACE: cull_mode = PVR_CULLING_CCW; break;
        case CULL_MODE_FRONT_AND_BACK_FACE: cull_mode = PVR_CULLING_SMALL; break;
        default: cull_mode = PVR_CULLING_CW; break;
    }

    int shade_mode = (next->shade_model() == SHADE_MODEL_FLAT) ?
        PVR_SHADE_FLAT : PVR_SHADE_GOURAUD;

    int fog_type;
    switch(next->fog_mode()) {
        case FOG_MODE_NONE: fog_type = PVR_FOG_DISABLE; break;
        case FOG_MODE_LINEAR: fog_type = PVR_FOG_TABLE; break;
        case FOG_MODE_EXP: fog_type = PVR_FOG_TABLE; break;
        case FOG_MODE_EXP2: fog_type = PVR_FOG_TABLE; break;
        default: fog_type = PVR_FOG_DISABLE; break;
    }

    /* Look up texture and build polygon context, then compile to poly_hdr_ */
    pvr_poly_cxt_t cxt;
    PVRTextureObject* tex_obj = nullptr;
    if((next->textures_enabled() & BASE_COLOR_MAP_ENABLED) != 0) {
        auto tex = next->base_color_map();
        if(tex) {
            tex_obj = renderer_->texture_manager().bind_texture(tex->_renderer_specific_id());
        }
    }

    if(tex_obj && tex_obj->texture_vram) {
        pvr_poly_cxt_txr(&cxt, renderer_->current_list_type_,
                         tex_obj->format,
                         tex_obj->width, tex_obj->height,
                         (pvr_ptr_t)tex_obj->texture_vram,
                         (tex_obj->filter == TEXTURE_FILTER_BILINEAR) ?
                             PVR_FILTER_BILINEAR : PVR_FILTER_NONE);
    } else {
        pvr_poly_cxt_col(&cxt, renderer_->current_list_type_);
    }

    cxt.fmt.color = PVR_CLRFMT_4FLOATS;
    cxt.fmt.uv = PVR_UVFMT_32BIT;
    cxt.gen.color_clamp = PVR_CLRCLAMP_ENABLE;
    cxt.gen.shading = shade_mode;
    cxt.gen.culling = cull_mode;
    cxt.gen.fog_type = fog_type;
    cxt.depth.comparison = next->is_depth_test_enabled() ? depth_func : PVR_DEPTHCMP_ALWAYS;
    cxt.depth.write = next->is_depth_write_enabled() ? PVR_DEPTHWRITE_ENABLE : PVR_DEPTHWRITE_DISABLE;
    cxt.blend.src = blend_src;
    cxt.blend.dst = blend_dst;

    if(renderer_->current_list_type_ == PVR_LIST_TR_POLY) {
        cxt.gen.alpha = PVR_ALPHA_ENABLE;
    }

    pvr_poly_compile(&poly_hdr_, &cxt);
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

/* Linearly interpolate a ClipVertex from v1 to v2 at parameter t */
static inline ClipVertex lerp_vertex(const ClipVertex& v1, const ClipVertex& v2, float t) {
    ClipVertex out;
    float inv_t = 1.0f - t;
    out.x = inv_t * v1.x + t * v2.x;
    out.y = inv_t * v1.y + t * v2.y;
    out.z = inv_t * v1.z + t * v2.z;
    out.w = inv_t * v1.w + t * v2.w;
    out.u = inv_t * v1.u + t * v2.u;
    out.v = inv_t * v1.v + t * v2.v;
    out.r = inv_t * v1.r + t * v2.r;
    out.g = inv_t * v1.g + t * v2.g;
    out.b = inv_t * v1.b + t * v2.b;
    out.a = inv_t * v1.a + t * v2.a;
    return out;
}

#endif

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

    /* Build viewport transform matrix */
    float hw = 320.0f; /* Half-width */
    float hh = 240.0f; /* Half-height */

    /* ================================================================
     * Submit or buffer the pre-compiled polygon header
     * ================================================================ */
    if(renderer_->current_list_type_ == PVR_LIST_OP_POLY) {
        pvr_vertex_t* hdr_dest = pvr_dr_target(renderer_->dr_state_);
        memcpy(hdr_dest, &poly_hdr_, sizeof(pvr_poly_hdr_t));
        pvr_dr_commit(hdr_dest);
    } else {
        auto& buf = (renderer_->current_list_type_ == PVR_LIST_PT_POLY) ? renderer_->pt_buffer_ : renderer_->tr_buffer_;
        std::size_t off = buf.size();
        buf.resize(off + sizeof(pvr_poly_hdr_t));
        memcpy(buf.data() + off, &poly_hdr_, sizeof(pvr_poly_hdr_t));
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

    /* Lambda to read a vertex and transform to clip space with lighting */
    auto read_and_transform_vertex = [&](uint32_t index) -> ClipVertex {
        const uint8_t* ptr = raw_data + (stride * index);
        ClipVertex cv;

        /* Position */
        float px, py, pz;
        {
            const float* p = (const float*)(ptr + pos_offset);
            px = p[0]; py = p[1]; pz = p[2];
        }

        /* Transform by MVP to clip space */
        cv.x = mvp[0] * px + mvp[4] * py + mvp[8]  * pz + mvp[12];
        cv.y = mvp[1] * px + mvp[5] * py + mvp[9]  * pz + mvp[13];
        cv.z = mvp[2] * px + mvp[6] * py + mvp[10] * pz + mvp[14];
        cv.w = mvp[3] * px + mvp[7] * py + mvp[11] * pz + mvp[15];

        /* UV */
        cv.u = 0.0f; cv.v = 0.0f;
        if(uv_offset) {
            const float* t = (const float*)(ptr + uv_offset);
            cv.u = t[0];
            cv.v = t[1];
        }

        /* Color - start with material diffuse */
        cv.r = mat_diffuse_[0]; cv.g = mat_diffuse_[1];
        cv.b = mat_diffuse_[2]; cv.a = mat_diffuse_[3];

        /* Read vertex color if present */
        float vert_r = 1.0f, vert_g = 1.0f, vert_b = 1.0f, vert_a = 1.0f;
        if(color_offset) {
            VertexAttribute attr = spec.color_attribute;
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
            case COLOR_MATERIAL_AMBIENT_AND_DIFFUSE:
                cv.r = vert_r; cv.g = vert_g; cv.b = vert_b; cv.a = vert_a;
                break;
            default:
                break;
        }

        /* Simple per-vertex directional lighting if enabled */
        if(lighting_enabled) {
            const float* n_ptr = (const float*)(ptr + normal_offset);
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
                    lx = -lights_[li].position[0];
                    ly = -lights_[li].position[1];
                    lz = -lights_[li].position[2];
                    float l_len = sqrtf(lx*lx + ly*ly + lz*lz);
                    if(l_len > 0.0001f) {
                        float inv = 1.0f / l_len;
                        lx *= inv; ly *= inv; lz *= inv;
                    }
                } else {
                    float vx = modelview[12], vy = modelview[13], vz = modelview[14];
                    lx = lights_[li].position[0] - vx;
                    ly = lights_[li].position[1] - vy;
                    lz = lights_[li].position[2] - vz;
                    float dist = sqrtf(lx*lx + ly*ly + lz*lz);
                    if(dist > 0.0001f) {
                        float inv = 1.0f / dist;
                        lx *= inv; ly *= inv; lz *= inv;
                    }
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

            if(cv.r > 1.0f) cv.r = 1.0f;
            if(cv.g > 1.0f) cv.g = 1.0f;
            if(cv.b > 1.0f) cv.b = 1.0f;
        }

        return cv;
    };

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
        float inv_w = 1.0f / w;

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
            pvr_vertex_t* dest1 = pvr_dr_target(renderer_->dr_state_);
            *((uint32_t*)dest1 + 0) = vert.flags;
            *((float*)dest1 + 1) = vert.x;
            *((float*)dest1 + 2) = vert.y;
            *((float*)dest1 + 3) = vert.z;
            *((float*)dest1 + 4) = vert.u;
            *((float*)dest1 + 5) = vert.v;
            *((uint32_t*)dest1 + 6) = 0;  /* padding */
            *((uint32_t*)dest1 + 7) = 0;  /* padding */
            pvr_dr_commit(dest1);

            pvr_vertex_t* dest2 = pvr_dr_target(renderer_->dr_state_);
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
            auto& buf = (renderer_->current_list_type_ == PVR_LIST_PT_POLY) ? renderer_->pt_buffer_ : renderer_->tr_buffer_;
            std::size_t off = buf.size();
            buf.resize(off + sizeof(pvr_vertex_type5_t));
            memcpy(buf.data() + off, &vert, sizeof(pvr_vertex_type5_t));
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
     * Submit geometry with near-plane clipping
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
                ClipVertex v0 = read_and_transform_vertex(get_index(i + 0));
                ClipVertex v1 = read_and_transform_vertex(get_index(i + 1));
                ClipVertex v2 = read_and_transform_vertex(get_index(i + 2));
                bool is_last = (i + 5 >= icount);
                process_triangle(v0, v1, v2, is_last);
            }
        } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
            /* For triangle strips, each consecutive 3 vertices form a triangle.
             * Vertices alternate winding, which we handle by swapping v1/v2. */
            if(icount >= 3) {
                for(std::size_t i = 0; i + 2 < icount; i++) {
                    ClipVertex v0 = read_and_transform_vertex(get_index(i + 0));
                    ClipVertex v1 = read_and_transform_vertex(get_index(i + 1));
                    ClipVertex v2 = read_and_transform_vertex(get_index(i + 2));
                    bool is_last = (i + 3 >= icount);
                    /* Odd triangles have reversed winding */
                    if(i & 1) {
                        process_triangle(v1, v0, v2, is_last);
                    } else {
                        process_triangle(v0, v1, v2, is_last);
                    }
                }
            }
        } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) {
            if(icount >= 3) {
                ClipVertex v0 = read_and_transform_vertex(get_index(0));
                for(std::size_t i = 1; i + 1 < icount; i++) {
                    ClipVertex v1 = read_and_transform_vertex(get_index(i));
                    ClipVertex v2 = read_and_transform_vertex(get_index(i + 1));
                    bool is_last = (i + 2 >= icount);
                    process_triangle(v0, v1, v2, is_last);
                }
            }
        }
    } else {
        /* Non-indexed range-based rendering */
        const VertexRange* ranges = renderable->vertex_ranges;
        std::size_t range_count = renderable->vertex_range_count;
        for(std::size_t ri = 0; ri < range_count; ++ri) {
            uint32_t start = ranges[ri].start;
            uint32_t count = ranges[ri].count;

            if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLES) {
                for(uint32_t i = 0; i + 2 < count; i += 3) {
                    ClipVertex v0 = read_and_transform_vertex(start + i + 0);
                    ClipVertex v1 = read_and_transform_vertex(start + i + 1);
                    ClipVertex v2 = read_and_transform_vertex(start + i + 2);
                    bool is_last = (i + 5 >= count) && (ri + 1 >= range_count);
                    process_triangle(v0, v1, v2, is_last);
                }
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
                if(count >= 3) {
                    for(uint32_t i = 0; i + 2 < count; i++) {
                        ClipVertex v0 = read_and_transform_vertex(start + i + 0);
                        ClipVertex v1 = read_and_transform_vertex(start + i + 1);
                        ClipVertex v2 = read_and_transform_vertex(start + i + 2);
                        bool is_last = (i + 3 >= count) && (ri + 1 >= range_count);
                        if(i & 1) {
                            process_triangle(v1, v0, v2, is_last);
                        } else {
                            process_triangle(v0, v1, v2, is_last);
                        }
                    }
                }
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) {
                if(count >= 3) {
                    ClipVertex v0 = read_and_transform_vertex(start);
                    for(uint32_t i = 1; i + 1 < count; i++) {
                        ClipVertex v1 = read_and_transform_vertex(start + i);
                        ClipVertex v2 = read_and_transform_vertex(start + i + 1);
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
