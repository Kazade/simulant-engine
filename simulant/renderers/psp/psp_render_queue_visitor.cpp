#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

#include "../../application.h"
#include "../../meshes/submesh.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../stage.h"
#include "../../vertex_data.h"
#include "../../math/mat4.h"
#include "../../window.h"
#include "psp_render_group_impl.h"
#include "psp_render_queue_visitor.h"
#include "psp_renderer.h"

namespace smlt {

PSPRenderQueueVisitor::PSPRenderQueueVisitor(PSPRenderer* renderer,
                                             CameraPtr camera) :
    renderer_(renderer), camera_(camera) {}

void PSPRenderQueueVisitor::start_traversal(const batcher::RenderQueue &queue,
                                            uint64_t frame_id,
                                            StageNode *stage_node)
{
    S_VERBOSE("start_traversal");

    _S_UNUSED(queue);
    _S_UNUSED(frame_id);

    auto l = stage_node->scene->lighting->ambient_light();
    ambient_[0] = l.r; ambient_[1] = l.g; ambient_[2] = l.b;
    /* Hardware ambient is left set but lighting is disabled for the soft path */
    sceGuAmbient(l.to_abgr_8888());
}

void PSPRenderQueueVisitor::visit(const Renderable* renderable, const MaterialPass* pass, batcher::Iteration iteration) {
    S_VERBOSE("visit");
    /* Invisible renderables are still in the queue so consumers like
     * ShadowCaster can read them back to generate shadow geometry from a
     * hidden low-poly proxy. Skip the actual draw here. */
    if(renderable && !renderable->is_visible) {
        return;
    }
    /* Modifier-volume passes (e.g. the PVR cheap-shadow material on
     * ShadowCaster) are only meaningful for renderers with a modifier list.
     * The PSP uses stencil shadows instead, so skip these passes here. */
    if(pass && pass->polygon_list_target() != POLYGON_LIST_TARGET_NONE) {
        return;
    }
    do_visit(renderable, pass, iteration);
}

void PSPRenderQueueVisitor::end_traversal(const batcher::RenderQueue &queue, StageNode *stage_node)
{
    _S_UNUSED(queue);
    _S_UNUSED(stage_node);

    S_VERBOSE("end_traversal");
}

void PSPRenderQueueVisitor::change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next) {
    S_VERBOSE("change_render_group");
    _S_UNUSED(prev);
    _S_UNUSED(next);
}

void PSPRenderQueueVisitor::change_material_pass(const MaterialPass* prev, const MaterialPass* next) {
    S_VERBOSE("change_material_pass");
    pass_ = next;

    if(!next) {
        return;
    }

    /* Store PBR properties for software per-vertex lighting */
    const Color& bc = next->base_color();
    mat_base_color_[0] = bc.r;
    mat_base_color_[1] = bc.g;
    mat_base_color_[2] = bc.b;
    mat_base_color_[3] = bc.a;
    mat_metallic_  = next->metallic();
    mat_roughness_ = next->roughness();
    mat_lighting_enabled_ = next->is_lighting_enabled();

    /* Software per-vertex PBR handles lighting; disable hardware lighting */
    sceGuDisable(GU_LIGHTING);

    if(next->is_depth_test_enabled()) {
        sceGuEnable(GU_DEPTH_TEST);
    } else {
        sceGuDisable(GU_DEPTH_TEST);
    }

    if(next->is_depth_write_enabled()) {
        sceGuDepthMask(GU_FALSE);
    } else {
        sceGuDepthMask(GU_TRUE);
    }

    switch(next->depth_func()) {
        case DEPTH_FUNC_NEVER:
            sceGuDepthFunc(GU_NEVER);
            break;
        case DEPTH_FUNC_LEQUAL:
            sceGuDepthFunc(GU_LEQUAL);
            break;
        case DEPTH_FUNC_ALWAYS:
            sceGuDepthFunc(GU_ALWAYS);
            break;
        case DEPTH_FUNC_EQUAL:
            sceGuDepthFunc(GU_EQUAL);
            break;
        case DEPTH_FUNC_GEQUAL:
            sceGuDepthFunc(GU_GEQUAL);
            break;
        case DEPTH_FUNC_GREATER:
            sceGuDepthFunc(GU_GREATER);
            break;
        case DEPTH_FUNC_LESS:
            sceGuDepthFunc(GU_LESS);
            break;
    }

    switch(next->cull_mode()) {
        case CULL_MODE_NONE:
            sceGuDisable(GU_CULL_FACE);
            break;
        case CULL_MODE_FRONT_AND_BACK_FACE:
        case CULL_MODE_FRONT_FACE:
            sceGuEnable(GU_CULL_FACE);
            sceGuFrontFace(GU_CW);
            break;
        case CULL_MODE_BACK_FACE:
            sceGuEnable(GU_CULL_FACE);
            sceGuFrontFace(GU_CCW);
            break;
    }

    switch(next->blend_func()) {
        case BLEND_NONE:
            sceGuDisable(GU_BLEND);
            sceGuDisable(GU_ALPHA_TEST);
            break;
        case BLEND_MASK:
            sceGuDisable(GU_BLEND);
            sceGuEnable(GU_ALPHA_TEST);
            sceGuAlphaFunc(GU_GREATER, next->alpha_threshold() * 255, 0xFF);
            break;
        case BLEND_ADD:
            sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, 0xFFFFFFFF, 0xFFFFFFFF);
            break;
        case BLEND_ALPHA:
            sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
            break;
        case BLEND_COLOR:
            sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, GU_SRC_COLOR, GU_ONE_MINUS_SRC_COLOR, 0, 0);
            break;
        case BLEND_MODULATE:
            sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_ZERO, 0, 0);
            break;
        case BLEND_ONE_ONE_MINUS_ALPHA:
            sceGuEnable(GU_BLEND);
            sceGuBlendFunc(GU_ADD, GU_FIX, GU_ONE_MINUS_SRC_ALPHA, 0xFFFFFFFF,
                           0);
            break;
        default:
            break;
    }

    // FIXME: Multitexture?
    // FIXME: texture matrix
    auto enabled = next->textures_enabled();
    if((enabled & (1 << 0))) {
        sceGuEnable(GU_TEXTURE_2D);

        // We have to keep the filter in sync
        auto id = next->base_color_map()->_renderer_specific_id();
        auto tex = renderer_->texture_manager_.find_texture(id);
        tex->filter = next->base_color_map()->texture_filter();

        renderer_->texture_manager_.bind_texture(id);
    } else {
        sceGuDisable(GU_TEXTURE_2D);
    }

    /* Color write mask. sceGuPixelMask uses inverted semantics: bits that
     * are ON in the mask are *not* written. For "color writes off, leave the
     * alpha/stencil channel writable", mask the RGB bits (0x00FFFFFF). */
    if(next->is_color_write_enabled()) {
        sceGuPixelMask(0x00000000);
    } else {
        sceGuPixelMask(0x00FFFFFF);
    }

    /* Stencil test. The PSP draw buffer is GU_PSM_8888 so we have a real
     * 8-bit stencil sharing the alpha channel of the framebuffer. */
    if(next->is_stencil_test_enabled()) {
        sceGuEnable(GU_STENCIL_TEST);

        static auto to_gu_func = [](StencilFunc f) -> int {
            switch(f) {
                case STENCIL_FUNC_NEVER:     return GU_NEVER;
                case STENCIL_FUNC_LESS:      return GU_LESS;
                case STENCIL_FUNC_LEQUAL:    return GU_LEQUAL;
                case STENCIL_FUNC_GREATER:   return GU_GREATER;
                case STENCIL_FUNC_GEQUAL:    return GU_GEQUAL;
                case STENCIL_FUNC_EQUAL:     return GU_EQUAL;
                case STENCIL_FUNC_NOT_EQUAL: return GU_NOTEQUAL;
                case STENCIL_FUNC_ALWAYS:
                default:                     return GU_ALWAYS;
            }
        };

        /* The PSP has no _WRAP variant of INCR/DECR; behaviour wraps
         * implicitly at the framebuffer's alpha/stencil bit-depth (8 bits
         * here), which is fine for the shadow-volume incr/decr pair. */
        static auto to_gu_op = [](StencilOp o) -> int {
            switch(o) {
                case STENCIL_OP_ZERO:    return GU_ZERO;
                case STENCIL_OP_REPLACE: return GU_REPLACE;
                case STENCIL_OP_INVERT:  return GU_INVERT;
                case STENCIL_OP_INCR:
                case STENCIL_OP_INCR_WRAP: return GU_INCR;
                case STENCIL_OP_DECR:
                case STENCIL_OP_DECR_WRAP: return GU_DECR;
                case STENCIL_OP_KEEP:
                default:                   return GU_KEEP;
            }
        };

        sceGuStencilFunc(to_gu_func(next->stencil_func()),
                         next->stencil_ref(), next->stencil_mask());
        sceGuStencilOp(to_gu_op(next->stencil_fail_op()),
                       to_gu_op(next->stencil_depth_fail_op()),
                       to_gu_op(next->stencil_pass_op()));
    } else {
        sceGuDisable(GU_STENCIL_TEST);
    }
}

void PSPRenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {
    light_count_ = 0;
    for(int i = 0; i < 4; ++i) lights_[i].enabled = false;

    for(uint8_t i = 0; i < count && i < 4; ++i) {
        if(!lights[i]) continue;

        auto light = lights[i];
        VertexLightState& state = lights_[i];
        state.enabled = true;
        light_count_ = i + 1;

        auto pos = camera_->view_matrix() * light->transform->position();
        state.position[0] = pos.x;
        state.position[1] = pos.y;
        state.position[2] = pos.z;
        state.position[3] = (light->light_type() == LIGHT_TYPE_DIRECTIONAL) ? 0.0f : 1.0f;

        state.color[0] = light->color().r;
        state.color[1] = light->color().g;
        state.color[2] = light->color().b;

        state.intensity = light->intensity();
        state.range = light->range();

        /* Pre-normalise direction for directional lights */
        if(state.position[3] < 0.5f) {
            float len = sqrtf(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
            if(len > 1e-8f) {
                state.dir[0] = -pos.x / len;
                state.dir[1] = -pos.y / len;
                state.dir[2] = -pos.z / len;
            }
        }
    }
}

struct PSPVertex {
    float u, v;
    uint16_t color;
    int16_t nx, ny, nz;
    float x, y, z;
};

void convert_position(float* vout, const uint8_t* vin, VertexAttribute type) {
    switch(type) {
        case VERTEX_ATTRIBUTE_2F:
            vout[0] = ((float*)vin)[0];
            vout[1] = ((float*)vin)[1];
            vout[2] = 0.0f;
            break;
        case VERTEX_ATTRIBUTE_3F:
        case VERTEX_ATTRIBUTE_4F:
            vout[0] = ((float*)vin)[0];
            vout[1] = ((float*)vin)[1];
            vout[2] = ((float*)vin)[2];
            break;
        default:
            break;
    }
}

void convert_uv(float* vout, const uint8_t* vin, VertexAttribute type) {
    switch(type) {
        case VERTEX_ATTRIBUTE_2F:
        case VERTEX_ATTRIBUTE_3F:
        case VERTEX_ATTRIBUTE_4F:
            vout[0] = ((float*)vin)[0];
            vout[1] = ((float*)vin)[1];
            break;
        default:
            break;
    }
}

void convert_color(uint16_t* vout, const uint8_t* vin, VertexAttribute type) {
    const float* v = (const float*)vin;
    switch(type) {
        case VERTEX_ATTRIBUTE_4F:
            *vout = smlt::Color(v[0], v[1], v[2], v[3]).to_abgr_4444();
            break;
        case VERTEX_ATTRIBUTE_3F:
            *vout = smlt::Color(v[0], v[1], v[2], 1.0f).to_abgr_4444();
            break;
        case VERTEX_ATTRIBUTE_4UB_RGBA:
            *vout = smlt::Color::from_bytes(vin[0], vin[1], vin[2], vin[3]).to_abgr_4444();
            break;
        case VERTEX_ATTRIBUTE_4UB_BGRA:
            *vout = smlt::Color::from_bytes(vin[2], vin[1], vin[0], vin[3]).to_abgr_4444();
            break;
        default:
            *vout = 0xFFFF;
            break;
    }
}

void convert_normal(int16_t* vout, const uint8_t* vin, VertexAttribute type) {
    float* v = (float*)vin;
    switch(type) {
        case VERTEX_ATTRIBUTE_3F:
            vout[0] = ((float*)v)[0] * 32767.0f;
            vout[1] = ((float*)v)[1] * 32767.0f;
            vout[2] = ((float*)v)[2] * 32767.0f;
            break;
        default:
            S_ERROR("{0}", type);
    }
}

struct PSPLightingContext {
    const smlt::VertexLightState* lights;
    int light_count;
    float ambient[3];
    float base_color[4];
    float metallic;
    float roughness;
    const Mat4* modelview;
    bool  lighting_enabled;
};

static void convert_and_push(std::vector<PSPVertex>& buffer, const uint8_t* it,
                             const VertexSpecification& spec,
                             const PSPLightingContext& lctx) {
    auto pos_off    = spec.position_offset(false);
    auto uv_off     = spec.has_texcoord0() ? spec.texcoord0_offset(false) : 0;
    auto color_off  = spec.has_color()     ? spec.color_offset(false)     : 0;
    auto normal_off = spec.has_normals()   ? spec.normal_offset(false)    : 0;

    int idx = buffer.size();
    buffer.push_back(PSPVertex());

    PSPVertex* v = &buffer[idx];
    memset(v, 0, sizeof(PSPVertex));
    v->color = 0xFFFF;

    if(uv_off) {
        convert_uv(&v->u, it + uv_off, spec.texcoord0_attribute);
    }

    /* Base colour from material (or vertex colour if present) */
    float base_r = lctx.base_color[0];
    float base_g = lctx.base_color[1];
    float base_b = lctx.base_color[2];
    float base_a = lctx.base_color[3];

    if(color_off) {
        float vc_r, vc_g, vc_b, vc_a;
        const uint8_t* c = it + color_off;
        VertexAttribute attr = spec.color_attribute;
        if(attr == smlt::VERTEX_ATTRIBUTE_4F) {
            const float* fc = (const float*)c;
            vc_r = fc[0]; vc_g = fc[1]; vc_b = fc[2]; vc_a = fc[3];
        } else if(attr == smlt::VERTEX_ATTRIBUTE_3F) {
            const float* fc = (const float*)c;
            vc_r = fc[0]; vc_g = fc[1]; vc_b = fc[2]; vc_a = 1.0f;
        } else if(attr == smlt::VERTEX_ATTRIBUTE_4UB_RGBA || attr == smlt::VERTEX_ATTRIBUTE_4UB) {
            vc_r = c[0]/255.0f; vc_g = c[1]/255.0f; vc_b = c[2]/255.0f; vc_a = c[3]/255.0f;
        } else if(attr == smlt::VERTEX_ATTRIBUTE_4UB_BGRA) {
            vc_b = c[0]/255.0f; vc_g = c[1]/255.0f; vc_r = c[2]/255.0f; vc_a = c[3]/255.0f;
        } else {
            vc_r = vc_g = vc_b = vc_a = 1.0f;
        }
        base_r *= vc_r; base_g *= vc_g; base_b *= vc_b; base_a *= vc_a;
    }

    float pos[3] = {0, 0, 0};
    convert_position(pos, it + pos_off, spec.position_attribute);
    v->x = pos[0]; v->y = pos[1]; v->z = pos[2];

    if(lctx.lighting_enabled && normal_off && lctx.light_count > 0) {
        /* Transform position to eye space */
        Vec4 ep = *lctx.modelview * Vec4(pos[0], pos[1], pos[2], 1.0f);
        float eye_pos[3] = {ep.x, ep.y, ep.z};

        /* Transform normal to eye space (direction — Mat4*Vec3 uses upper-3x3) */
        const uint8_t* n_raw = it + normal_off;
        float nx_model = ((float*)n_raw)[0];
        float ny_model = ((float*)n_raw)[1];
        float nz_model = ((float*)n_raw)[2];
        Vec3 nv = (*lctx.modelview * Vec3(nx_model, ny_model, nz_model)).normalized();
        float N[3] = {nv.x, nv.y, nv.z};

        /* Vertex colour modulates base_color BEFORE PBR (same as GL2X shader) */
        float combined[4] = {base_r, base_g, base_b, base_a};
        float lit_color[4];
        smlt::compute_pbr_vertex_color(N, eye_pos, combined, lctx.metallic,
                                       lctx.roughness, lctx.ambient,
                                       lctx.lights, lctx.light_count, lit_color);

        v->color = smlt::Color(lit_color[0], lit_color[1], lit_color[2], base_a).to_abgr_4444();

        /* Store normals for format compatibility (hardware lighting is disabled) */
        if(spec.normal_attribute == smlt::VERTEX_ATTRIBUTE_3F) {
            v->nx = (int16_t)(nx_model * 32767.0f);
            v->ny = (int16_t)(ny_model * 32767.0f);
            v->nz = (int16_t)(nz_model * 32767.0f);
        }
    } else {
        /* No lighting: use base colour directly */
        v->color = smlt::Color(base_r, base_g, base_b, base_a).to_abgr_4444();

        if(normal_off) {
            convert_normal(&v->nx, it + normal_off, spec.normal_attribute);
        }
    }
}

static std::vector<PSPVertex> buffer;

static void zclip_tristrips_and_submit_range(const VertexRange* range,
                                             const VertexSpecification& spec,
                                             const uint8_t* data,
                                             std::size_t stride,
                                             const PSPLightingContext& lctx) {
    buffer.clear();

    const uint8_t* it = data + (stride * range->start);

    for(std::size_t i = 0; i < range->count; ++i) {
        convert_and_push(buffer, it, spec, lctx);
        it += stride;
    }

    PSPVertex* output =
        (PSPVertex*)sceGuGetMemory(buffer.size() * sizeof(PSPVertex));
    memcpy(output, buffer.data(), buffer.size() * sizeof(PSPVertex));

    sceGumDrawArray(GU_TRIANGLE_STRIP,
                    GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_NORMAL_16BIT |
                        GU_TRANSFORM_3D | GU_COLOR_4444,
                    buffer.size(), 0, output);
}

static void zclip_triangles_and_submit_range(const VertexRange* range,
                                             const VertexSpecification& spec,
                                             const uint8_t* data,
                                             std::size_t stride,
                                             const PSPLightingContext& lctx) {
    buffer.clear();

    const uint8_t* it = data + (stride * range->start);

    for(std::size_t i = 0; i < range->count; ++i) {
        convert_and_push(buffer, it, spec, lctx);
        it += stride;
    }

    PSPVertex* output =
        (PSPVertex*)sceGuGetMemory(buffer.size() * sizeof(PSPVertex));
    memcpy(output, buffer.data(), buffer.size() * sizeof(PSPVertex));

    sceGumDrawArray(GU_TRIANGLES,
                    GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_NORMAL_16BIT |
                        GU_TRANSFORM_3D | GU_COLOR_4444,
                    buffer.size(), 0, output);
}

void PSPRenderQueueVisitor::do_visit(const Renderable* renderable, const MaterialPass* material_pass, batcher::Iteration iteration) {
    _S_UNUSED(material_pass);
    _S_UNUSED(iteration);

    auto element_count = renderable->index_element_count;
    auto vertex_range_count = renderable->vertex_range_count;

    if(!element_count && !vertex_range_count) {
        return;
    }

    renderer_->prepare_to_render(renderable);

    const auto& model = *renderable->final_transformation;
    const auto& view = camera_->view_matrix();
    const auto& projection = camera_->projection_matrix();

    /* Build modelview for software per-vertex lighting */
    Mat4 modelview = view * model;

    ScePspFMatrix4* psp_model =
        (ScePspFMatrix4*)sceGuGetMemory(3 * sizeof(ScePspFMatrix4));
    ScePspFMatrix4* psp_view = psp_model + 1;
    ScePspFMatrix4* psp_proj = psp_model + 2;

    std::memcpy(psp_model, model.data(), sizeof(ScePspFMatrix4));
    std::memcpy(psp_view, view.data(), sizeof(ScePspFMatrix4));
    std::memcpy(psp_proj, projection.data(), sizeof(ScePspFMatrix4));

    /* PSP uses an inverse coordinate system, so we need to flip
     * some things to match GL */
    psp_proj->z.z *= -1;
    psp_proj->w.z *= -1;

    sceGuSetMatrix(GU_MODEL, psp_model);
    sceGuSetMatrix(GU_VIEW, psp_view);
    sceGuSetMatrix(GU_PROJECTION, psp_proj);

    /* Build lighting context for per-vertex PBR */
    PSPLightingContext lctx;
    lctx.lights          = lights_;
    lctx.light_count     = light_count_;
    lctx.ambient[0]      = ambient_[0];
    lctx.ambient[1]      = ambient_[1];
    lctx.ambient[2]      = ambient_[2];
    lctx.base_color[0]   = mat_base_color_[0];
    lctx.base_color[1]   = mat_base_color_[1];
    lctx.base_color[2]   = mat_base_color_[2];
    lctx.base_color[3]   = mat_base_color_[3];
    lctx.metallic        = mat_metallic_;
    lctx.roughness       = mat_roughness_;
    lctx.modelview       = &modelview;
    lctx.lighting_enabled = mat_lighting_enabled_;

    auto total = 0;

    if(element_count) {
        std::vector<uint8_t> buffer;
        auto stride = renderable->vertex_data->stride();
        buffer.resize(renderable->index_element_count * stride);

        uint8_t* dst = &buffer[0];

        for(std::size_t i = 0; i < renderable->index_element_count; ++i) {
            auto idx = renderable->index_data->at(i);
            auto offset = idx * stride;
            std::memcpy(dst, renderable->vertex_data->data() + offset, stride);
            dst += stride;
        }

        VertexRange range;
        range.start = 0;
        range.count = renderable->index_element_count;

        switch(renderable->arrangement) {
            case MESH_ARRANGEMENT_TRIANGLE_STRIP:
                zclip_tristrips_and_submit_range(
                    &range, renderable->vertex_data->vertex_specification(),
                    &buffer[0], stride, lctx);
                break;
            case MESH_ARRANGEMENT_TRIANGLES:
                zclip_triangles_and_submit_range(
                    &range, renderable->vertex_data->vertex_specification(),
                    &buffer[0], stride, lctx);
                break;
            default:
                break;
        }

        total += range.count;
    } else {
        auto range = renderable->vertex_ranges;
        auto spec_stride = renderable->vertex_data->vertex_specification().stride();

        for(std::size_t i = 0; i < renderable->vertex_range_count; ++i, ++range) {
            switch(renderable->arrangement) {
                case MESH_ARRANGEMENT_TRIANGLE_STRIP:
                    zclip_tristrips_and_submit_range(
                        range, renderable->vertex_data->vertex_specification(),
                        renderable->vertex_data->data(), spec_stride, lctx);
                    break;
                case MESH_ARRANGEMENT_TRIANGLES:
                    zclip_triangles_and_submit_range(
                        range, renderable->vertex_data->vertex_specification(),
                        renderable->vertex_data->data(), spec_stride, lctx);
                    break;
                default:
                    break;
            }

            total += range->count;
        }
    }

    get_app()->stats->increment_polygons_rendered(renderable->arrangement,
                                                  total);
}
}
