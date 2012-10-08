
#include "generic_renderer.h"

#include "kazmath/mat4.h"

namespace kglt {

void GenericRenderer::set_auto_uniforms_on_shader(
    ShaderProgram& s,
    Scene& scene,
    const std::vector<LightID>& lights_within_range,
    uint32_t iteration) {

    //Calculate the modelview-projection matrix
    kmMat4 modelview_projection;
    kmMat4Multiply(&modelview_projection, &projection().top(), &modelview().top());

    if(s.params().uses_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX)) {
        s.params().set_mat4x4(
            s.params().auto_uniform_variable_name(SP_AUTO_MODELVIEW_PROJECTION_MATRIX),
            modelview_projection
        );
    }

    if(s.params().uses_auto(SP_AUTO_MODELVIEW_MATRIX)) {
        s.params().set_mat4x4(
            s.params().auto_uniform_variable_name(SP_AUTO_MODELVIEW_MATRIX),
            modelview().top()
        );
    }

    if(s.params().uses_auto(SP_AUTO_PROJECTION_MATRIX)) {
        s.params().set_mat4x4(
            s.params().auto_uniform_variable_name(SP_AUTO_PROJECTION_MATRIX),
            projection().top()
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

void GenericRenderer::set_auto_attributes_on_shader(ShaderProgram& s) {
    uint32_t stride = (sizeof(float) * 3) +
                      (sizeof(float) * 2) +
                      (sizeof(float) * 4) +
                      (sizeof(float) * 3);

    if(s.params().uses_attribute(SP_ATTR_VERTEX_POSITION)) {
        //Find the location of the attribute, enable it and then point the vertex data at it
        int32_t loc = s.get_attrib_loc(s.params().attribute_variable_name(SP_ATTR_VERTEX_POSITION));
        if(loc > -1) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
        }
    }

    if(s.params().uses_attribute(SP_ATTR_VERTEX_TEXCOORD0)) {
        int32_t loc = s.get_attrib_loc(s.params().attribute_variable_name(SP_ATTR_VERTEX_TEXCOORD0));
        if(loc > -1) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(sizeof(float) * 3));
        }
    }

    if(s.params().uses_attribute(SP_ATTR_VERTEX_DIFFUSE)) {
        int32_t loc = s.get_attrib_loc(s.params().attribute_variable_name(SP_ATTR_VERTEX_DIFFUSE));
        if(loc > -1) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(sizeof(float) * 5));
        }
    }

    if(s.params().uses_attribute(SP_ATTR_VERTEX_NORMAL)) {
        int32_t loc = s.get_attrib_loc(s.params().attribute_variable_name(SP_ATTR_VERTEX_NORMAL));
        if(loc > -1) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(sizeof(float) * 9));
        } else {
            L_ERROR("Unable to find attribute for vertex normal");
        }
    }
}

void GenericRenderer::render_mesh(const Mesh &mesh) {
    Scene& scene = scene();

    glPushAttrib(GL_DEPTH_BUFFER_BIT);

    if(!mesh.depth_test_enabled()) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
    }

    if(!mesh.depth_writes_enabled()) {
        glDepthMask(GL_FALSE);
    } else {
        glDepthMask(GL_TRUE);
    }

    MaterialID mid = mesh.material();
    if(mid == 0) {
        //No material was specified so fallback to the default
        mid = scene.default_material();
    }

    Material& mat = scene.material(mid); //Grab the material for the mesh

    //FIXME: Read the active technique from somewhere
    MaterialTechnique& technique = mat.technique(DEFAULT_MATERIAL_SCHEME);

    //Set up the VBO for the mesh
    mesh.vbo(
        VERTEX_ATTRIBUTE_POSITION |
        VERTEX_ATTRIBUTE_TEXCOORD_1 |
        VERTEX_ATTRIBUTE_DIFFUSE |
        VERTEX_ATTRIBUTE_NORMAL
    );

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(uint32_t i = 0; i < technique.pass_count(); ++i) {
        MaterialPass& pass = technique.pass(i);

        //Grab and activate the shader for the pass
        ShaderProgram& s = scene.shader(pass.shader() != 0 ? pass.shader() : scene.default_shader());
        s.activate(); //Activate the shader

        std::vector<LightID> lights = scene.partitioner().lights_within_range(mesh.position());
        uint32_t iteration_count = 1;
        if(pass.iteration() == ITERATE_N) {
            iteration_count = pass.max_iterations();
        } else if (pass.iteration() == ITERATE_ONCE_PER_LIGHT) {
            iteration_count = std::min<uint32_t>(lights.size(), pass.max_iterations());
        }

        //Attributes don't change per-iteration of a pass
        set_auto_attributes_on_shader(s);

        //Go through the texture units and bind the textures
        for(uint32_t j = 0; j < pass.texture_unit_count(); ++j) {
            glClientActiveTexture(GL_TEXTURE0 + j);
            glBindTexture(GL_TEXTURE_2D, scene.texture(pass.texture_unit(j).texture()).gl_tex());
        }

        for(uint32_t j = 0; j < iteration_count; ++j) {
            set_auto_uniforms_on_shader(s, scene, lights, j); //Uniforms might change depending on the iteration

            //Render the mesh, once for each iteration of the pass
            if(mesh.arrangement() == MESH_ARRANGEMENT_POINTS) {
                glDrawArrays(GL_POINTS, 0, mesh.vertices().size());
            } else if(mesh.arrangement() == MESH_ARRANGEMENT_LINE_STRIP) {
                glDrawArrays(GL_LINE_STRIP, 0, mesh.vertices().size());
            } else if(mesh.arrangement() == MESH_ARRANGEMENT_TRIANGLES) {
                glDrawArrays(GL_TRIANGLES, 0, mesh.triangles().size() * 3);
            } else {
                assert(0);
            }
        }

        //FIXME: should restore whatever was before the loop
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //Unbind the textures
        for(uint32_t j = 0; j < pass.texture_unit_count(); ++j) {
            glClientActiveTexture(GL_TEXTURE0 + j);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE); //Additive after first pass
        assert(glGetError() == GL_NO_ERROR);
    }

    glPopAttrib();
}

}
