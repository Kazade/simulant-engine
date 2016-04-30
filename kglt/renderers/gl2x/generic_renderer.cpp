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

/*
 * FIXME: Stupid argument ordering
 */
void GenericRenderer::set_auto_uniforms_on_shader(GPUProgramInstance& program,
    CameraID camera,
    Renderable &subactor) {

    //Calculate the modelview-projection matrix
    Mat4 modelview_projection;
    Mat4 modelview;

    const Mat4 model = subactor.final_transformation();
    const Mat4& view = window->camera(camera)->view_matrix();
    const Mat4& projection = window->camera(camera)->projection_matrix();

    kmMat4Multiply(&modelview, &view, &model);
    kmMat4Multiply(&modelview_projection, &projection, &modelview);

    if(program.uniforms->uses_auto(SP_AUTO_VIEW_MATRIX)) {
        program.program->set_uniform_mat4x4(
            program.uniforms->auto_variable_name(SP_AUTO_VIEW_MATRIX),
            view
        );
    }

    if(program.uniforms->uses_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX)) {
        program.program->set_uniform_mat4x4(
            program.uniforms->auto_variable_name(SP_AUTO_MODELVIEW_PROJECTION_MATRIX),
            modelview_projection
        );
    }

    if(program.uniforms->uses_auto(SP_AUTO_MODELVIEW_MATRIX)) {
        program.program->set_uniform_mat4x4(
            program.uniforms->auto_variable_name(SP_AUTO_MODELVIEW_MATRIX),
            modelview
        );
    }

    if(program.uniforms->uses_auto(SP_AUTO_PROJECTION_MATRIX)) {
        program.program->set_uniform_mat4x4(
            program.uniforms->auto_variable_name(SP_AUTO_PROJECTION_MATRIX),
            projection
        );
    }

    if(program.uniforms->uses_auto(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX)) {
        Mat3 inverse_transpose_modelview;

        kmMat4ExtractRotationMat3(&modelview, &inverse_transpose_modelview);
        kmMat3Inverse(&inverse_transpose_modelview, &inverse_transpose_modelview);
        kmMat3Transpose(&inverse_transpose_modelview, &inverse_transpose_modelview);

        program.program->set_uniform_mat3x3(
            program.uniforms->auto_variable_name(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX),
            inverse_transpose_modelview
        );
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

    switch(buffer.arrangement()) {
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

}
