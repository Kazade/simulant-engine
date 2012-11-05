#include "glee/GLee.h"

#include "generic_renderer.h"

#include "../shader.h"
#include "../scene.h"

#include "kazmath/mat4.h"
#include "../utils/gl_error.h"

namespace kglt {

void GenericRenderer::set_auto_uniforms_on_shader(
    ShaderProgram& s,
    Scene& scene,
    const std::vector<LightID>& lights_within_range,
    uint32_t iteration,
    CameraID camera) {

    //Calculate the modelview-projection matrix
    kmMat4 modelview_projection;

    const kmMat4* modelview = &scene.camera(camera).modelview_matrix();
    const kmMat4* projection = &scene.camera(camera).projection_matrix();

    kmMat4Multiply(
        &modelview_projection,
        projection,
        modelview
    );

    if(s.params().uses_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX)) {
        s.params().set_mat4x4(
            s.params().auto_uniform_variable_name(SP_AUTO_MODELVIEW_PROJECTION_MATRIX),
            modelview_projection
        );
    }

    if(s.params().uses_auto(SP_AUTO_MODELVIEW_MATRIX)) {
        s.params().set_mat4x4(
            s.params().auto_uniform_variable_name(SP_AUTO_MODELVIEW_MATRIX),
            *modelview
        );
    }

    if(s.params().uses_auto(SP_AUTO_PROJECTION_MATRIX)) {
        s.params().set_mat4x4(
            s.params().auto_uniform_variable_name(SP_AUTO_PROJECTION_MATRIX),
            *projection
        );
    }

    if(s.params().uses_auto(SP_AUTO_LIGHT_POSITION)) {
        //Transform the light position by the modelview matrix before
        //passing to the shader
        kmVec3 light_pos;
        kmVec3Fill(&light_pos, 0, 0, 0);
        if(iteration < lights_within_range.size()) {
            light_pos = scene.light(lights_within_range.at(iteration)).position();
        }

        kmVec3Transform(&light_pos, &light_pos, &modelview_projection);

        s.params().set_vec4(
            s.params().auto_uniform_variable_name(SP_AUTO_LIGHT_POSITION),
            Vec4(light_pos, 1.0)
        );
    }

    if(s.params().uses_auto(SP_AUTO_LIGHT_AMBIENT)) {
        kglt::Colour ambient(0, 0, 0, 1);
        if(iteration < lights_within_range.size()) {
            ambient = scene.light(lights_within_range.at(iteration)).ambient();
        }
        s.params().set_colour(
            s.params().auto_uniform_variable_name(SP_AUTO_LIGHT_AMBIENT),
            ambient
        );
    }

    if(s.params().uses_auto(SP_AUTO_LIGHT_DIFFUSE)) {
        kglt::Colour diffuse(0, 0, 0, 1);

        if(iteration < lights_within_range.size()) {
            diffuse = scene.light(lights_within_range.at(iteration)).diffuse();
        }

        s.params().set_colour(
            s.params().auto_uniform_variable_name(SP_AUTO_LIGHT_DIFFUSE),
            diffuse
        );
    }

    if(s.params().uses_auto(SP_AUTO_LIGHT_SPECULAR)) {
        kglt::Colour specular(0, 0, 0, 1);

        if(iteration < lights_within_range.size()) {
            specular = scene.light(lights_within_range.at(iteration)).specular();
        }

        s.params().set_colour(
            s.params().auto_uniform_variable_name(SP_AUTO_LIGHT_SPECULAR),
            specular
        );
    }

    if(s.params().uses_auto(SP_AUTO_LIGHT_CONSTANT_ATTENUATION)) {
        float constant_attenuation = 1.0;

        if(iteration < lights_within_range.size()) {
            constant_attenuation = scene.light(lights_within_range.at(iteration)).constant_attenuation();
        }

        s.params().set_float(
            s.params().auto_uniform_variable_name(SP_AUTO_LIGHT_CONSTANT_ATTENUATION),
            constant_attenuation
        );
    }

    if(s.params().uses_auto(SP_AUTO_LIGHT_LINEAR_ATTENUATION)) {
        float linear_attenuation = 1.0;

        if(iteration < lights_within_range.size()) {
            linear_attenuation = scene.light(lights_within_range.at(iteration)).linear_attenuation();
        }

        s.params().set_float(
            s.params().auto_uniform_variable_name(SP_AUTO_LIGHT_LINEAR_ATTENUATION),
            linear_attenuation
        );
    }

    if(s.params().uses_auto(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION)) {
        float quadratic_attenuation = 1.0;

        if(iteration < lights_within_range.size()) {
            quadratic_attenuation = scene.light(lights_within_range.at(iteration)).quadratic_attenuation();
        }

        s.params().set_float(
            s.params().auto_uniform_variable_name(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION),
            quadratic_attenuation
        );
    }

    if(s.params().uses_auto(SP_AUTO_LIGHT_GLOBAL_AMBIENT)) {
        s.params().set_colour(
            s.params().auto_uniform_variable_name(SP_AUTO_LIGHT_GLOBAL_AMBIENT),
            scene.ambient_light()
        );
    }
}

void GenericRenderer::set_auto_attributes_on_shader(ShaderProgram& s, SubEntity& buffer) {
    uint32_t stride = buffer.vertex_data().stride();

    if(s.params().uses_attribute(SP_ATTR_VERTEX_POSITION)) {
        //Find the location of the attribute, enable it and then point the vertex data at it
        int32_t loc = s.get_attrib_loc(s.params().attribute_variable_name(SP_ATTR_VERTEX_POSITION));
        if(loc > -1 && buffer.vertex_data().has_positions()) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(buffer.vertex_data().position_offset()));
        }
    }

    if(s.params().uses_attribute(SP_ATTR_VERTEX_TEXCOORD0)) {
        int32_t loc = s.get_attrib_loc(s.params().attribute_variable_name(SP_ATTR_VERTEX_TEXCOORD0));        
        if(loc > -1 && buffer.vertex_data().has_texcoord0()) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(buffer.vertex_data().texcoord0_offset()));
        } else {
            L_WARN("Couldn't locate attribute, either on the mesh or in the shader");
        }
    }

    if(s.params().uses_attribute(SP_ATTR_VERTEX_DIFFUSE)) {
        int32_t loc = s.get_attrib_loc(s.params().attribute_variable_name(SP_ATTR_VERTEX_DIFFUSE));
        if(loc > -1 && buffer.vertex_data().has_diffuse()) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(buffer.vertex_data().diffuse_offset()));
        }
    }

    if(s.params().uses_attribute(SP_ATTR_VERTEX_NORMAL)) {
        int32_t loc = s.get_attrib_loc(s.params().attribute_variable_name(SP_ATTR_VERTEX_NORMAL));
        if(loc > -1 && buffer.vertex_data().has_normals()) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(buffer.vertex_data().normal_offset()));
        } else {
            L_ERROR("Unable to find attribute for vertex normal");
        }
    }
}

void GenericRenderer::set_blending_mode(BlendType type) {
    if(type == BLEND_NONE) {
        glDisable(GL_BLEND);
        return;
    }

    glEnable(GL_BLEND);
    switch(type) {
        case BLEND_ADD: glBlendFunc(GL_ONE, GL_ONE);
        break;
        case BLEND_ALPHA: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
        case BLEND_COLOUR: glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
        break;
        case BLEND_MODULATE: glBlendFunc(GL_DST_COLOR, GL_ZERO);
        break;
    default:
        throw ValueError("Invalid blend type specified");
    }
}

void GenericRenderer::render_subentity(SubEntity& buffer, CameraID camera) {
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    if(!buffer.index_data().count()) {
        return;
    }

    buffer.vertex_data().buffer_object().bind();
    buffer.index_data().buffer_object().bind();

    check_and_log_error(__FILE__, __LINE__);

    MaterialID mid = buffer.material();
    if(!mid) {
        mid = scene().default_material();
    }
    Material& mat = scene().material(mid);
    MaterialTechnique& technique = mat.technique(DEFAULT_MATERIAL_SCHEME);

    for(uint32_t i = 0; i < technique.pass_count(); ++i) {
        check_and_log_error(__FILE__, __LINE__);

        MaterialPass& pass = technique.pass(i);

        set_blending_mode(pass.blending());

        //Grab and activate the shader for the pass
        ShaderProgram& s = scene().shader(pass.shader() != 0 ? pass.shader() : scene().default_shader());
        s.activate(); //Activate the shader

        //FIXME: lights within range of what?
        Vec3 pos;
        std::vector<LightID> lights = scene().pipeline().partitioner().lights_within_range(pos);
        uint32_t iteration_count = 1;
        if(pass.iteration() == ITERATE_N) {
            iteration_count = pass.max_iterations();
        } else if (pass.iteration() == ITERATE_ONCE_PER_LIGHT) {
            iteration_count = std::min<uint32_t>(lights.size(), pass.max_iterations());
        }

        //Attributes don't change per-iteration of a pass
        set_auto_attributes_on_shader(s, buffer);

        //Go through the texture units and bind the textures
        for(uint32_t j = 0; j < pass.texture_unit_count(); ++j) {
            glClientActiveTexture(GL_TEXTURE0 + j);
            glBindTexture(GL_TEXTURE_2D, scene().texture(pass.texture_unit(j).texture()).gl_tex());
        }

        for(uint32_t j = 0; j < iteration_count; ++j) {
            set_auto_uniforms_on_shader(s, scene(), lights, j, camera); //Uniforms might change depending on the iteration

            //Render the mesh, once for each iteration of the pass
            if(buffer.arrangement() == MESH_ARRANGEMENT_POINTS) {
                glDrawArrays(GL_POINTS, 0, buffer.index_data().count());
            } else if(buffer.arrangement() == MESH_ARRANGEMENT_LINE_STRIP) {
                glDrawArrays(GL_LINE_STRIP, 0, buffer.index_data().count());
            } else if(buffer.arrangement() == MESH_ARRANGEMENT_TRIANGLES) {
                glDrawArrays(GL_TRIANGLES, 0, buffer.index_data().count());
            } else {
                assert(0);
            }
        }

        //Unbind the textures
        for(uint32_t j = 0; j < pass.texture_unit_count(); ++j) {
            glClientActiveTexture(GL_TEXTURE0 + j);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
        glDisableVertexAttribArray(4);
        glDisableVertexAttribArray(5);
        glDisableVertexAttribArray(6);

        check_and_log_error(__FILE__, __LINE__);
    }
}

}
