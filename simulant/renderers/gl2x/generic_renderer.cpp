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

#ifdef SIMULANT_GL_VERSION_2X

#include "generic_renderer.h"

#include "../../nodes/actor.h"
#include "../../stage.h"
#include "../../camera.h"
#include "../../nodes/light.h"
#include "../../partitioner.h"
#include "../../types.h"
#include "gpu_program.h"

#include "./glad/glad/glad.h"
#include "../../utils/gl_error.h"

namespace smlt {

class GL2RenderGroupImpl: public batcher::RenderGroupImpl {
public:
    GL2RenderGroupImpl(RenderPriority priority):
        batcher::RenderGroupImpl(priority) {}

    TextureID texture_id[MAX_TEXTURE_UNITS] = {TextureID()};
    ShaderID shader_id;

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
                if(texture_id[j].value() == rhs->texture_id[j].value()) {
                    continue;
                }

                return texture_id[j].value() < rhs->texture_id[j].value();
            }
        }

        return false;
    }
};

batcher::RenderGroup GenericRenderer::new_render_group(Renderable* renderable, MaterialPass *material_pass) {
    auto impl = std::make_shared<GL2RenderGroupImpl>(renderable->render_priority());
    for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        if(i < material_pass->texture_unit_count()) {
            auto tex_id = material_pass->texture_unit(i).texture_id();
            impl->texture_id[i] = tex_id;
        } else {
            impl->texture_id[i] = TextureID();
        }
    }
    impl->shader_id = material_pass->program->program->id();
    return batcher::RenderGroup(impl);
}

void GenericRenderer::set_light_uniforms(GPUProgramInstance *program_instance, Light *light) {
    auto& program = program_instance->program;
    auto& uniforms = program_instance->uniforms;

    if(uniforms->uses_auto(SP_AUTO_LIGHT_POSITION)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_POSITION);
        program->set_uniform_vec4(
            varname,
            Vec4(light->absolute_position(), (light->type() == LIGHT_TYPE_DIRECTIONAL) ? 0.0 : 1.0)
        );
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_AMBIENT)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_AMBIENT);
        program->set_uniform_colour(varname, light->ambient());
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_DIFFUSE)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_DIFFUSE);
        program->set_uniform_colour(varname, light->diffuse());
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_SPECULAR)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_SPECULAR);
        program->set_uniform_colour(varname, light->specular());
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_CONSTANT_ATTENUATION)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_CONSTANT_ATTENUATION);
        program->set_uniform_float(varname, light->constant_attenuation());
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_LINEAR_ATTENUATION)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_LINEAR_ATTENUATION);
        program->set_uniform_float(varname, light->linear_attenuation());
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION)) {
        auto varname = uniforms->auto_variable_name(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION);
        program->set_uniform_float(varname, light->quadratic_attenuation());
    }
}

void GenericRenderer::set_material_uniforms(GPUProgramInstance* program_instance, MaterialPass* pass) {
    auto& uniforms = program_instance->uniforms;
    auto& program = program_instance->program;

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

}

/*
 * FIXME: Stupid argument ordering
 */
void GenericRenderer::set_auto_uniforms_on_shader(GPUProgramInstance* program,
    CameraPtr camera,
    Renderable* subactor, const smlt::Colour& global_ambient) {

    //Calculate the modelview-projection matrix
    Mat4 modelview_projection;
    Mat4 modelview;

    const Mat4 model = subactor->final_transformation();
    const Mat4& view = camera->view_matrix();
    const Mat4& projection = camera->projection_matrix();

    kmMat4Multiply(&modelview, &view, &model);
    kmMat4Multiply(&modelview_projection, &projection, &modelview);

    if(program->uniforms->uses_auto(SP_AUTO_VIEW_MATRIX)) {
        program->program->set_uniform_mat4x4(
            program->uniforms->auto_variable_name(SP_AUTO_VIEW_MATRIX),
            view
        );
    }

    if(program->uniforms->uses_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX)) {
        program->program->set_uniform_mat4x4(
            program->uniforms->auto_variable_name(SP_AUTO_MODELVIEW_PROJECTION_MATRIX),
            modelview_projection
        );
    }

    if(program->uniforms->uses_auto(SP_AUTO_MODELVIEW_MATRIX)) {
        program->program->set_uniform_mat4x4(
            program->uniforms->auto_variable_name(SP_AUTO_MODELVIEW_MATRIX),
            modelview
        );
    }

    if(program->uniforms->uses_auto(SP_AUTO_PROJECTION_MATRIX)) {
        program->program->set_uniform_mat4x4(
            program->uniforms->auto_variable_name(SP_AUTO_PROJECTION_MATRIX),
            projection
        );
    }

    if(program->uniforms->uses_auto(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX)) {
        Mat3 inverse_transpose_modelview;

        kmMat4ExtractRotationMat3(&modelview, &inverse_transpose_modelview);
        kmMat3Inverse(&inverse_transpose_modelview, &inverse_transpose_modelview);
        kmMat3Transpose(&inverse_transpose_modelview, &inverse_transpose_modelview);

        program->program->set_uniform_mat3x3(
            program->uniforms->auto_variable_name(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX),
            inverse_transpose_modelview
        );
    }

    if(program->uniforms->uses_auto(SP_AUTO_LIGHT_GLOBAL_AMBIENT)) {
        auto varname = program->uniforms->auto_variable_name(SP_AUTO_LIGHT_GLOBAL_AMBIENT);
        program->program->set_uniform_colour(varname, global_ambient);
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
void send_attribute(ShaderAvailableAttributes attr,
                    const VertexSpecification& vertex_spec,
                    EnabledMethod exists_on_data_predicate,
                    OffsetMethod offset_func) {

    int32_t loc = (int32_t) attr;

    auto get_has_attribute = std::bind(exists_on_data_predicate, vertex_spec);

    if(get_has_attribute()) {
        auto offset = std::bind(offset_func, vertex_spec, false)();

        enable_vertex_attribute(loc);

        auto attr_size = vertex_attribute_size(attribute_for_type(convert(attr), vertex_spec));
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

void GenericRenderer::set_auto_attributes_on_shader(Renderable &buffer) {
    /*
     *  Binding attributes generically is hard. So we have some template magic in the send_attribute
     *  function above that takes the VertexData member functions we need to provide the attribute
     *  and just makes the whole thing generic. Before this was 100s of lines of boilerplate. Thank god
     *  for templates!
     */        
    const VertexSpecification& vertex_spec = buffer.vertex_attribute_specification();

    send_attribute(SP_ATTR_VERTEX_POSITION, vertex_spec, &VertexSpecification::has_positions, &VertexSpecification::position_offset);
    send_attribute(SP_ATTR_VERTEX_DIFFUSE, vertex_spec, &VertexSpecification::has_diffuse, &VertexSpecification::diffuse_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD0, vertex_spec, &VertexSpecification::has_texcoord0, &VertexSpecification::texcoord0_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD1, vertex_spec, &VertexSpecification::has_texcoord1, &VertexSpecification::texcoord1_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD2, vertex_spec, &VertexSpecification::has_texcoord2, &VertexSpecification::texcoord2_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD3, vertex_spec, &VertexSpecification::has_texcoord3, &VertexSpecification::texcoord3_offset);
    send_attribute(SP_ATTR_VERTEX_NORMAL, vertex_spec, &VertexSpecification::has_normals, &VertexSpecification::normal_offset);
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

void GenericRenderer::render(CameraPtr camera, bool render_group_changed, const batcher::RenderGroup* current_group,
    Renderable* renderable, MaterialPass* material_pass, Light* light, const Colour &global_ambient, batcher::Iteration iteration) {

    // Casting blindly because I can't see how it's possible that it's anything else!
    GL2RenderGroupImpl* current = (GL2RenderGroupImpl*) current_group->impl();
    ResourceManager& resource_manager = material_pass->material->resource_manager();

    static ShaderID last_shader_id;

    if(render_group_changed) {       
        if(material_pass->program->program->id() != last_shader_id) {
            material_pass->program->program->build();
            material_pass->program->program->activate();

            last_shader_id = material_pass->program->program->id();
        }

        for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
            GLCheck(glActiveTexture, GL_TEXTURE0 + i);
            if(current->texture_id[i]) {
                auto texture = resource_manager.texture(current->texture_id[i]);
                GLCheck(glBindTexture, GL_TEXTURE_2D, texture->gl_tex());
            } else {
                GLCheck(glBindTexture, GL_TEXTURE_2D, 0);
            }
        }
    }

    // Don't bother doing *anything* if there is nothing to render
    if(!renderable->index_element_count()) {
        return;
    }

    auto& program_instance = material_pass->program;
    auto& program = program_instance->program;

    set_auto_uniforms_on_shader(program_instance.get(), camera, renderable, global_ambient);
    set_material_uniforms(program_instance.get(), material_pass);

    if(light) {
        set_light_uniforms(program_instance.get(), light);
    }

    for(auto& p: material_pass->staged_float_uniforms()) {
        program->set_uniform_float(p.first, p.second);
    }

    for(auto& p: material_pass->staged_int_uniforms()) {
        program->set_uniform_int(p.first, p.second);
    }

    renderable->prepare_buffers();

    auto* vertex_buffer = renderable->vertex_attribute_buffer();
    auto* index_buffer = renderable->index_buffer();

    // Bind the buffers to the correct targets (purpose)
    vertex_buffer->bind(HARDWARE_BUFFER_VERTEX_ATTRIBUTES);
    index_buffer->bind(HARDWARE_BUFFER_VERTEX_ARRAY_INDICES);

    set_auto_attributes_on_shader(*renderable);

    if(material_pass->depth_test_enabled()) {
        GLCheck(glEnable, GL_DEPTH_TEST);
    } else {
        GLCheck(glDisable, GL_DEPTH_TEST);
    }

    if(material_pass->depth_write_enabled()) {
        GLCheck(glDepthMask, GL_TRUE);
    } else {
        GLCheck(glDepthMask, GL_FALSE);
    }

    glPointSize(material_pass->point_size());

    switch(material_pass->polygon_mode()) {
        case POLYGON_MODE_POINT:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
        case POLYGON_MODE_LINE:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
        default:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if(material_pass->cull_mode() != CULL_MODE_NONE) {
        glEnable(GL_CULL_FACE);
    }

    switch(material_pass->cull_mode()) {
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

    for(uint8_t i = 0; i < material_pass->texture_unit_count(); ++i) {
        if(program_instance->uniforms->uses_auto(texture_matrix_auto(i))) {
            auto name = program_instance->uniforms->auto_variable_name(
                ShaderAvailableAuto(SP_AUTO_MATERIAL_TEX_MATRIX0 + i)
            );

            auto& unit = material_pass->texture_unit(i);
            program->set_uniform_mat4x4(name, unit.matrix());
        }
    }

    set_blending_mode(material_pass->blending());
    send_geometry(renderable);
}

void GenericRenderer::send_geometry(Renderable *renderable) {
    std::size_t index_count = renderable->index_element_count();
    if(!index_count) {
        return;
    }

    switch(renderable->arrangement()) {
        case MESH_ARRANGEMENT_POINTS:
            GLCheck(glDrawElements, GL_POINTS, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_LINES:
            GLCheck(glDrawElements, GL_LINES, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_LINE_STRIP:
            GLCheck(glDrawElements, GL_LINE_STRIP, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLES:
            GLCheck(glDrawElements, GL_TRIANGLES, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLE_STRIP:
            GLCheck(glDrawElements, GL_TRIANGLE_STRIP, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLE_FAN:
            GLCheck(glDrawElements, GL_TRIANGLE_FAN, index_count, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        break;
        default:
            L_DEBUG("Tried to render a mesh with an invalid arrangement");
    }
}

void GenericRenderer::init_context() {
    if(!gladLoadGL()) {
        throw std::runtime_error("Unable to intialize OpenGL 2.1");
    }

    GLCheck(glEnable, GL_DEPTH_TEST);
    GLCheck(glDepthFunc, GL_LEQUAL);
    GLCheck(glEnable, GL_CULL_FACE);
}


/*
void GenericRenderer::render(Renderable& buffer, CameraID camera, GPUProgramInstance *program) {

    if(!program) {
        L_ERROR("No shader is bound, so nothing will be rendered");
        return;
    }

    std::size_t index_count = buffer.index_data->count();
    if(!index_count) {
        return;
    }

    GLStateStash s2(GL_ELEMENT_ARRAY_BUFFER_BINDING);
    GLStateStash s3(GL_ARRAY_BUFFER_BINDING);

    buffer._update_vertex_array_object();
    buffer._bind_vertex_array_object();

    //Attributes don't change per-iteration of a pass
    set_auto_attributes_on_shader(buffer);
    set_auto_uniforms_on_shader(*program, camera, buffer);

}*/

}

#endif //SIMULANT_GL_VERSION_2X
