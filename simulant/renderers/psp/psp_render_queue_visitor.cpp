#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

#include <algorithm>
#include <cstring>
#include <vector>

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
    auto base_color_map = enabled & (1 << 0) ? next->base_color_map() : nullptr;
    if(base_color_map) {
        sceGuEnable(GU_TEXTURE_2D);

        // We have to keep the filter in sync
        auto id = base_color_map->_renderer_specific_id();
        auto tex = renderer_->texture_manager_.find_texture(id);
        if(tex) {
            tex->filter = base_color_map->texture_filter();
        }

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

        /* Pre-normalised toward-light direction for directional lights.
         * position already stores -pointing_dir (set_direction negates on
         * write), so pos.xyz = view_rot * (-pointing_dir) = toward_light in
         * eye space. No further negation needed. */
        if(state.position[3] < 0.5f) {
            float len = sqrtf(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
            if(len > 1e-8f) {
                state.dir[0] = pos.x / len;
                state.dir[1] = pos.y / len;
                state.dir[2] = pos.z / len;
            }
        }
    }
}

/* The GE's vertex format. Normals aren't included: lighting is worked out in
 * software (see VertexConverter), so the GE never reads them. The color's
 * padded so the position is 4 byte aligned, as the GE expects. */
struct PSPVertex {
    float u, v;
    uint16_t color;
    uint16_t pad;
    float x, y, z;
};

static_assert(sizeof(PSPVertex) == 24, "PSPVertex must match PSP_VERTEX_FORMAT");

#define PSP_VERTEX_FORMAT \
    (GU_TEXTURE_32BITF | GU_COLOR_4444 | GU_VERTEX_32BITF | GU_TRANSFORM_3D)

static void convert_position(float* vout, const uint8_t* vin, VertexAttribute type) {
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
            vout[0] = vout[1] = vout[2] = 0.0f;
            break;
    }
}

static bool read_color(float* out, const uint8_t* c, VertexAttribute attr) {
    switch(attr) {
        case VERTEX_ATTRIBUTE_4F: {
            const float* fc = (const float*)c;
            out[0] = fc[0]; out[1] = fc[1]; out[2] = fc[2]; out[3] = fc[3];
        } return true;
        case VERTEX_ATTRIBUTE_3F: {
            const float* fc = (const float*)c;
            out[0] = fc[0]; out[1] = fc[1]; out[2] = fc[2]; out[3] = 1.0f;
        } return true;
        case VERTEX_ATTRIBUTE_4UB_RGBA:  // Also VERTEX_ATTRIBUTE_4UB
            out[0] = c[0] / 255.0f; out[1] = c[1] / 255.0f;
            out[2] = c[2] / 255.0f; out[3] = c[3] / 255.0f;
            return true;
        case VERTEX_ATTRIBUTE_4UB_BGRA:
            out[0] = c[2] / 255.0f; out[1] = c[1] / 255.0f;
            out[2] = c[0] / 255.0f; out[3] = c[3] / 255.0f;
            return true;
        default:
            out[0] = out[1] = out[2] = out[3] = 1.0f;
            return false;
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

/* Converts Simulant's vertices to the GE's. Everything that's the same for
 * every vertex of a draw (where the attributes are, their formats, whether
 * it's lit) is worked out once, up front. */
class VertexConverter {
public:
    VertexConverter(const VertexSpecification& spec, const PSPLightingContext& lctx):
        lctx_(lctx),
        position_offset_(spec.position_offset(false)),
        position_attribute_(spec.position_attribute),
        has_uv_(spec.has_texcoord0() && (
            spec.texcoord0_attribute == VERTEX_ATTRIBUTE_2F ||
            spec.texcoord0_attribute == VERTEX_ATTRIBUTE_3F ||
            spec.texcoord0_attribute == VERTEX_ATTRIBUTE_4F)),
        uv_offset_(has_uv_ ? spec.texcoord0_offset(false) : 0),
        has_color_(spec.has_color()),
        color_offset_(has_color_ ? spec.color_offset(false) : 0),
        color_attribute_(spec.color_attribute) {

        /* Lighting needs float normals */
        bool has_normals = spec.has_normals() &&
            spec.normal_attribute == VERTEX_ATTRIBUTE_3F;
        normal_offset_ = has_normals ? spec.normal_offset(false) : 0;
        lit_ = lctx.lighting_enabled && has_normals && lctx.light_count > 0;

        /* Without lighting or vertex colors, every vertex is the
         * material's color */
        flat_color_ = smlt::Color(
            lctx.base_color[0], lctx.base_color[1],
            lctx.base_color[2], lctx.base_color[3]
        ).to_abgr_4444();
    }

    void convert(const uint8_t* in, PSPVertex* out) const {
        if(has_uv_) {
            const float* uv = (const float*)(in + uv_offset_);
            out->u = uv[0];
            out->v = uv[1];
        } else {
            out->u = out->v = 0.0f;
        }

        out->pad = 0;

        float pos[3];
        convert_position(pos, in + position_offset_, position_attribute_);
        out->x = pos[0];
        out->y = pos[1];
        out->z = pos[2];

        if(!lit_ && !has_color_) {
            out->color = flat_color_;
            return;
        }

        /* Vertex color modulates the base color (before lighting, as in
         * the GL2X shader) */
        float color[4] = {
            lctx_.base_color[0], lctx_.base_color[1],
            lctx_.base_color[2], lctx_.base_color[3]
        };

        if(has_color_) {
            float vc[4];
            read_color(vc, in + color_offset_, color_attribute_);
            color[0] *= vc[0]; color[1] *= vc[1];
            color[2] *= vc[2]; color[3] *= vc[3];
        }

        if(!lit_) {
            out->color = smlt::Color(color[0], color[1], color[2], color[3]).to_abgr_4444();
            return;
        }

        /* Lighting's done in eye space */
        const Mat4& modelview = *lctx_.modelview;
        Vec4 ep = modelview * Vec4(pos[0], pos[1], pos[2], 1.0f);
        float eye_pos[3] = {ep.x, ep.y, ep.z};

        const float* n = (const float*)(in + normal_offset_);
        Vec3 nv = (modelview * Vec3(n[0], n[1], n[2])).normalized();
        float normal[3] = {nv.x, nv.y, nv.z};

        float lit_color[4];
        smlt::compute_pbr_vertex_color(normal, eye_pos, color, lctx_.metallic,
                                       lctx_.roughness, lctx_.ambient,
                                       lctx_.lights, lctx_.light_count, lit_color);

        out->color = smlt::Color(lit_color[0], lit_color[1], lit_color[2], color[3]).to_abgr_4444();
    }

private:
    const PSPLightingContext& lctx_;

    std::size_t position_offset_;
    VertexAttribute position_attribute_;
    bool has_uv_;
    std::size_t uv_offset_;
    bool has_color_;
    std::size_t color_offset_;
    VertexAttribute color_attribute_;
    std::size_t normal_offset_ = 0;
    bool lit_ = false;
    uint16_t flat_color_ = 0xFFFF;
};

static int gu_primitive(MeshArrangement arrangement) {
    switch(arrangement) {
        case MESH_ARRANGEMENT_TRIANGLE_STRIP: return GU_TRIANGLE_STRIP;
        case MESH_ARRANGEMENT_TRIANGLES: return GU_TRIANGLES;
        default: return -1;
    }
}

/* Vertices and indices go straight into the display list, where the GE reads
 * them from, rather than being built up somewhere else and copied in */
static void submit_range(int prim, const VertexRange& range,
                         const uint8_t* data, std::size_t stride,
                         const VertexConverter& converter) {
    if(!range.count) {
        return;
    }

    auto out = (PSPVertex*) sceGuGetMemory(range.count * sizeof(PSPVertex));
    const uint8_t* it = data + (stride * range.start);
    for(std::size_t i = 0; i < range.count; ++i, it += stride) {
        converter.convert(it, out + i);
    }

    sceGumDrawArray(prim, PSP_VERTEX_FORMAT, range.count, 0, out);
}

/* Indexed meshes share vertices between triangles (typically each is used by
 * several), so each vertex the draw uses is converted - and lit, the costly
 * part - just once, and the GE is given indices into them. They're
 * renumbered from 0, in the order they're first used, so a draw that only
 * uses a few of a big mesh's vertices only converts those. */
static void submit_indexed(int prim, const IndexData& indices,
                           std::size_t first, std::size_t count,
                           const VertexData& vertices,
                           const VertexConverter& converter) {
    if(!count) {
        return;
    }

    const uint8_t* data = vertices.data();
    std::size_t stride = vertices.stride();

    /* The GE's indices are 16 bit at most. Each vertex gets one at most,
     * so up to this many indices there can't be more vertices than that. */
    if(count > 0xFFFF) {
        auto out = (PSPVertex*) sceGuGetMemory(count * sizeof(PSPVertex));
        for(std::size_t i = 0; i < count; ++i) {
            converter.convert(data + indices.at(first + i) * stride, out + i);
        }
        sceGumDrawArray(prim, PSP_VERTEX_FORMAT, count, 0, out);
        return;
    }

    /* The new number of each vertex in this draw, if it has one: it has if
     * its stamp is this draw's. Stamping saves clearing the whole table
     * (sized for the biggest mesh) for each draw. */
    static std::vector<uint32_t> stamps;
    static std::vector<uint16_t> numbers;
    static std::vector<uint32_t> used;  // The vertices used, by new number
    static uint32_t stamp = 0;

    std::size_t vertex_count = vertices.count();
    if(stamps.size() < vertex_count) {
        stamps.resize(vertex_count, 0);
        numbers.resize(vertex_count, 0);
    }

    if(++stamp == 0) {
        std::fill(stamps.begin(), stamps.end(), 0);
        stamp = 1;
    }

    used.clear();

    auto out_indices = (uint16_t*) sceGuGetMemory(count * sizeof(uint16_t));
    for(std::size_t i = 0; i < count; ++i) {
        uint32_t index = indices.at(first + i);
        if(index >= vertex_count) {
            index = 0;
        }

        if(stamps[index] != stamp) {
            stamps[index] = stamp;
            numbers[index] = (uint16_t) used.size();
            used.push_back(index);
        }

        out_indices[i] = numbers[index];
    }

    auto out = (PSPVertex*) sceGuGetMemory(used.size() * sizeof(PSPVertex));
    for(std::size_t i = 0; i < used.size(); ++i) {
        converter.convert(data + used[i] * stride, out + i);
    }

    sceGumDrawArray(prim, PSP_VERTEX_FORMAT | GU_INDEX_16BIT, count,
                    out_indices, out);
}

void PSPRenderQueueVisitor::do_visit(const Renderable* renderable, const MaterialPass* material_pass, batcher::Iteration iteration) {
    _S_UNUSED(material_pass);
    _S_UNUSED(iteration);

    auto element_count = renderable->index_element_count;
    auto vertex_range_count = renderable->vertex_range_count;

    if(!element_count && !vertex_range_count) {
        return;
    }

    int prim = gu_primitive(renderable->arrangement);
    if(prim < 0) {
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

    const auto& spec = renderable->vertex_data->vertex_specification();
    VertexConverter converter(spec, lctx);

    std::size_t total = 0;
    if(element_count) {
        submit_indexed(prim, *renderable->index_data, renderable->first_index,
                       element_count, *renderable->vertex_data, converter);
        total = element_count;
    } else {
        auto stride = spec.stride();
        auto data = renderable->vertex_data->data();
        auto range = renderable->vertex_ranges;
        for(std::size_t i = 0; i < vertex_range_count; ++i, ++range) {
            submit_range(prim, *range, data, stride, converter);
            total += range->count;
        }
    }

    get_app()->stats->increment_polygons_rendered(renderable->arrangement,
                                                  total);
}

}
