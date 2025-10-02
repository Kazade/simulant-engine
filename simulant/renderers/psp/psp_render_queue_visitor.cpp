#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

#include "../../utils/pbr.h"
#include "psp_render_group_impl.h"
#include "psp_render_queue_visitor.h"
#include "psp_renderer.h"

#include "../../application.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../stage.h"
#include "../../window.h"

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
    sceGuAmbient(l.to_abgr_8888());
}

void PSPRenderQueueVisitor::visit(const Renderable* renderable, const MaterialPass* pass, batcher::Iteration iteration) {
    S_VERBOSE("visit");
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

    auto s = pbr_to_traditional(next->base_color(), next->metallic(),
                                next->roughness(), next->specular_color(),
                                next->specular());

    const auto& diffuse = s.diffuse;
    sceGuMaterial(GU_DIFFUSE,
                  GU_COLOR(diffuse.r, diffuse.g, diffuse.b, diffuse.a));

    const auto& ambient = s.ambient;
    sceGuMaterial(GU_AMBIENT,
                  GU_COLOR(ambient.r, ambient.g, ambient.b, ambient.a));

    const auto& specular = s.specular;
    sceGuMaterial(GU_SPECULAR,
                  GU_COLOR(specular.r, specular.g, specular.b, specular.a));

    sceGuSpecular(s.shininess);

    switch (next->color_material()) {
    case COLOR_MATERIAL_NONE:
        sceGuColorMaterial(0);
        break;
    case COLOR_MATERIAL_AMBIENT:
        sceGuColorMaterial(GU_AMBIENT);
        break;
    case COLOR_MATERIAL_DIFFUSE:
        sceGuColorMaterial(GU_DIFFUSE);
        break;
    case COLOR_MATERIAL_AMBIENT_AND_DIFFUSE:
        sceGuColorMaterial(GU_AMBIENT | GU_DIFFUSE);
        break;
    default:
        break;
    }

    if(next->is_lighting_enabled()) {
        sceGuEnable(GU_LIGHTING);
    } else {
        sceGuDisable(GU_LIGHTING);
    }

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

    sceGuEnable(GU_BLEND);
    switch(next->blend_func()) {
        case BLEND_NONE:
            sceGuDisable(GU_BLEND);
            break;
        case BLEND_ADD:
            sceGuBlendFunc(GU_ADD, GU_FIX, GU_FIX, 0xFFFFFFFF, 0xFFFFFFFF);
            break;
        case BLEND_ALPHA:
            sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
            break;
        case BLEND_COLOR:
            sceGuBlendFunc(GU_ADD, GU_SRC_COLOR, GU_ONE_MINUS_SRC_COLOR, 0, 0);
            break;
        case BLEND_MODULATE:
            sceGuBlendFunc(GU_ADD, GU_DST_COLOR, GU_ZERO, 0, 0);
            break;
        case BLEND_ONE_ONE_MINUS_ALPHA:
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
}

void PSPRenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {
    if(!count) {
        return;
    }

    for(uint8_t i = 0; i < 4; ++i) {
        auto light = lights[i];
        bool enabled = i < count;

        if(enabled) {
            auto pos = camera_->view_matrix() * light->transform->position();

            ScePspFVector3 light_pos = {pos.x, pos.y, -pos.z};
            sceGuEnable(GU_LIGHT0 + i);

            if (light->light_type() == LIGHT_TYPE_DIRECTIONAL) {
                sceGuLight(i, GU_DIRECTIONAL, GU_DIFFUSE_AND_SPECULAR,
                           &light_pos);
            } else {
                sceGuLight(i, GU_POINTLIGHT, GU_DIFFUSE_AND_SPECULAR,
                           &light_pos);
            }

            auto d = light->diffuse();
            auto a = light->ambient();
            auto s = light->specular();

            sceGuLightColor(i, GU_DIFFUSE, d.to_abgr_8888());
            sceGuLightColor(i, GU_AMBIENT, a.to_abgr_8888());
            sceGuLightColor(i, GU_SPECULAR, s.to_abgr_8888());

            sceGuLightAtt(i, light->constant_attenuation(),
                          light->linear_attenuation(),
                          light->quadratic_attenuation());
        } else {
            sceGuDisable(GU_LIGHT0 + i);
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

static void convert_and_push(std::vector<PSPVertex>& buffer, const uint8_t* it,
                             const VertexSpecification& spec) {
    auto pos_off = spec.position_offset(false);
    auto uv_off = (spec.has_texcoord0())
                      ? spec.texcoord0_offset(false)
                      : 0; // FIXME: 0 assumes not first attribute
    auto color_off = (spec.has_base_color()) ? spec.diffuse_offset(false) : 0;
    auto normal_off = (spec.has_normals()) ? spec.normal_offset(false) : 0;

    int i = buffer.size();
    buffer.push_back(PSPVertex());

    PSPVertex* v = &buffer[i];
    memset(v, 0, sizeof(PSPVertex));
    v->color = 0xFFFF;

    if(uv_off) {
        convert_uv(&v->u, it + uv_off, spec.texcoord0_attribute);
    }

    if(color_off) {
        convert_color(&v->color, it + color_off, spec.diffuse_attribute);
    }

    if(normal_off) {
        convert_normal(&v->nx, it + normal_off, spec.normal_attribute);
    }

    convert_position(&v->x, it + pos_off, spec.position_attribute);
}

static std::vector<PSPVertex> buffer;

static void zclip_tristrips_and_submit_range(const VertexRange* range,
                                             const VertexSpecification& spec,
                                             const uint8_t* data,
                                             std::size_t stride) {
    buffer.clear();

    const uint8_t* it = data + (stride * range->start);

    for(std::size_t i = 0; i < range->count; ++i) {
        convert_and_push(buffer, it, spec);
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
                                             std::size_t stride) {
    buffer.clear();

    const uint8_t* it = data + (stride * range->start);

    for(std::size_t i = 0; i < range->count; ++i) {
        convert_and_push(buffer, it, spec);
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

    const auto& model = renderable->final_transformation;
    const auto& view = camera_->view_matrix();
    const auto& projection = camera_->projection_matrix();

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
                    &buffer[0], stride);
                break;
            case MESH_ARRANGEMENT_TRIANGLES:
                zclip_triangles_and_submit_range(
                    &range, renderable->vertex_data->vertex_specification(),
                    &buffer[0], stride);
                break;
            default:
                break;
        }

        total += range.count;
    } else {
        auto range = renderable->vertex_ranges;

        for(std::size_t i = 0; i < renderable->vertex_range_count; ++i, ++range) {
            switch(renderable->arrangement) {
                case MESH_ARRANGEMENT_TRIANGLE_STRIP:
                    zclip_tristrips_and_submit_range(
                        range, renderable->vertex_data->vertex_specification(),
                        renderable->vertex_data->data(),
                        renderable->vertex_data->vertex_specification()
                            .stride());
                    break;
                case MESH_ARRANGEMENT_TRIANGLES:
                    zclip_triangles_and_submit_range(
                        range, renderable->vertex_data->vertex_specification(),
                        renderable->vertex_data->data(),
                        renderable->vertex_data->vertex_specification()
                            .stride());
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


