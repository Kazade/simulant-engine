#include "generic_renderer.h"

#include "../../actor.h"
#include "../../stage.h"
#include "../../camera.h"
#include "../../light.h"
#include "../../partitioner.h"
#include "../../gpu_program.h"

#include "kazmath/mat4.h"
#include "../../utils/glcompat.h"
#include "../../utils/gl_error.h"

namespace kglt {

class GL2RenderGroupImpl: public new_batcher::RenderGroupImpl {
public:
    GL2RenderGroupImpl(RenderPriority priority):
        new_batcher::RenderGroupImpl(priority) {}

    GLuint texture_id[MAX_TEXTURE_UNITS] = {0};
    GLuint program_object;

    bool lt(const RenderGroupImpl& other) const override {
        const GL2RenderGroupImpl* rhs = dynamic_cast<const GL2RenderGroupImpl*>(&other);
        if(!rhs) {
            // Should never happen... throw an error maybe?
            return false;
        }

        for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
            if(texture_id[i] < rhs->texture_id[i]) {
                return true;
            }
        }

        if(program_object < rhs->program_object) {
            return true;
        }

        return false;
    }
};

new_batcher::RenderGroup GenericRenderer::new_render_group(Renderable* renderable, MaterialPass *material_pass) {
    auto impl = std::make_shared<GL2RenderGroupImpl>(renderable->render_priority());
    for(uint32_t i = 0; i < material_pass->texture_unit_count(); ++i) {
        auto tex_id = material_pass->texture_unit(i).texture_id();
        auto gltex = material_pass->material->resource_manager().texture(tex_id)->gl_tex();
        impl->texture_id[i] = gltex;
    }
    impl->program_object = material_pass->program->program->program_object();
    return new_batcher::RenderGroup(impl);
}

/*
 * FIXME: Stupid argument ordering
 */
void GenericRenderer::set_auto_uniforms_on_shader(GPUProgramInstance* program,
    CameraPtr camera,
    Renderable* subactor, const kglt::Colour& global_ambient) {

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

template<typename EnabledMethod, typename OffsetMethod>
void send_attribute(ShaderAvailableAttributes attr,
                    const VertexData& data,
                    EnabledMethod exists_on_data_predicate,
                    OffsetMethod offset_func) {

    int32_t loc = (int32_t) attr;

    auto get_has_attribute = std::bind(exists_on_data_predicate, std::reference_wrapper<const VertexData>(data));

    if(get_has_attribute()) {
        auto offset = std::bind(offset_func, std::reference_wrapper<const VertexData>(data))();

        GLCheck(glEnableVertexAttribArray, loc);
        GLCheck(glVertexAttribPointer,
            loc,
            SHADER_ATTRIBUTE_SIZES.find(attr)->second,
            GL_FLOAT,
            GL_FALSE,
            data.stride(),
            BUFFER_OFFSET(offset)
        );
    } else {
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
    send_attribute(SP_ATTR_VERTEX_POSITION, buffer.vertex_data(), &VertexData::has_positions, &VertexData::position_offset);
    send_attribute(SP_ATTR_VERTEX_DIFFUSE, buffer.vertex_data(), &VertexData::has_diffuse, &VertexData::diffuse_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD0, buffer.vertex_data(), &VertexData::has_texcoord0, &VertexData::texcoord0_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD1, buffer.vertex_data(), &VertexData::has_texcoord1, &VertexData::texcoord1_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD2, buffer.vertex_data(), &VertexData::has_texcoord2, &VertexData::texcoord2_offset);
    send_attribute(SP_ATTR_VERTEX_TEXCOORD3, buffer.vertex_data(), &VertexData::has_texcoord3, &VertexData::texcoord3_offset);
    send_attribute(SP_ATTR_VERTEX_NORMAL, buffer.vertex_data(), &VertexData::has_normals, &VertexData::normal_offset);
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
        throw ValueError("Invalid blend type specified");
    }
}

void GenericRenderer::render(CameraPtr camera, StagePtr stage, const new_batcher::RenderGroup* current_group, const new_batcher::RenderGroup* last_group,
    Renderable* renderable, MaterialPass* material_pass, new_batcher::Iteration iteration) {

    static GPUProgramInstance* program_instance = nullptr;

    // Casting blindly because I can't see how it's possible that it's anything else!
    GL2RenderGroupImpl* last = (last_group) ? (GL2RenderGroupImpl*) last_group->impl() : nullptr;
    GL2RenderGroupImpl* current = (GL2RenderGroupImpl*) current_group->impl();

    for(uint32_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        if(!last || last->texture_id[i] != current->texture_id[i]) {
            GLCheck(glActiveTexture, GL_TEXTURE0 + i);
            GLCheck(glBindTexture, GL_TEXTURE_2D, current->texture_id[i]);
        }
    }

    // Active the shader if it changed since last time
    if(!last || last->program_object != current->program_object) {
        program_instance = material_pass->program.get();
        program_instance->program->build();
        program_instance->program->activate();
        assert(program_instance);
    }

    assert(program_instance);

    auto& program = program_instance->program;

    set_auto_uniforms_on_shader(program_instance, camera, renderable, stage->ambient_light());

    for(auto attribute: SHADER_AVAILABLE_ATTRS) {
        if(program_instance->attributes->uses_auto(attribute)) {
            auto varname = program_instance->attributes->variable_name(attribute);
            program->set_attribute_location(varname, attribute);
        }
    }

    for(auto& p: material_pass->staged_float_uniforms()) {
        program->set_uniform_float(p.first, p.second);
    }

    for(auto& p: material_pass->staged_int_uniforms()) {
        program->set_uniform_int(p.first, p.second);
    }

    program->relink();

    renderable->_update_vertex_array_object();
    renderable->_bind_vertex_array_object();

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

    auto texture_matrix_auto = [](uint8_t which) -> ShaderAvailableAuto {
        switch(which) {
        case 0: return SP_AUTO_MATERIAL_TEX_MATRIX0;
        case 1: return SP_AUTO_MATERIAL_TEX_MATRIX1;
        case 2: return SP_AUTO_MATERIAL_TEX_MATRIX2;
        case 3: return SP_AUTO_MATERIAL_TEX_MATRIX3;
        default:
            throw ValueError("Invalid tex matrix index");
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
    std::size_t index_count = renderable->index_data().count();
    if(!index_count) {
        return;
    }

    switch(renderable->arrangement()) {
        case MESH_ARRANGEMENT_POINTS:
            GLCheck(glDrawElements, GL_POINTS, index_count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_LINES:
            GLCheck(glDrawElements, GL_LINES, index_count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_LINE_STRIP:
            GLCheck(glDrawElements, GL_LINE_STRIP, index_count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLES:
            GLCheck(glDrawElements, GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLE_STRIP:
            GLCheck(glDrawElements, GL_TRIANGLE_STRIP, index_count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLE_FAN:
            GLCheck(glDrawElements, GL_TRIANGLE_FAN, index_count, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        default:
            throw NotImplementedError(__FILE__, __LINE__);
    }
}

/*
void GenericRenderer::render(Renderable& buffer, CameraID camera, GPUProgramInstance *program) {

    if(!program) {
        L_ERROR("No shader is bound, so nothing will be rendered");
        return;
    }

    std::size_t index_count = buffer.index_data().count();
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
