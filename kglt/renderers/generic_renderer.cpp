#include "generic_renderer.h"

#include "../actor.h"
#include "../stage.h"
#include "../camera.h"
#include "../light.h"
#include "../partitioner.h"

#include "kazmath/mat4.h"
#include "../utils/glcompat.h"
#include "../utils/gl_error.h"
#include "../utils/vao_abstraction.h"

namespace kglt {

/*
 * FIXME: Stupid argument ordering
 */
void GenericRenderer::set_auto_uniforms_on_shader(GPUProgram& program,
    CameraID camera,
    Renderable &subactor) {

    //Calculate the modelview-projection matrix
    Mat4 modelview_projection;
    Mat4 modelview;

    const Mat4 model = subactor.final_transformation();
    const Mat4& view = window().camera(camera)->view_matrix();
    const Mat4& projection = window().camera(camera)->projection_matrix();

    kmMat4Multiply(&modelview, &view, &model);
    kmMat4Multiply(&modelview_projection, &projection, &modelview);

    if(program.uniforms().uses_auto(SP_AUTO_VIEW_MATRIX)) {
        program.uniforms().set_mat4x4(
            program.uniforms().auto_variable_name(SP_AUTO_VIEW_MATRIX),
            view
        );
    }

    if(program.uniforms().uses_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX)) {
        program.uniforms().set_mat4x4(
            program.uniforms().auto_variable_name(SP_AUTO_MODELVIEW_PROJECTION_MATRIX),
            modelview_projection
        );
    }

    if(program.uniforms().uses_auto(SP_AUTO_MODELVIEW_MATRIX)) {
        program.uniforms().set_mat4x4(
            program.uniforms().auto_variable_name(SP_AUTO_MODELVIEW_MATRIX),
            modelview
        );
    }

    if(program.uniforms().uses_auto(SP_AUTO_PROJECTION_MATRIX)) {
        program.uniforms().set_mat4x4(
            program.uniforms().auto_variable_name(SP_AUTO_PROJECTION_MATRIX),
            projection
        );
    }

    if(program.uniforms().uses_auto(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX)) {
        Mat3 inverse_transpose_modelview;

        kmMat3AssignMat4(&inverse_transpose_modelview, &modelview);
        kmMat3Inverse(&inverse_transpose_modelview, &inverse_transpose_modelview);
        kmMat3Transpose(&inverse_transpose_modelview, &inverse_transpose_modelview);

        program.uniforms().set_mat3x3(
            program.uniforms().auto_variable_name(SP_AUTO_INVERSE_TRANSPOSE_MODELVIEW_MATRIX),
            inverse_transpose_modelview
        );
    }
/*
    if(pass.uses_auto_uniform(SP_AUTO_MATERIAL_AMBIENT)) {
        pass.program()->uniforms().set_colour(
            pass.auto_uniform_variable_name(SP_AUTO_MATERIAL_AMBIENT),
            pass.ambient()
        );
    }

    if(pass.uses_auto_uniform(SP_AUTO_MATERIAL_DIFFUSE)) {
        pass.program()->uniforms().set_colour(
            pass.auto_uniform_variable_name(SP_AUTO_MATERIAL_DIFFUSE),
            pass.diffuse()
        );
    }

    if(pass.uses_auto_uniform(SP_AUTO_MATERIAL_SPECULAR)) {
        pass.program()->uniforms().set_colour(
            pass.auto_uniform_variable_name(SP_AUTO_MATERIAL_SPECULAR),
            pass.specular()
        );
    }

    if(pass.uses_auto_uniform(SP_AUTO_MATERIAL_SHININESS)) {
        pass.program()->uniforms().set_float(
            pass.auto_uniform_variable_name(SP_AUTO_MATERIAL_SHININESS),
            pass.shininess()
        );
    }

    if(pass.uses_auto_uniform(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS)) {
        pass.program()->uniforms().set_int(
            pass.auto_uniform_variable_name(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS),
            pass.texture_unit_count()
        );
    }*/
}

template<typename EnabledMethod, typename OffsetMethod>
void send_attribute(GPUProgram& program,
                    ShaderAvailableAttributes attr,
                    const VertexData& data,
                    EnabledMethod exists_on_data_predicate,
                    OffsetMethod offset_func) {

    auto attributes = program.attributes();

    if(!attributes.uses_auto(attr)) {
        return;
    }

    int32_t loc = attributes.locate(attributes.variable_name(attr));
    if(loc < 0) {
        L_WARN("Couldn't locate attribute, on the shader");
        return;
    }

    auto get_has_attribute = std::bind(exists_on_data_predicate, std::reference_wrapper<const VertexData>(data));

    if(get_has_attribute()) {
        auto get_offset = std::bind(offset_func, std::reference_wrapper<const VertexData>(data));

        GLCheck(glEnableVertexAttribArray, loc);
        GLCheck(vaoVertexAttribPointer,
            loc,
            SHADER_ATTRIBUTE_SIZES.find(attr)->second,
            GL_FLOAT,
            GL_FALSE,
            data.stride(),
            BUFFER_OFFSET(get_offset())
        );
    } else {
        L_WARN_ONCE(_u("Couldn't locate attribute on the mesh: {0}").format(attr));
    }
}

void GenericRenderer::set_auto_attributes_on_shader(GPUProgram& program, Renderable &buffer) {
    /*
     *  Binding attributes generically is hard. So we have some template magic in the send_attribute
     *  function above that takes the VertexData member functions we need to provide the attribute
     *  and just makes the whole thing generic. Before this was 100s of lines of boilerplate. Thank god
     *  for templates!
     */
    send_attribute(program, SP_ATTR_VERTEX_POSITION, buffer.vertex_data(), &VertexData::has_positions, &VertexData::position_offset);
    send_attribute(program, SP_ATTR_VERTEX_TEXCOORD0, buffer.vertex_data(), &VertexData::has_texcoord0, &VertexData::texcoord0_offset);
    send_attribute(program, SP_ATTR_VERTEX_TEXCOORD1, buffer.vertex_data(), &VertexData::has_texcoord1, &VertexData::texcoord1_offset);
    send_attribute(program, SP_ATTR_VERTEX_TEXCOORD2, buffer.vertex_data(), &VertexData::has_texcoord2, &VertexData::texcoord2_offset);
    send_attribute(program, SP_ATTR_VERTEX_TEXCOORD3, buffer.vertex_data(), &VertexData::has_texcoord3, &VertexData::texcoord3_offset);
    send_attribute(program, SP_ATTR_VERTEX_DIFFUSE, buffer.vertex_data(), &VertexData::has_diffuse, &VertexData::diffuse_offset);
    send_attribute(program, SP_ATTR_VERTEX_NORMAL, buffer.vertex_data(), &VertexData::has_normals, &VertexData::normal_offset);
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

void GenericRenderer::render(Renderable& buffer, CameraID camera, GPUProgram* program) {

    if(!program) {
        L_ERROR("No shader is bound, so nothing will be rendered");
        return;
    }

    if(!buffer.index_data().count()) {
        return;
    }

    GLStateStash s1(GL_VERTEX_ARRAY_BINDING);
    GLStateStash s2(GL_ELEMENT_ARRAY_BUFFER_BINDING);
    GLStateStash s3(GL_ARRAY_BUFFER_BINDING);

    buffer._update_vertex_array_object();
    buffer._bind_vertex_array_object();

    //Attributes don't change per-iteration of a pass
    set_auto_attributes_on_shader(*program, buffer);
    set_auto_uniforms_on_shader(*program, camera, buffer);

    //Render the mesh, once for each iteration of the pass
    switch(buffer.arrangement()) {
        case MESH_ARRANGEMENT_POINTS:
            GLCheck(glDrawElements, GL_POINTS, buffer.index_data().count(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_LINES:
            GLCheck(glDrawElements, GL_LINES, buffer.index_data().count(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_LINE_STRIP:
            GLCheck(glDrawElements, GL_LINE_STRIP, buffer.index_data().count(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLES:
            GLCheck(glDrawElements, GL_TRIANGLES, buffer.index_data().count(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLE_STRIP:
            GLCheck(glDrawElements, GL_TRIANGLE_STRIP, buffer.index_data().count(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        case MESH_ARRANGEMENT_TRIANGLE_FAN:
            GLCheck(glDrawElements, GL_TRIANGLE_FAN, buffer.index_data().count(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
        break;
        default:
            throw NotImplementedError(__FILE__, __LINE__);
    }

}

}
