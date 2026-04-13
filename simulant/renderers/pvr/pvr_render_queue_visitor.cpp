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
 * PVR Vertex type: standard 32-byte packed color format
 * Using pvr_vertex_t which matches what KOS direct rendering expects.
 * ======================================================================== */

/* ========================================================================
 * Constructor
 * ======================================================================== */

PVRRenderQueueVisitor::PVRRenderQueueVisitor(PVRRenderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera),
    prev_list_type_(-1) {
    memset(&dr_state_, 0, sizeof(dr_state_));
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

    prev_list_type_ = -1;

    /* Get ambient light from the stage if available */
    if(stage_node) {
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
    /* Mark the current list as used and finish it */
    if(prev_list_type_ >= 0 && prev_list_type_ < 5) {
        renderer_->set_pvr_list_used(prev_list_type_);
        pvr_list_finish();
        prev_list_type_ = -1;
    }
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
    int new_list_type;
    if(blend == BLEND_NONE) {
        /* Check for alpha test (punch-through) */
        if(next->alpha_func() != ALPHA_FUNC_NONE) {
            new_list_type = PVR_LIST_PT_POLY;
        } else {
            new_list_type = PVR_LIST_OP_POLY;
        }
    } else {
        new_list_type = PVR_LIST_TR_POLY;
    }

    /* If list type changed, start a new one */
    if(new_list_type != prev_list_type_) {
        ensure_list_opened(new_list_type);
    }

    /* Cache material state */
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

    /* Map cull mode */
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
 * ensure_list_opened - open a PVR list if needed
 * ======================================================================== */

void PVRRenderQueueVisitor::ensure_list_opened(int list_type) {
#ifdef __DREAMCAST__
    /* KOS only allows one pvr_list_begin/pvr_list_finish cycle per list type
     * per scene. If this list was already used, we can't reopen it. */
    if(list_type >= 0 && list_type < 5 && renderer_->pvr_list_used(list_type)) {
        return; /* List already used this scene, skip */
    }

    if(list_type != prev_list_type_) {
        /* Close any previously opened list first */
        if(prev_list_type_ >= 0 && prev_list_type_ < 5) {
            renderer_->set_pvr_list_used(prev_list_type_);
            pvr_list_finish();
        }
        pvr_list_begin(list_type);
        renderer_->set_pvr_list_used(list_type);
        pvr_dr_init(&dr_state_);
        prev_list_type_ = list_type;
    }
#else
    _S_UNUSED(list_type);
#endif
}

/* ========================================================================
 * submit_vertex - Submit a vertex via direct rendering API
 * ======================================================================== */

void PVRRenderQueueVisitor::submit_vertex(float x, float y, float z,
                                           float u, float v,
                                           float r, float g, float b, float a) {
#ifdef __DREAMCAST__
    pvr_vertex_t* vert = pvr_dr_target(dr_state_);

    vert->x = x;
    vert->y = y;
    vert->z = z;
    vert->u = u;
    vert->v = v;

    /* Pack ARGB color */
    uint32_t argb = ((uint32_t)(a * 255.0f) << 24) |
                    ((uint32_t)(r * 255.0f) << 16) |
                    ((uint32_t)(g * 255.0f) << 8)  |
                    ((uint32_t)(b * 255.0f) << 0);
    vert->argb = argb;

    vert->oargb = 0;

    pvr_dr_commit(vert);
#else
    _S_UNUSED(x); _S_UNUSED(y); _S_UNUSED(z);
    _S_UNUSED(u); _S_UNUSED(v);
    _S_UNUSED(r); _S_UNUSED(g); _S_UNUSED(b); _S_UNUSED(a);
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

    /* Build viewport transform matrix */
    float hw = 320.0f; /* Half-width */
    float hh = 240.0f; /* Half-height */

    /* ================================================================
     * Build polygon context and header
     * ================================================================ */
    pvr_poly_cxt_t cxt;

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
                filter = PVR_FILTER_TRILINEAR1;
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

    if(current_list_type_ == PVR_LIST_TR_POLY) {
        cxt.gen.alpha = PVR_ALPHA_ENABLE;
        if(tex_obj) {
            cxt.txr.alpha = PVR_TXRALPHA_ENABLE;
        }
    }

    pvr_poly_hdr_t hdr;
    pvr_poly_compile(&hdr, &cxt);

    /* Set Col_Type to Packed Color (0) in bits 5-4 for pvr_vertex_t format */
    hdr.cmd = (hdr.cmd & ~(3 << 4)) | (0 << 4);

    /* Submit polygon header via direct rendering */
    pvr_vertex_t* hdr_vert = pvr_dr_target(dr_state_);
    memcpy(hdr_vert, &hdr, sizeof(hdr));
    pvr_dr_commit(hdr_vert);

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

    /* Lambda to read a vertex, transform, light, and submit */
    auto process_and_submit_vertex = [&](uint32_t index, bool is_last) {
        const uint8_t* ptr = raw_data + (stride * index);

        /* Position */
        float px, py, pz;
        {
            const float* p = (const float*)(ptr + pos_offset);
            px = p[0]; py = p[1]; pz = p[2];
        }

        /* Transform by MVP */
        float cx = mvp[0] * px + mvp[4] * py + mvp[8]  * pz + mvp[12];
        float cy = mvp[1] * px + mvp[5] * py + mvp[9]  * pz + mvp[13];
        float cz = mvp[2] * px + mvp[6] * py + mvp[10] * pz + mvp[14];
        float cw = mvp[3] * px + mvp[7] * py + mvp[11] * pz + mvp[15];

        /* Apply viewport transform */
        cx = cx * hw + hw * cw;
        cy = -cy * hh + hh * cw;

        /* Perspective divide */
        if(cw == 0.0f) cw = FLT_EPSILON;
        float inv_w = 1.0f / cw;

        float sx = cx * inv_w;
        float sy = cy * inv_w;
        float sz = inv_w; /* PVR uses 1/w for depth */

        /* UV */
        float tu = 0.0f, tv = 0.0f;
        if(uv_offset) {
            const float* t = (const float*)(ptr + uv_offset);
            tu = t[0];
            tv = t[1];
        }

        /* Color - start with material diffuse */
        float cr = mat_diffuse_[0], cg = mat_diffuse_[1], cb = mat_diffuse_[2], ca = mat_diffuse_[3];

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
                cr = vert_r; cg = vert_g; cb = vert_b; ca = vert_a;
                break;
            case COLOR_MATERIAL_AMBIENT:
                break;
            case COLOR_MATERIAL_AMBIENT_AND_DIFFUSE:
                cr = vert_r; cg = vert_g; cb = vert_b; ca = vert_a;
                break;
            case COLOR_MATERIAL_NONE:
            default:
                break;
        }

        /* Simple per-vertex directional lighting if enabled */
        if(material_pass->is_lighting_enabled() && normal_offset) {
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
                total_r += cr * lights_[li].color[0] * ndotl * intensity;
                total_g += cg * lights_[li].color[1] * ndotl * intensity;
                total_b += cb * lights_[li].color[2] * ndotl * intensity;
            }

            cr = total_r;
            cg = total_g;
            cb = total_b;

            if(cr > 1.0f) cr = 1.0f;
            if(cg > 1.0f) cg = 1.0f;
            if(cb > 1.0f) cb = 1.0f;
        }

        /* Submit via direct rendering */
        pvr_vertex_t* vert = pvr_dr_target(dr_state_);
        vert->x = sx;
        vert->y = sy;
        vert->z = sz;
        vert->u = tu;
        vert->v = tv;

        uint32_t argb = ((uint32_t)(ca * 255.0f) << 24) |
                        ((uint32_t)(cr * 255.0f) << 16) |
                        ((uint32_t)(cg * 255.0f) << 8)  |
                        ((uint32_t)(cb * 255.0f) << 0);
        vert->argb = argb;
        vert->oargb = 0;
        vert->flags = is_last ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;

        pvr_dr_commit(vert);
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
                process_and_submit_vertex(get_index(i + 0), false);
                process_and_submit_vertex(get_index(i + 1), false);
                process_and_submit_vertex(get_index(i + 2), true);
            }
        } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
            for(std::size_t i = 0; i < icount; i++) {
                process_and_submit_vertex(get_index(i), i == icount - 1);
            }
        } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) {
            if(icount >= 3) {
                for(std::size_t i = 1; i + 1 < icount; i++) {
                    process_and_submit_vertex(get_index(0), false);
                    process_and_submit_vertex(get_index(i), false);
                    process_and_submit_vertex(get_index(i + 1), i + 1 == icount - 1);
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
                    process_and_submit_vertex(start + i + 0, false);
                    process_and_submit_vertex(start + i + 1, false);
                    process_and_submit_vertex(start + i + 2, true);
                }
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
                for(uint32_t i = 0; i < count; i++) {
                    process_and_submit_vertex(start + i, i == count - 1);
                }
            } else if(renderable->arrangement == MESH_ARRANGEMENT_TRIANGLE_FAN) {
                if(count >= 3) {
                    for(uint32_t i = 1; i + 1 < count; i++) {
                        process_and_submit_vertex(start + 0, false);
                        process_and_submit_vertex(start + i, false);
                        process_and_submit_vertex(start + i + 1, i + 1 == count - 1);
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
