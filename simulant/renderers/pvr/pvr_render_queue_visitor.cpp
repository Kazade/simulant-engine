#include "pvr_render_queue_visitor.h"
#include "pvr_api.h"
#include "pvr_render_group_impl.h"
#include "pvr_renderer.h"

#include "../../application.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../stage.h"
#include "../../window.h"

namespace smlt {

PVRRenderQueueVisitor::PVRRenderQueueVisitor(PVRRenderer* renderer,
                                             CameraPtr camera) :
    renderer_(renderer), camera_(camera) {}

void PVRRenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue,
                                            uint64_t frame_id, Stage* stage) {

    S_VERBOSE("start_traversal");

    _S_UNUSED(queue);
    _S_UNUSED(frame_id);

    auto l = stage->ambient_light();
    pvr_ambient_color(l.to_abgr_8888());
}

void PVRRenderQueueVisitor::visit(const Renderable* renderable,
                                  const MaterialPass* pass,
                                  batcher::Iteration iteration) {
    S_VERBOSE("visit");
    do_visit(renderable, pass, iteration);
}

void PVRRenderQueueVisitor::end_traversal(const batcher::RenderQueue& queue,
                                          Stage* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(stage);

    S_VERBOSE("end_traversal");
}

void PVRRenderQueueVisitor::change_render_group(
    const batcher::RenderGroup* prev, const batcher::RenderGroup* next) {
    S_VERBOSE("change_render_group");
    _S_UNUSED(prev);
    _S_UNUSED(next);
}

void PVRRenderQueueVisitor::change_material_pass(const MaterialPass* prev,
                                                 const MaterialPass* next) {
    S_VERBOSE("change_material_pass");
    pass_ = next;

    if(!next) {
        return;
    }

    const auto& diffuse = next->diffuse();
    pvr_material(PVR_MATERIAL_MODE_DIFFUSE,
                 ARGB_COLOR(diffuse.r, diffuse.g, diffuse.b, diffuse.a));

    const auto& ambient = next->ambient();
    pvr_material(PVR_MATERIAL_MODE_AMBIENT,
                 ARGB_COLOR(ambient.r, ambient.g, ambient.b, ambient.a));

    const auto& specular = next->specular();
    pvr_material(PVR_MATERIAL_MODE_SPECULAR,
                 ARGB_COLOR(specular.r, specular.g, specular.b, specular.a));

    pvr_specular(next->shininess());

    switch(next->colour_material()) {
        case COLOUR_MATERIAL_NONE:
            pvr_color_material(0);
            break;
        case COLOUR_MATERIAL_AMBIENT:
            pvr_color_material(PVR_MATERIAL_MODE_AMBIENT);
            break;
        case COLOUR_MATERIAL_DIFFUSE:
            pvr_color_material(PVR_MATERIAL_MODE_DIFFUSE);
            break;
        case COLOUR_MATERIAL_AMBIENT_AND_DIFFUSE:
            pvr_color_material(PVR_MATERIAL_MODE_AMBIENT |
                               PVR_MATERIAL_MODE_DIFFUSE);
            break;
        default:
            break;
    }

    if(next->is_lighting_enabled()) {
        pvr_enable(PVR_STATE_LIGHTING);
    } else {
        pvr_disable(PVR_STATE_LIGHTING);
    }

    if(next->is_depth_test_enabled()) {
        pvr_enable(PVR_STATE_DEPTH_TEST);
    } else {
        pvr_disable(PVR_STATE_DEPTH_TEST);
    }

    if(next->is_depth_write_enabled()) {
        pvr_depth_mask(PVR_TRUE);
    } else {
        pvr_depth_mask(PVR_FALSE);
    }

    switch(next->depth_func()) {
        case DEPTH_FUNC_NEVER:
            pvr_depth_func(PVR_DEPTH_FUNC_NEVER);
            break;
        case DEPTH_FUNC_LEQUAL:
            pvr_depth_func(PVR_DEPTH_FUNC_LEQUAL);
            break;
        case DEPTH_FUNC_ALWAYS:
            pvr_depth_func(PVR_DEPTH_FUNC_ALWAYS);
            break;
        case DEPTH_FUNC_EQUAL:
            pvr_depth_func(PVR_DEPTH_FUNC_EQUAL);
            break;
        case DEPTH_FUNC_GEQUAL:
            pvr_depth_func(PVR_DEPTH_FUNC_GEQUAL);
            break;
        case DEPTH_FUNC_GREATER:
            pvr_depth_func(PVR_DEPTH_FUNC_GREATER);
            break;
        case DEPTH_FUNC_LESS:
            pvr_depth_func(PVR_DEPTH_FUNC_LESS);
            break;
    }

    switch(next->cull_mode()) {
        case CULL_MODE_NONE:
            pvr_disable(PVR_STATE_CULL_FACE);
            break;
        case CULL_MODE_FRONT_AND_BACK_FACE:
        case CULL_MODE_FRONT_FACE:
            pvr_enable(PVR_STATE_CULL_FACE);
            pvr_front_face(PVR_WINDING_CW);
            break;
        case CULL_MODE_BACK_FACE:
            pvr_enable(PVR_STATE_CULL_FACE);
            pvr_front_face(PVR_WINDING_CCW);
            break;
    }

    pvr_enable(PVR_STATE_BLENDING);
    switch(next->blend_func()) {
        case BLEND_NONE:
            pvr_disable(PVR_STATE_BLENDING);
            break;
        case BLEND_ADD:
            pvr_blend_func(PVR_BLEND_FACTOR_ONE, PVR_BLEND_FACTOR_ONE);
            break;
        case BLEND_ALPHA:
            pvr_blend_func(PVR_BLEND_FACTOR_SRC_ALPHA, PVR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
            break;
        case BLEND_COLOUR:
            pvr_blend_func(PVR_BLEND_FACTOR_ONE, PVR_BLEND_FACTOR_ONE_MINUS_OTHER_COLOR);
            break;
        case BLEND_MODULATE:
            pvr_blend_func(PVR_BLEND_FACTOR_OTHER_COLOR, PVR_BLEND_FACTOR_ZERO);
            break;
        case BLEND_ONE_ONE_MINUS_ALPHA:
            pvr_blend_func(PVR_BLEND_FACTOR_ONE, PVR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
            break;
        default:
            break;
    }

    // FIXME: Multitexture?
    // FIXME: texture matrix
    auto enabled = next->textures_enabled();
    if((enabled & (1 << 0))) {
        pvr_enable(PVR_STATE_TEXTURE_2D);

        // We have to keep the filter in sync
        auto id = next->diffuse_map()->_renderer_specific_id();
        auto tex = renderer_->texture_manager_.find_texture(id);
        tex->filter = next->diffuse_map()->texture_filter();

        renderer_->texture_manager_.bind_texture(id);
    } else {
        pvr_disable(PVR_STATE_TEXTURE_2D);
    }
}

void PVRRenderQueueVisitor::apply_lights(const LightPtr* lights,
                                         const uint8_t count) {
    if(!count) {
        return;
    }

    for(uint8_t i = 0; i < 4; ++i) {
        auto light = lights[i];
        bool enabled = i < count;

        pvr_light_t light_no = (pvr_light_t)(PVR_LIGHT0 + i);

        if(enabled) {
            auto pos = camera_->view_matrix() * light->absolute_position();

            pvr_vec3_t light_pos = {pos.x, pos.y, -pos.z};
            pvr_enable((pvr_state_t)(PVR_STATE_LIGHT0 + i));

            if(light->type() == LIGHT_TYPE_DIRECTIONAL) {
                pvr_light(light_no, PVR_LIGHT_TYPE_DIRECTIONAL,
                          PVR_LIGHT_COMP_DIFFUSE_AND_SPECULAR, &light_pos);
            } else {
                pvr_light(light_no, PVR_LIGHT_TYPE_POINT,
                          PVR_LIGHT_COMP_DIFFUSE_AND_SPECULAR, &light_pos);
            }

            auto d = light->diffuse();
            auto a = light->ambient();
            auto s = light->specular();

            pvr_light_color(light_no, PVR_LIGHT_COMP_DIFFUSE, d.to_abgr_8888());
            pvr_light_color(light_no, PVR_LIGHT_COMP_AMBIENT, a.to_abgr_8888());
            pvr_light_color(light_no, PVR_LIGHT_COMP_SPECULAR,
                            s.to_abgr_8888());

            pvr_light_attenuation(light_no, light->constant_attenuation(),
                                  light->linear_attenuation(),
                                  light->quadratic_attenuation());
        } else {
            pvr_disable((pvr_state_t)(PVR_STATE_LIGHT0 + i));
        }
    }
}

struct PVRVertex {
    pvr_vec3_t xyz;
    pvr_vec2_t uv;
    pvr_vec3_t normal;
    argb_color_t color;
    argb_color_t color_offset;
    pvr_vec2_t st;
};

void convert_position(pvr_vec3_t* vout, const uint8_t* vin,
                      VertexAttribute type) {
    switch(type) {
        case VERTEX_ATTRIBUTE_2F:
            vout->x = ((float*)vin)[0];
            vout->y = ((float*)vin)[1];
            vout->z = 0.0f;
            break;
        case VERTEX_ATTRIBUTE_3F:
        case VERTEX_ATTRIBUTE_4F:
            vout->x = ((float*)vin)[0];
            vout->y = ((float*)vin)[1];
            vout->z = ((float*)vin)[2];
            break;
        default:
            break;
    }
}

void convert_uv(pvr_vec2_t* vout, const uint8_t* vin, VertexAttribute type) {
    switch(type) {
        case VERTEX_ATTRIBUTE_2F:
        case VERTEX_ATTRIBUTE_3F:
        case VERTEX_ATTRIBUTE_4F:
            vout->x = ((float*)vin)[0];
            vout->y = ((float*)vin)[1];
            break;
        default:
            break;
    }
}

void convert_color(argb_color_t* vout, const uint8_t* vin,
                   VertexAttribute type) {
    const float* v = (const float*)vin;
    switch(type) {
        case VERTEX_ATTRIBUTE_4F:
            *vout = smlt::Colour(v[0], v[1], v[2], v[3]).to_argb_4444();
            break;
        case VERTEX_ATTRIBUTE_3F:
            *vout = smlt::Colour(v[0], v[1], v[2], 1.0f).to_argb_4444();
            break;
        case VERTEX_ATTRIBUTE_4UB_RGBA:
            *vout = smlt::Colour::from_bytes(vin[0], vin[1], vin[2], vin[3])
                        .to_argb_4444();
            break;
        case VERTEX_ATTRIBUTE_4UB_BGRA:
            *vout = smlt::Colour::from_bytes(vin[2], vin[1], vin[0], vin[3])
                        .to_argb_4444();
            break;
        default:
            *vout = 0xFFFF;
            break;
    }
}

void convert_normal(pvr_vec3_t* vout, const uint8_t* vin,
                    VertexAttribute type) {
    float* v = (float*)vin;
    switch(type) {
        case VERTEX_ATTRIBUTE_3F:
            vout->x = ((float*)v)[0];
            vout->y = ((float*)v)[1];
            vout->z = ((float*)v)[2];
            break;
        default:
            S_ERROR("{0}", type);
    }
}

static void convert_and_push(std::vector<PVRVertex>& buffer, const uint8_t* it,
                             const VertexSpecification& spec) {
    auto pos_off = spec.position_offset(false);
    auto uv_off = (spec.has_texcoord0())
                      ? spec.texcoord0_offset(false)
                      : 0; // FIXME: 0 assumes not first attribute
    auto color_off = (spec.has_diffuse()) ? spec.diffuse_offset(false) : 0;
    auto normal_off = (spec.has_normals()) ? spec.normal_offset(false) : 0;

    int i = buffer.size();
    buffer.push_back(PVRVertex());

    PVRVertex* v = &buffer[i];
    memset(v, 0, sizeof(PVRVertex));
    v->color = 0xFFFF;

    if(uv_off) {
        convert_uv(&v->uv, it + uv_off, spec.texcoord0_attribute);
    }

    if(color_off) {
        convert_color(&v->color, it + color_off, spec.diffuse_attribute);
    }

    if(normal_off) {
        convert_normal(&v->normal, it + normal_off, spec.normal_attribute);
    }

    convert_position(&v->xyz, it + pos_off, spec.position_attribute);
}

static std::vector<PVRVertex> buffer;

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

    pvr_vertex_pointers(&buffer[0].xyz, &buffer[0].uv, &buffer[0].color,
                        &buffer[0].color_offset, &buffer[0].normal,
                        &buffer[0].st, sizeof(PVRVertex));

    pvr_draw_arrays(
        PVR_PRIM_TRIANGLE_STRIP,
        0, buffer.size()
    );
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

    pvr_vertex_pointers(&buffer[0].xyz, &buffer[0].uv, &buffer[0].color,
                        &buffer[0].color_offset, &buffer[0].normal,
                        &buffer[0].st, sizeof(PVRVertex));

    pvr_draw_arrays(
        PVR_PRIM_TRIANGLES,
        0, buffer.size()
    );
}

void PVRRenderQueueVisitor::do_visit(const Renderable* renderable,
                                     const MaterialPass* material_pass,
                                     batcher::Iteration iteration) {
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
    auto projection = camera_->projection_matrix();

    const auto modelview = view * model;

    /* PVR uses an inverse coordinate system, so we need to flip
     * some things */
    projection[10] *= -1;
    projection[14] *= -1;

    pvr_set_matrix(PVR_MATRIX_MODE_MODELVIEW, (pvr_mat4_t*)modelview.data());
    pvr_set_matrix(PVR_MATRIX_MODE_PROJECTION, (pvr_mat4_t*) projection.data());

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


