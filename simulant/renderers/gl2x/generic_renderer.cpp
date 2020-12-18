//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "generic_renderer.h"

#include "../../nodes/actor.h"
#include "../../stage.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../partitioner.h"
#include "../../types.h"
#include "gpu_program.h"
#include "vbo_manager.h"

#include "../glad/glad/glad.h"
#include "../../utils/gl_error.h"
#include "../../window.h"

namespace smlt {

/* These are bare minimal shaders so that *something* is displayed, they're only used when the
 * Material is incomplete (no shaders provided) */

const std::string default_vertex_shader = R"(
#version 120
void main(void) {
    gl_Position = ftransform();
}
)";

const std::string default_fragment_shader = R"(
#version 120

void main(void) {
    gl_FragColor = vec4(0.4,0.4,0.8,1.0);
}

)";


struct GL2RenderGroupImpl {
    GLuint texture_id[MAX_TEXTURE_UNITS];
    GPUProgramID shader_id;
};

GPUProgramID GenericRenderer::default_gpu_program_id() const {
    return default_gpu_program_id_;
}

GenericRenderer::GenericRenderer(Window *window):
    Renderer(window),
    GLRenderer(window),
    buffer_manager_(VBOManager::create()) {

}

batcher::RenderGroupKey GenericRenderer::prepare_render_group(
    batcher::RenderGroup* group,
    const Renderable *renderable,
    const MaterialPass *material_pass,
    const uint8_t pass_number,
    const bool is_blended,
    const float distance_to_camera) {

    _S_UNUSED(group);
    _S_UNUSED(renderable);
    _S_UNUSED(material_pass);
    _S_UNUSED(pass_number);
    _S_UNUSED(is_blended);
    _S_UNUSED(distance_to_camera);

    return batcher::generate_render_group_key(
        pass_number,
        is_blended,
        distance_to_camera
    );
}

void GenericRenderer::set_light_uniforms(const MaterialPass* pass, GPUProgram* program, const LightPtr light) {
    auto pos_property = pass->property_value(LIGHT_POSITION_PROPERTY);
    auto amb_property = pass->property_value(LIGHT_AMBIENT_PROPERTY);
    auto diff_property = pass->property_value(LIGHT_DIFFUSE_PROPERTY);
    auto spec_property = pass->property_value(LIGHT_SPECULAR_PROPERTY);
    auto ca_property = pass->property_value(LIGHT_CONSTANT_ATTENUATION_PROPERTY);
    auto la_property = pass->property_value(LIGHT_LINEAR_ATTENUATION_PROPERTY);
    auto qa_property = pass->property_value(LIGHT_QUADRATIC_ATTENUATION_PROPERTY);

    auto pos_loc = program->locate_uniform(pos_property->shader_variable(), true);
    if(pos_loc > -1) {
        auto pos = (light) ? light->absolute_position() : Vec3();
        auto vec = (light) ? Vec4(pos, (light->type() == LIGHT_TYPE_DIRECTIONAL) ? 0.0 : 1.0) : Vec4();
        program->set_uniform_vec4(pos_loc, vec);
    }

    auto amb_loc = program->locate_uniform(amb_property->shader_variable(), true);
    if(amb_loc > -1) {
        program->set_uniform_colour(
            amb_loc,
            (light) ? light->ambient() : Colour::NONE
        );
    }

    auto diff_loc = program->locate_uniform(diff_property->shader_variable(), true);
    if(diff_loc > -1) {
        auto diffuse = (light) ? light->diffuse() : smlt::Colour::NONE;
        program->set_uniform_colour(diff_loc, diffuse);
    }

    auto spec_loc = program->locate_uniform(spec_property->shader_variable(), true);
    if(spec_loc > -1) {
        auto specular = (light) ? light->specular() : smlt::Colour::NONE;
        program->set_uniform_colour(spec_loc, specular);
    }

    auto ca_loc = program->locate_uniform(ca_property->shader_variable(), true);
    if(ca_loc > -1) {
        auto att = (light) ? light->constant_attenuation() : 0;
        program->set_uniform_float(ca_loc, att);
    }

    auto la_loc = program->locate_uniform(la_property->shader_variable(), true);
    if(la_loc > -1) {
        auto att = (light) ? light->linear_attenuation() : 0;
        program->set_uniform_float(la_loc, att);
    }

    auto qa_loc = program->locate_uniform(qa_property->shader_variable(), true);
    if(qa_loc > -1) {
        auto att = (light) ? light->quadratic_attenuation() : 0;
        program->set_uniform_float(qa_loc, att);
    }
}

void GenericRenderer::set_material_uniforms(const MaterialPass* pass, GPUProgram* program) {
    auto mat = pass->material();
    auto amb_property = pass->property_value(mat->material_ambient_id_);
    auto diff_property = pass->property_value(mat->material_diffuse_id_);
    auto spec_property = pass->property_value(mat->material_specular_id_);
    auto shin_property = pass->property_value(mat->material_shininess_id_);
    auto ps_property = pass->property_value(mat->point_size_id_);

    auto amb_loc = program->locate_uniform(amb_property->shader_variable(), true);
    if(amb_loc > -1) {
        auto varname = amb_property->shader_variable();
        program->set_uniform_colour(varname, pass->ambient());
    }

    auto diff_loc = program->locate_uniform(diff_property->shader_variable(), true);
    if(diff_loc > -1) {
        auto varname = diff_property->shader_variable();
        program->set_uniform_colour(varname, pass->diffuse());
    }

    auto spec_loc = program->locate_uniform(spec_property->shader_variable(), true);
    if(spec_loc > -1) {
        auto varname = spec_property->shader_variable();
        program->set_uniform_colour(varname, pass->specular());
    }

    auto shin_loc = program->locate_uniform(shin_property->shader_variable(), true);
    if(shin_loc > -1) {
        auto varname = shin_property->shader_variable();
        program->set_uniform_float(varname, pass->shininess());
    }

    auto ps_loc = program->locate_uniform(ps_property->shader_variable(), true);
    if(ps_loc > -1) {
        auto varname = ps_property->shader_variable();
        program->set_uniform_float(varname, pass->point_size());
    }

    /* Each texture property has a counterpart matrix, this passes those down if they exist */
    auto texture_props = pass->registry()->texture_properties();
    uint8_t texture_unit = 0;
    for(auto& tex_prop: texture_props) {
        auto prop = pass->property_value(tex_prop->id);

        auto tloc = program->locate_uniform(prop->shader_variable(), true);
        if(tloc > -1) {
            // This texture is being used
            program->set_uniform_int(tloc, texture_unit++);
        }

        auto shader_name = prop->shader_variable();
        shader_name += "_matrix";

        auto loc = program->locate_uniform(shader_name, true);
        if(loc > -1) {
            program->set_uniform_mat4x4(loc, prop->value<TextureUnit>().texture_matrix());
        }
    }
}

void GenericRenderer::set_stage_uniforms(const MaterialPass *pass, GPUProgram *program, const Colour &global_ambient) {
    _S_UNUSED(pass);

    auto varname = "s_global_ambient";
    auto loc = program->locate_uniform(varname, true);

    if(loc > -1) {
        program->set_uniform_colour(loc, global_ambient);
    }
}


/* Shadows GL state to avoid unnecessary GL calls */
static uint8_t enabled_vertex_attributes_ = 0;

void enable_vertex_attribute(uint8_t i) {
    uint8_t v = 1 << i;
    if((enabled_vertex_attributes_ & v) == v) {
        return;
    }

    GLCheck(glEnableVertexAttribArray, i);

    enabled_vertex_attributes_ ^= v;
}

void disable_vertex_attribute(uint8_t i) {
    uint8_t v = 1 << i;

    if((enabled_vertex_attributes_ & v) != v) {
        return;
    }

    GLCheck(glDisableVertexAttribArray, i);

    enabled_vertex_attributes_ ^= v;
}

template<typename EnabledMethod, typename OffsetMethod>
void send_attribute(int32_t loc,
                    VertexAttributeType attr,
                    const VertexSpecification& vertex_spec,
                    EnabledMethod exists_on_data_predicate,
                    OffsetMethod offset_func, uint32_t global_offset) {

    if(loc > -1 && (vertex_spec.*exists_on_data_predicate)()) {
        auto offset = (vertex_spec.*offset_func)(false);

        enable_vertex_attribute(loc);

        auto attr_for_type = attribute_for_type(attr, vertex_spec);
        auto attr_size = vertex_attribute_size(attr_for_type);
        auto stride = vertex_spec.stride();

        auto type = (attr_for_type == VERTEX_ATTRIBUTE_4UB) ? GL_UNSIGNED_BYTE :
                    (attr_for_type == VERTEX_ATTRIBUTE_PACKED_VEC4_1I) ? GL_UNSIGNED_INT_2_10_10_10_REV : GL_FLOAT;

        auto size = (attr_for_type == VERTEX_ATTRIBUTE_4UB) ? GL_BGRA :
                    (attr_for_type == VERTEX_ATTRIBUTE_PACKED_VEC4_1I) ? 4 : attr_size / sizeof(float);

        auto normalized = (attr_for_type == VERTEX_ATTRIBUTE_4UB) ? GL_TRUE : GL_FALSE;

        GLCheck(glVertexAttribPointer,
            loc,
            size,
            type,
            normalized,
            stride,
            BUFFER_OFFSET(global_offset + offset)
        );
    } else if(loc > -1){
        disable_vertex_attribute(loc);
        //L_WARN_ONCE(_u("Couldn't locate attribute on the mesh: {0}").format(attr));
    }
}

void GenericRenderer::set_auto_attributes_on_shader(GPUProgram* program, const Renderable* renderable, GPUBuffer* buffers) {
    /*
     *  Binding attributes generically is hard. So we have some template magic in the send_attribute
     *  function above that takes the VertexData member functions we need to provide the attribute
     *  and just makes the whole thing generic. Before this was 100s of lines of boilerplate. Thank god
     *  for templates!
     */
    const VertexSpecification& vertex_spec = renderable->vertex_data->vertex_specification();
    auto offset = buffers->vertex_vbo->byte_offset(buffers->vertex_vbo_slot);

    send_attribute(
        program->locate_attribute("s_position", true),
        VERTEX_ATTRIBUTE_TYPE_POSITION,
        vertex_spec,
        &VertexSpecification::has_positions,
        &VertexSpecification::position_offset,
        offset
    );

    send_attribute(
        program->locate_attribute("s_diffuse", true),
        VERTEX_ATTRIBUTE_TYPE_DIFFUSE,
        vertex_spec,
        &VertexSpecification::has_diffuse,
        &VertexSpecification::diffuse_offset,
        offset
    );

    send_attribute( program->locate_attribute("s_texcoord0", true),
                    VERTEX_ATTRIBUTE_TYPE_TEXCOORD0, vertex_spec,
                    &VertexSpecification::has_texcoord0,
                    &VertexSpecification::texcoord0_offset, offset);
    send_attribute(program->locate_attribute("s_texcoord1", true),
                   VERTEX_ATTRIBUTE_TYPE_TEXCOORD1, vertex_spec,
                   &VertexSpecification::has_texcoord1,
                   &VertexSpecification::texcoord1_offset, offset);
    send_attribute(program->locate_attribute("s_texcoord2", true),
                   VERTEX_ATTRIBUTE_TYPE_TEXCOORD2, vertex_spec,
                   &VertexSpecification::has_texcoord2,
                   &VertexSpecification::texcoord2_offset, offset);
    send_attribute(program->locate_attribute("s_texcoord3", true),
                   VERTEX_ATTRIBUTE_TYPE_TEXCOORD3, vertex_spec,
                   &VertexSpecification::has_texcoord3,
                   &VertexSpecification::texcoord3_offset, offset);
    send_attribute(program->locate_attribute("s_normal", true),
                   VERTEX_ATTRIBUTE_TYPE_NORMAL, vertex_spec,
                   &VertexSpecification::has_normals, &VertexSpecification::normal_offset, offset);
}

void GenericRenderer::set_blending_mode(BlendType type) {
    if(type == BLEND_NONE) {
        GLCheck(glDisable, GL_BLEND);
        return;
    }

    GLCheck(glEnable, GL_BLEND);
    switch(type) {
        case BLEND_ADD: GLCheck(glBlendFunc, GL_ONE, GL_ONE);
        break;
        case BLEND_ALPHA: GLCheck(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
        case BLEND_COLOUR: GLCheck(glBlendFunc, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
        break;
        case BLEND_MODULATE: GLCheck(glBlendFunc, GL_DST_COLOR, GL_ZERO);
        break;
        case BLEND_ONE_ONE_MINUS_ALPHA: GLCheck(glBlendFunc, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    default:
        throw std::logic_error("Invalid blend type specified");
    }
}


std::shared_ptr<batcher::RenderQueueVisitor> GenericRenderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<GL2RenderQueueVisitor>(this, camera);
}

smlt::GPUProgramID smlt::GenericRenderer::new_or_existing_gpu_program(const std::string &vertex_shader_source, const std::string &fragment_shader_source) {
    /* FIXME: This doesn't do what the function implies... it should either be called new_gpu_program, or it should try to return an existing progra
     * if the source matches */

    auto program = program_manager_.make(this, vertex_shader_source, fragment_shader_source);

    program_manager_.set_garbage_collection_method(program->id(), GARBAGE_COLLECT_PERIODIC);

    /* Build the GPU program on the main thread */
    if(cort::within_coroutine()) {
        window->idle->add_once([&]() {
            program->build();
        });

        /* Let the main routine do the build before
         * resuming */
        cort::yield_coroutine();
    } else {
        program->build();
    }

    return program->id();
}

smlt::GPUProgramPtr smlt::GenericRenderer::gpu_program(const smlt::GPUProgramID &program_id) const {
    return program_manager_.get(program_id);
}

GL2RenderQueueVisitor::GL2RenderQueueVisitor(GenericRenderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera) {

}

void GL2RenderQueueVisitor::visit(const Renderable* renderable, const MaterialPass* material_pass, batcher::Iteration iteration) {
    do_visit(renderable, material_pass, iteration);
}

void GL2RenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, Stage* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(frame_id);
    _S_UNUSED(stage);

    global_ambient_ = stage->ambient_light();
}

void GL2RenderQueueVisitor::end_traversal(const batcher::RenderQueue &queue, Stage* stage) {
    _S_UNUSED(queue);
    _S_UNUSED(stage);
}

void GL2RenderQueueVisitor::apply_lights(const LightPtr* lights, const uint8_t count) {
    if(count == 1) {
        renderer_->set_light_uniforms(pass_, program_, lights[0]);
    } else {
        // FIXME: This should fill out a light array in the shader. Needs a new property defined!
    }

}

void GL2RenderQueueVisitor::change_material_pass(const MaterialPass* prev, const MaterialPass* next) {
    pass_ = next;

    // Active the new program, if this render group uses a different one
    if(!prev || prev->gpu_program_id() != next->gpu_program_id()) {
        program_ = this->renderer_->gpu_program(pass_->gpu_program_id()).get();
        program_->build();
        program_->activate();
    }

    /* First we bind any used texture properties to their associated variables */
    uint8_t texture_unit = 0;
    for(auto& defined_property: pass_->material()->texture_properties()) {
        auto property_value = pass_->property_value(defined_property->id);

        // Do we use this texture property? Then bind the texture appropriately
        // FIXME: Is this right? The GL1 renderer uses the existence of a texture_id
        // to decide whether or not to bind a texture to a unit. This checks the variable exists
        // and also whether there's a texture ID. The question is, should the material have some other
        // type of existence check for texture properties? Is checking the texture_id right for all situations?
        // If someone uses s_diffuse_map, but doesn't set a value, surely that should get the default texture?
        auto loc = program_->locate_uniform(property_value->shader_variable(), true);
        if(loc > -1 && (texture_unit + 1u) < MAX_TEXTURE_UNITS) {
            const TextureUnit& unit = property_value->value<TextureUnit>();

            auto tex = unit.texture();
            GLCheck(glActiveTexture, GL_TEXTURE0 + texture_unit);
            GLCheck(glBindTexture, GL_TEXTURE_2D, (tex) ? tex->_renderer_specific_id() : 0);
            texture_unit++;
        }
    }

    /* Next, we wipe out any unused texture units */
    for(uint8_t i = texture_unit; i < MAX_TEXTURE_UNITS; ++i) {
        GLCheck(glActiveTexture, GL_TEXTURE0 + texture_unit);
        GLCheck(glBindTexture, GL_TEXTURE_2D, 0);
    }

    if(!prev || prev->is_depth_test_enabled() != next->is_depth_test_enabled()) {
        if(next->is_depth_test_enabled()) {
            GLCheck(glEnable, GL_DEPTH_TEST);
        } else {
            GLCheck(glDisable, GL_DEPTH_TEST);
        }
    }

    if(!prev || prev->is_depth_write_enabled() != next->is_depth_write_enabled()) {
        if(next->is_depth_write_enabled()) {
            GLCheck(glDepthMask, GL_TRUE);
        } else {
            GLCheck(glDepthMask, GL_FALSE);
        }
    }

    if(!prev || prev->point_size() != next->point_size()) {
        glPointSize(next->point_size());
    }

    if(!prev || prev->polygon_mode() != next->polygon_mode()) {
        switch(next->polygon_mode()) {
            case POLYGON_MODE_POINT:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
            case POLYGON_MODE_LINE:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
            default:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    if(!prev || prev->cull_mode() != next->cull_mode()) {
        if(next->cull_mode() != CULL_MODE_NONE) {
            glEnable(GL_CULL_FACE);
        }

        switch(next->cull_mode()) {
            case CULL_MODE_NONE:
                glDisable(GL_CULL_FACE);
            break;
            case CULL_MODE_FRONT_FACE:
                glCullFace(GL_FRONT);
            break;
            case CULL_MODE_BACK_FACE:
                glCullFace(GL_BACK);
            break;
            case CULL_MODE_FRONT_AND_BACK_FACE:
                glCullFace(GL_FRONT_AND_BACK);
            break;
        default:
            assert(0 && "Invalid cull mode");
        }
    }

    if(!prev || prev->blend_func() != next->blend_func()) {
        renderer_->set_blending_mode(next->blend_func());
    }

    if(!prev || prev->shade_model() != next->shade_model()) {
        if(next->shade_model() == SHADE_MODEL_SMOOTH) {
            GLCheck(glShadeModel, GL_SMOOTH);
        } else {
            GLCheck(glShadeModel, GL_FLAT);
        }
    }



    renderer_->set_stage_uniforms(next, program_, global_ambient_);
    renderer_->set_material_uniforms(next, program_);

    for(auto prop: next->material()->custom_properties()) {
        auto property_value = next->property_value(prop->id);

        switch(prop->type) {
        case MATERIAL_PROPERTY_TYPE_INT:
            program_->set_uniform_int(prop->name, property_value->value<int>(), /* fail_silently= */true);
        break;
        case MATERIAL_PROPERTY_TYPE_FLOAT:
            program_->set_uniform_float(prop->name, property_value->value<float>(), /* fail_silently= */true);
        break;
        case MATERIAL_PROPERTY_TYPE_TEXTURE:
            // Ignore, we handle textures separately
        break;
        default:
            throw std::runtime_error("UNIMPLEMENTED property type");
        }
    }

   // rebind_attribute_locations_if_necessary(next, program_);
}

void GenericRenderer::set_renderable_uniforms(const MaterialPass* pass, GPUProgram* program, const Renderable* renderable, Camera* camera) {
    //Calculate the modelview-projection matrix
    const Mat4 model = renderable->final_transformation;
    const Mat4& view = camera->view_matrix();
    const Mat4& projection = camera->projection_matrix();

    Mat4 modelview = view * model;
    Mat4 modelview_projection = projection * modelview;

    auto v_prop = pass->property_value(VIEW_MATRIX_PROPERTY);
    auto mvp_prop = pass->property_value(MODELVIEW_PROJECTION_MATRIX_PROPERTY);
    auto mv_prop = pass->property_value(MODELVIEW_MATRIX_PROPERTY);
    auto p_prop = pass->property_value(PROJECTION_MATRIX_PROPERTY);
    auto itmv_prop = pass->property_value(INVERSE_TRANSPOSE_MODELVIEW_MATRIX_PROPERTY);

    auto v_loc = program->locate_uniform(v_prop->shader_variable(), true);
    if(v_loc > -1) {
        program->set_uniform_mat4x4(
            v_prop->shader_variable(),
            view
        );
    }

    auto mvp_loc = program->locate_uniform(mvp_prop->shader_variable(), true);
    if(mvp_loc > -1) {
        program->set_uniform_mat4x4(
            mvp_prop->shader_variable(),
            modelview_projection
        );
    }

    auto mv_loc = program->locate_uniform(mv_prop->shader_variable(), true);
    if(mv_loc > -1) {
        program->set_uniform_mat4x4(
            mv_prop->shader_variable(),
            modelview
        );
    }

    auto p_loc = program->locate_uniform(p_prop->shader_variable(), true);
    if(p_loc > -1) {
        program->set_uniform_mat4x4(
            p_prop->shader_variable(),
            projection
        );
    }

    auto itmv_loc = program->locate_uniform(itmv_prop->shader_variable(), true);
    if(itmv_loc > -1) {
        // PERF: Recalculating every frame will be costly!
        Mat3 inverse_transpose_modelview(modelview);
        inverse_transpose_modelview.inverse();
        inverse_transpose_modelview.transpose();

        program->set_uniform_mat3x3(
            itmv_prop->shader_variable(),
            inverse_transpose_modelview
        );
    }
}

/*
void GL2RenderQueueVisitor::rebind_attribute_locations_if_necessary(const MaterialPass* pass, GPUProgram* program) {
    static const std::set<ShaderAvailableAttributes> SHADER_AVAILABLE_ATTRS = {
        SP_ATTR_VERTEX_POSITION,
        SP_ATTR_VERTEX_DIFFUSE,
        SP_ATTR_VERTEX_NORMAL,
        SP_ATTR_VERTEX_TEXCOORD0,
        SP_ATTR_VERTEX_TEXCOORD1,
        SP_ATTR_VERTEX_TEXCOORD2,
        SP_ATTR_VERTEX_TEXCOORD3,
    };

    for(auto attribute: SHADER_AVAILABLE_ATTRS) {
        if(pass->attributes->uses_auto(attribute)) {
            auto varname = pass->attributes->variable_name(attribute);
            program->set_attribute_location(varname, attribute);
        }
    }

    program->relink(); // Will only do somethig if set_attribute_location did something
}*/

void GL2RenderQueueVisitor::change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next) {
    _S_UNUSED(prev);
    _S_UNUSED(next);
}

void GL2RenderQueueVisitor::do_visit(const Renderable* renderable, const MaterialPass* material_pass, batcher::Iteration iteration) {
    _S_UNUSED(iteration);

    renderer_->set_renderable_uniforms(material_pass, program_, renderable, camera_);
    renderer_->prepare_to_render(renderable);
    renderer_->set_auto_attributes_on_shader(program_, renderable, renderer_->buffer_stash_.get());
    renderer_->send_geometry(renderable, renderer_->buffer_stash_.get());
}

static GLenum convert_id_type(IndexType type) {
    switch(type) {
    case INDEX_TYPE_8_BIT: return GL_UNSIGNED_BYTE;
    case INDEX_TYPE_16_BIT: return GL_UNSIGNED_SHORT;
    case INDEX_TYPE_32_BIT: return GL_UNSIGNED_INT;
    default:
        return GL_UNSIGNED_SHORT;
    }
}

static GLenum convert_arrangement(MeshArrangement arrangement) {
    switch(arrangement) {
    case MESH_ARRANGEMENT_LINES:
        return GL_LINES;
    case MESH_ARRANGEMENT_LINE_STRIP:
        return GL_LINE_STRIP;
    case MESH_ARRANGEMENT_TRIANGLES:
        return GL_TRIANGLES;
    case MESH_ARRANGEMENT_TRIANGLE_STRIP:
        return GL_TRIANGLE_STRIP;
    case MESH_ARRANGEMENT_TRIANGLE_FAN:
        return GL_TRIANGLE_FAN;
    case MESH_ARRANGEMENT_QUADS:
        return GL_QUADS;
    default:
        assert(0 && "Invalid mesh arrangement");
        return GL_TRIANGLES;
    }
}

GPUProgramID GenericRenderer::current_gpu_program_id() const {
    GLint id;
    GLCheck(glGetIntegerv, GL_CURRENT_PROGRAM, &id);

    GPUProgramID ret;

    program_manager_.each([&](uint32_t, GPUProgramPtr program) {
        if(program->program_object() == (GLuint) id) {
            ret = program->id();
        }
    });

    return ret;
}

void GenericRenderer::send_geometry(const Renderable *renderable, GPUBuffer *buffers) {
    auto element_count = renderable->index_element_count;
    auto index_type = convert_id_type(renderable->index_data->index_type());
    auto arrangement = renderable->arrangement;

    auto offset = buffers->index_vbo->byte_offset(buffers->index_vbo_slot);

    GLCheck(glDrawElements, convert_arrangement(arrangement), element_count, index_type, BUFFER_OFFSET(offset));
    window->stats->increment_polygons_rendered(arrangement, element_count);
}

void GenericRenderer::init_context() {
    if(!gladLoadGL()) {
        throw std::runtime_error("Unable to intialize OpenGL 2.1");
    }

    const GLubyte* GL_vendor = glGetString(GL_VENDOR);
    const GLubyte* GL_renderer = glGetString(GL_RENDERER);
    const GLubyte* GL_version = glGetString(GL_VERSION);
    const GLubyte* GL_extensions = glGetString(GL_EXTENSIONS);

    std::cout << "\n\nOpenGL Information:\n\n";
    std::cout << _F("\tVendor: {0}\n").format(GL_vendor);
    std::cout << _F("\tRenderer: {0}\n").format(GL_renderer);
    std::cout << _F("\tVersion: {0}\n\n").format(GL_version);
    std::cout << _F("\tExtensions: {0}\n\n").format(GL_extensions);

    GLCheck(glEnable, GL_DEPTH_TEST);
    GLCheck(glDepthFunc, GL_LEQUAL);
    GLCheck(glEnable, GL_CULL_FACE);

    if(!default_gpu_program_id_) {
        default_gpu_program_id_ = new_or_existing_gpu_program(default_vertex_shader, default_fragment_shader);
    }
}

void GenericRenderer::prepare_to_render(const Renderable *renderable) {
    /* Here we allocate VBOs for the renderable if necessary, and then upload
     * any new data */
    buffer_stash_.reset(new GPUBuffer(buffer_manager_->update_and_fetch_buffers(renderable)));
    buffer_stash_->bind_vbos();
}


}
