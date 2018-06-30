//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef SIMULANT_GL_VERSION_3X

#include "generic_renderer.h"

#include "../../nodes/actor.h"
#include "../../stage.h"
#include "../../nodes/camera.h"
#include "../../nodes/light.h"
#include "../../partitioner.h"
#include "../../types.h"
#include "gpu_program.h"

#include "./glad/glad/glad.h"
#include "../../utils/gl_error.h"

namespace smlt {

class GL2RenderGroupImpl:
    public batcher::RenderGroupImpl,
    public std::enable_shared_from_this<GL2RenderGroupImpl> {

public:
    GL2RenderGroupImpl(RenderPriority priority):
        batcher::RenderGroupImpl(priority) {}

    GLuint texture_id[MAX_TEXTURE_UNITS];
    GPUProgramID shader_id;

    bool lt(const RenderGroupImpl& other) const override {
        const GL2RenderGroupImpl* rhs = dynamic_cast<const GL2RenderGroupImpl*>(&other);

        assert(rhs);

        if(!rhs) {
            // Should never happen... throw an error maybe?
            return false;
        }

        // Build a list of shader + texture ids, and return true if the
        // first non-equal id is less than the rhs equivalent

        for(uint32_t i = 0; i < MAX_TEXTURE_UNITS + 1; ++i) {
            if(i == 0) {
                if(shader_id.value() == rhs->shader_id.value()) {
                    continue;
                } else {
                    return shader_id.value() < rhs->shader_id.value();
                }
            } else {
                auto j = i - 1; // i is 1-based because of the shader check
                if(texture_id[j] == rhs->texture_id[j]) {
                    continue;
                }

                return texture_id[j] < rhs->texture_id[j];
            }
        }

        return false;
    }
};

batcher::RenderGroup GL4Renderer::new_render_group(Renderable* renderable, MaterialPass *material_pass) {
    auto impl = std::make_shared<GL2RenderGroupImpl>(renderable->render_priority());
    for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        if(i < material_pass->texture_unit_count()) {
            auto tex_id = material_pass->texture_unit(i).texture_id();
            impl->texture_id[i] = this->texture_objects_.at(tex_id);
        } else {
            impl->texture_id[i] = TextureID();
        }
    }
    impl->shader_id = material_pass->gpu_program_id();
    return batcher::RenderGroup(impl);
}

void GL4Renderer::set_light_uniforms(const MaterialPass* pass, GPUProgram* program, const Light *light) {
    auto& uniforms = pass->uniforms;

    if(uniforms->uses_auto(SP_AUTO_LIGHT_POSITION)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_POSITION);
        auto pos = (light) ? light->absolute_position() : Vec3();
        auto vec = (light) ? Vec4(pos, (light->type() == LIGHT_TYPE_DIRECTIONAL) ? 0.0 : 1.0) : Vec4();

        program->set_uniform_vec4(varname, vec);
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_AMBIENT)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_AMBIENT);
        program->set_uniform_colour(
            varname,
            (light) ? light->ambient() : Colour::NONE
        );
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_DIFFUSE)) {
        auto diffuse = (light) ? light->diffuse() : smlt::Colour::NONE;
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_DIFFUSE);
        program->set_uniform_colour(varname, diffuse);
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_SPECULAR)) {
        auto specular = (light) ? light->specular() : smlt::Colour::NONE;
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_SPECULAR);
        program->set_uniform_colour(varname, specular);
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_CONSTANT_ATTENUATION)) {
        auto att = (light) ? light->constant_attenuation() : 0;
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_CONSTANT_ATTENUATION);
        program->set_uniform_float(varname, att);
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_LINEAR_ATTENUATION)) {
        auto att = (light) ? light->linear_attenuation() : 0;
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_LINEAR_ATTENUATION);
        program->set_uniform_float(varname, att);
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION)) {
        auto att = (light) ? light->quadratic_attenuation() : 0;
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION);
        program->set_uniform_float(varname, att);
    }
}

void GL4Renderer::set_material_uniforms(const MaterialPass* pass, GPUProgram* program) {
    auto& uniforms = pass->uniforms;

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_AMBIENT)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_MATERIAL_AMBIENT);
        program->set_uniform_colour(varname, pass->ambient());
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_DIFFUSE)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_MATERIAL_DIFFUSE);
        program->set_uniform_colour(varname, pass->diffuse());
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_SPECULAR)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_MATERIAL_SPECULAR);
        program->set_uniform_colour(varname, pass->specular());
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_SHININESS)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_MATERIAL_SHININESS);
        program->set_uniform_float(varname, pass->shininess());
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_POINT_SIZE)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_MATERIAL_POINT_SIZE);
        program->set_uniform_float(varname, pass->point_size());
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS);
        program->set_uniform_int(varname, pass->texture_unit_count());
    }

    auto texture_matrix_auto = [](uint8_t which) -> ShaderAvailableAuto {
        switch(which) {
        case 0: return SP_AUTO_MATERIAL_TEX_MATRIX0;
        case 1: return SP_AUTO_MATERIAL_TEX_MATRIX1;
        case 2: return SP_AUTO_MATERIAL_TEX_MATRIX2;
        case 3: return SP_AUTO_MATERIAL_TEX_MATRIX3;
        case 4: return SP_AUTO_MATERIAL_TEX_MATRIX4;
        case 5: return SP_AUTO_MATERIAL_TEX_MATRIX5;
        case 6: return SP_AUTO_MATERIAL_TEX_MATRIX6;
        case 7: return SP_AUTO_MATERIAL_TEX_MATRIX7;
        default:
            throw std::logic_error("Invalid tex matrix index");
        }
    };

    for(uint8_t i = 0; i < pass->texture_unit_count(); ++i) {
        if(pass->uniforms->uses_auto(texture_matrix_auto(i))) {
            auto name = pass->uniforms->auto_variable_name(
                ShaderAvailableAuto(SP_AUTO_MATERIAL_TEX_MATRIX0 + i)
            );

            auto& unit = pass->texture_unit(i);
            program->set_uniform_mat4x4(name, unit.matrix());
        }
    }
}

void GL4Renderer::set_stage_uniforms(const MaterialPass *pass, GPUProgram *program, const Colour &global_ambient) {
    if(pass->uniforms->uses_auto(SP_AUTO_LIGHT_GLOBAL_AMBIENT)) {
        auto varname = pass->uniforms->auto_variable_name(SP_AUTO_LIGHT_GLOBAL_AMBIENT);
        program->set_uniform_colour(varname, global_ambient);
    }
}

void enable_vertex_attribute(uint8_t i) {
    GLCheck(glEnableVertexAttribArray, i);
}

void disable_vertex_attribute(uint8_t i) {
    GLCheck(glDisableVertexAttribArray, i);
}

template<typename EnabledMethod, typename OffsetMethod>
void send_attribute(ShaderAvailableAttributes attr,
                    const VertexSpecification& vertex_spec,
                    EnabledMethod exists_on_data_predicate,
                    OffsetMethod offset_func) {

    int32_t loc = (int32_t) attr;

    if((vertex_spec.*exists_on_data_predicate)()) {
        auto offset = (vertex_spec.*offset_func)(false);

        enable_vertex_attribute(loc);

        auto converted = convert(attr);
        auto attr_for_type = attribute_for_type(converted, vertex_spec);
        auto attr_size = vertex_attribute_size(attr_for_type);
        auto stride = vertex_spec.stride();

        GLCheck(glVertexAttribPointer,
            loc,
            attr_size / sizeof(float),
            GL_FLOAT,
            GL_FALSE,
            stride,
            BUFFER_OFFSET(offset)
        );
    } else {
        disable_vertex_attribute(loc);
        //L_WARN_ONCE(_u("Couldn't locate attribute on the mesh: {0}").format(attr));
    }
}

void GL4Renderer::prepare_vertex_array_object(const VertexSpecification& vertex_spec) {
    /*
     *  Binding attributes generically is hard. So we have some template magic in the send_attribute
     *  function above that takes the VertexData member functions we need to provide the attribute
     *  and just makes the whole thing generic. Before this was 100s of lines of boilerplate. Thank god
     *  for templates!
     */

    auto it = vertex_array_objects_.find(vertex_spec);
    if(it == vertex_array_objects_.end()) {
        GLuint vao;

        GLCheck(glGenVertexArrays, 1, &vao);
        GLCheck(glBindVertexArray, vao);

        vertex_array_objects_[vertex_spec] = vao;
    } else {
        GLCheck(glBindVertexArray, it->second);
    }
}

static void send_vertex_attributes(const VertexSpecification& vertex_spec) {
    send_attribute(SP_ATTR_VERTEX_POSITION, vertex_spec, &VertexSpecification::has_positions, &VertexSpecification::position_offset);
    send_attribute(SP_ATTR_VERTEX_DIFFUSE, vertex_spec, &VertexSpecification::has_diffuse, &VertexSpecification::diffuse_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD0, vertex_spec, &VertexSpecification::has_texcoord0, &VertexSpecification::texcoord0_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD1, vertex_spec, &VertexSpecification::has_texcoord1, &VertexSpecification::texcoord1_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD2, vertex_spec, &VertexSpecification::has_texcoord2, &VertexSpecification::texcoord2_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD3, vertex_spec, &VertexSpecification::has_texcoord3, &VertexSpecification::texcoord3_offset);
    send_attribute(SP_ATTR_VERTEX_NORMAL, vertex_spec, &VertexSpecification::has_normals, &VertexSpecification::normal_offset);
}

void GL4Renderer::set_blending_mode(BlendType type) {
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


std::shared_ptr<batcher::RenderQueueVisitor> GL4Renderer::get_render_queue_visitor(CameraPtr camera) {
    return std::make_shared<GL4RenderQueueVisitor>(this, camera);
}

smlt::GPUProgramID smlt::GL4Renderer::new_or_existing_gpu_program(const std::string &vertex_shader_source, const std::string &fragment_shader_source) {
    return program_manager_.make(GARBAGE_COLLECT_PERIODIC, vertex_shader_source, fragment_shader_source);
}

smlt::GPUProgramPtr smlt::GL4Renderer::gpu_program(const smlt::GPUProgramID &program_id) {
    return program_manager_.get(program_id).lock();
}

GL4RenderQueueVisitor::GL4RenderQueueVisitor(GL4Renderer* renderer, CameraPtr camera):
    renderer_(renderer),
    camera_(camera) {

}

void GL4RenderQueueVisitor::visit(Renderable* renderable, MaterialPass* material_pass, batcher::Iteration iteration) {
    queue_blended_objects_ = true;
    do_visit(renderable, material_pass, iteration);
}

void GL4RenderQueueVisitor::start_traversal(const batcher::RenderQueue& queue, uint64_t frame_id, Stage* stage) {
    global_ambient_ = stage->ambient_light();
}

void GL4RenderQueueVisitor::end_traversal(const batcher::RenderQueue &queue, Stage* stage) {
    // When running do_visit, don't queue blended objects just render them
    queue_blended_objects_ = false;

    // Should be ordered by distance to camera
    for(auto p: blended_object_queue_) {
        RenderState& state = p.second;

        if(state.render_group_impl != current_group_) {
            // Make sure we change render group (shaders, textures etc.)
            batcher::RenderGroup prev(current_group_->shared_from_this());
            batcher::RenderGroup next(state.render_group_impl->shared_from_this());

            change_render_group(&prev, &next);
            current_group_ = state.render_group_impl;            
        }

        if(pass_ != state.pass) {
            change_material_pass(pass_, state.pass);
        }

        // FIXME: Pass the previous light from the last iteration, not nullptr
        change_light(nullptr, state.light);

        // Render the transparent / blended objects
        do_visit(
            state.renderable,
            state.pass,
            state.iteration
        );
    }

    blended_object_queue_.clear();
    queue_blended_objects_ = true;
}

void GL4RenderQueueVisitor::change_light(const Light *prev, const Light *next) {
    light_ = next;

    renderer_->set_light_uniforms(pass_, program_, next);
}

void GL4RenderQueueVisitor::change_material_pass(const MaterialPass* prev, const MaterialPass* next) {
    pass_ = next;

    if(!prev || prev->depth_test_enabled() != next->depth_test_enabled()) {
        if(next->depth_test_enabled()) {
            GLCheck(glEnable, GL_DEPTH_TEST);
        } else {
            GLCheck(glDisable, GL_DEPTH_TEST);
        }
    }

    if(!prev || prev->depth_write_enabled() != next->depth_write_enabled()) {
        if(next->depth_write_enabled()) {
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

    if(!prev || prev->blending() != next->blending()) {
        renderer_->set_blending_mode(next->blending());
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

    /* Set any material properties on the gpu program */
    for(auto& p: next->material->properties()) {
        auto& name = p.first;
        const MaterialProperty& property = p.second;

        if(!property.is_set) {
            L_WARN_ONCE(_F("Property {0} was not set").format(name));
        }

        /* As properties apply across passes, we must not throw an error if the uniform variable
         * does not exist (as a single pass may not have a uniform) this is in contrast to automatic
         * uniforms which *do* throw as they are pass-specific */
        switch(property.type) {
        case MATERIAL_PROPERTY_TYPE_INT:
            program_->set_uniform_int(name, property.int_value, /* fail_silently= */true);
         break;
        case MATERIAL_PROPERTY_TYPE_FLOAT:
            program_->set_uniform_float(name, property.float_value, /* fail_silently= */true);
        break;
        default:
            throw std::runtime_error("UNIMPLEMENTED property type");
        }
    }

    rebind_attribute_locations_if_necessary(next, program_);
}

void GL4Renderer::set_renderable_uniforms(const MaterialPass* pass, GPUProgram* program, Renderable* renderable, Camera* camera) {
    //Calculate the modelview-projection matrix    
    const Mat4 model = renderable->final_transformation();
    const Mat4& view = camera->view_matrix();
    const Mat4& projection = camera->projection_matrix();

    Mat4 modelview = view * model;
    Mat4 modelview_projection = projection * modelview;

    if(pass->uniforms->uses_auto(SP_AUTO_VIEW_MATRIX)) {
        program->set_uniform_mat4x4(
            pass->uniforms->auto_variable_name(SP_AUTO_VIEW_MATRIX),
            view
        );
    }

    if(pass->uniforms->uses_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX)) {
        program->set_uniform_mat4x4(
            pass->uniforms->auto_variable_name(SP_AUTO_MODELVIEW_PROJECTION_MATRIX),
            modelview_projection
        );
    }

    if(pass->uniforms->uses_auto(SP_AUTO_MODELVIEW_MATRIX)) {
        program->set_uniform_mat4x4(
            pass->uniforms->auto_variable_name(SP_AUTO_MODELVIEW_MATRIX),
            modelview
        );
    }

    if(pass->uniforms->uses_auto(SP_AUTO_PROJECTION_MATRIX)) {
        program->set_uniform_mat4x4(
            pass->uniforms->auto_variable_name(SP_AUTO_PROJECTION_MATRIX),
            projection
        );
    }

    if(pass->uniforms->uses_auto(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX)) {
        Mat3 inverse_transpose_modelview(modelview);
        inverse_transpose_modelview.inverse();
        inverse_transpose_modelview.transpose();

        program->set_uniform_mat3x3(
            pass->uniforms->auto_variable_name(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX),
            inverse_transpose_modelview
        );
    }
}

void GL4RenderQueueVisitor::rebind_attribute_locations_if_necessary(const MaterialPass* pass, GPUProgram* program) {
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
}

void GL4RenderQueueVisitor::change_render_group(const batcher::RenderGroup *prev, const batcher::RenderGroup *next) {

    // Casting blindly because I can't see how it's possible that it's anything else!
    auto last_group = (prev) ? (GL2RenderGroupImpl*) prev->impl() : nullptr;
    current_group_ = (GL2RenderGroupImpl*) next->impl();

    // Active the new program, if this render group uses a different one
    if(!last_group || current_group_->shader_id != last_group->shader_id) {
        program_ = this->renderer_->gpu_program(current_group_->shader_id).get();
        program_->build();
        program_->activate();
    }

    // Set up the textures appropriately depending on the group textures
    for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        if(!last_group || last_group->texture_id[i] != current_group_->texture_id[i]) {

            GLCheck(glActiveTexture, GL_TEXTURE0 + i);
            GLCheck(glBindTexture, GL_TEXTURE_2D, current_group_->texture_id[i]);
        }
    }
}

void GL4RenderQueueVisitor::do_visit(Renderable* renderable, MaterialPass* material_pass, batcher::Iteration iteration) {
    // Queue transparent objects for render later
    if(material_pass->is_blended() && queue_blended_objects_) {
        auto pos = renderable->transformed_aabb().centre();
        auto plane = camera_->frustum().plane(FRUSTUM_PLANE_NEAR);

        float key = plane.distance_to(pos);

        RenderState state;
        state.renderable = renderable;
        state.pass = material_pass;
        state.light = light_;
        state.iteration = iteration;
        state.render_group_impl = current_group_;

        blended_object_queue_.insert(
            std::make_pair(key, state)
        );

        // We are done for now, we'll render this in back-to-front order later
        return;
    }

    // Don't bother doing *anything* if there is nothing to render
    if(!renderable->index_element_count()) {
        return;
    }

    /* Set up the VAO */
    renderer_->prepare_vertex_array_object(renderable->vertex_attribute_specification());

    renderer_->set_renderable_uniforms(material_pass, program_, renderable, camera_);

    renderable->prepare_buffers(renderer_);

    /* Now bind the real buffers */
    auto* vertex_buffer = renderable->vertex_attribute_buffer();
    auto* index_buffer = renderable->index_buffer();

    // Bind the buffers to the correct targets (purpose)
    vertex_buffer->bind(HARDWARE_BUFFER_VERTEX_ATTRIBUTES);
    index_buffer->bind(HARDWARE_BUFFER_VERTEX_ARRAY_INDICES);

    send_vertex_attributes(renderable->vertex_attribute_specification());

    renderer_->send_geometry(renderable);
}

static GLenum convert_index_type(IndexType type) {
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
    default:
        assert(0 && "Invalid mesh arrangement");
        return GL_TRIANGLES;
    }
}


void GL4Renderer::send_geometry(Renderable *renderable) {
    auto element_count = renderable->index_element_count();
    if(!element_count) {
        return;
    }

    auto index_type = convert_index_type(renderable->index_type());
    auto arrangement = renderable->arrangement();

    GLCheck(glDrawElements, convert_arrangement(arrangement), element_count, index_type, BUFFER_OFFSET(0));
    window->stats->increment_polygons_rendered(arrangement, element_count);
}

void GL4Renderer::init_context() {
    if(!gladLoadGL()) {
        throw std::runtime_error("Unable to intialize OpenGL 2.1");
    }

    GLCheck(glEnable, GL_DEPTH_TEST);
    GLCheck(glDepthFunc, GL_LEQUAL);
    GLCheck(glEnable, GL_CULL_FACE);
}


}

#endif //SIMULANT_GL_VERSION_3X
