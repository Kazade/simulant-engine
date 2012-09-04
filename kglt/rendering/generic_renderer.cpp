#include "glee/GLee.h"

#include "kglt/utils/gl_error.h"
#include "kazmath/mat4.h"
#include "kaztext/kaztext.h"

#include "kglt/scene.h"
#include "kglt/renderer.h"
#include "kglt/mesh.h"
#include "kglt/shader.h"
#include "kglt/window.h"

#include "../utils/gl_error.h"

namespace kglt {



void GenericRenderer::_initialize(Scene& scene) {

}

void GenericRenderer::on_start_render(Scene& scene) {
	glEnable(GL_TEXTURE_2D);
    
    if(options().wireframe_enabled) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(!options().backface_culling_enabled) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glPointSize(options().point_size);

	GLint depth_bits;
	glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
	assert(depth_bits > 0);
}

void GenericRenderer::visit(Text& text) {
    KTuint kt_font = text.font().kt_font(); //Get the kaztext font ID

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    ktBindFont(kt_font);

    float tmp[16];
    for(int i = 0; i < 16; ++i) tmp[i] = (float) projection().top().mat[i];
    ktSetProjectionMatrix(tmp);

    for(int i = 0; i < 16; ++i) tmp[i] = (float) modelview().top().mat[i];
    ktSetModelviewMatrix(tmp);

    ktDrawText(0, (text.font().size() * 0.25) , text.text().c_str());

    check_and_log_error(__FILE__, __LINE__);
}

void GenericRenderer::render_mesh(Mesh& mesh, Scene& scene) {
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
    mesh.vbo(VERTEX_ATTRIBUTE_POSITION | VERTEX_ATTRIBUTE_TEXCOORD_1 | VERTEX_ATTRIBUTE_DIFFUSE);

    uint32_t stride = (sizeof(float) * 3) + (sizeof(float) * 2) + (sizeof(float) * 4);

    //Calculate the modelview-projection matrix
    kmMat4 modelview_projection;
    kmMat4Multiply(&modelview_projection, &projection().top(), &modelview().top());

    for(uint32_t i = 0; i < technique.pass_count(); ++i) {
        MaterialPass& pass = technique.pass(i);

        //Grab and activate the shader for the pass
        ShaderProgram& s = scene.shader(pass.shader() != 0 ? pass.shader() : scene.default_shader());
        s.activate(); //Activate the shader

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

        if(s.params().uses_auto(SP_AUTO_MODELVIEW_PROJECTION_MATRIX)) {
            s.params().set_mat4x4(
                s.params().auto_uniform_variable_name(SP_AUTO_MODELVIEW_PROJECTION_MATRIX),
                modelview_projection.mat
            );
        }

        if(s.params().uses_auto(SP_AUTO_MODELVIEW_MATRIX)) {
            s.params().set_mat4x4(
                s.params().auto_uniform_variable_name(SP_AUTO_MODELVIEW_MATRIX),
                modelview().top().mat
            );
        }

        if(s.params().uses_auto(SP_AUTO_PROJECTION_MATRIX)) {
            s.params().set_mat4x4(
                s.params().auto_uniform_variable_name(SP_AUTO_PROJECTION_MATRIX),
                projection().top().mat
            );
        }

        //Go through the texture units and bind the textures
        for(uint32_t j = 0; j < pass.texture_unit_count(); ++j) {
            glClientActiveTexture(GL_TEXTURE0 + j);
            glBindTexture(GL_TEXTURE_2D, scene.texture(pass.texture_unit(j).texture()).gl_tex());
        }        

        //Render the mesh, once for each pass
        if(mesh.arrangement() == MESH_ARRANGEMENT_POINTS) {
            glDrawArrays(GL_POINTS, 0, mesh.vertices().size());
        } else if(mesh.arrangement() == MESH_ARRANGEMENT_LINE_STRIP) {
            glDrawArrays(GL_LINE_STRIP, 0, mesh.vertices().size());
        } else if(mesh.arrangement() == MESH_ARRANGEMENT_TRIANGLES) {
            glDrawArrays(GL_TRIANGLES, 0, mesh.triangles().size() * 3);
        } else {
            assert(0);
        }

        //Unbind the textures
        for(uint32_t j = 0; j < pass.texture_unit_count(); ++j) {
            glClientActiveTexture(GL_TEXTURE0 + j);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }

    glPopAttrib();
}

void GenericRenderer::visit(Background& background) {
    /*
     *  We store the current projection matrix, then manipulate it so that the correct part
     *  of the background fills the screen. Finally we render the background layers in order
     *  and restore the projection.
     */

    projection().push();

    kmMat4 new_proj;
    kmMat4OrthographicProjection(
                &new_proj, -background.visible_x() / 2.0,
                background.visible_x() / 2.0,
                -background.visible_y() / 2.0,
                background.visible_y() / 2.0, -1.0, 1.0
    );

    kmMat4Assign(&projection().top(), &new_proj);

    for(uint32_t i = 0; i < background.layer_count(); ++i) {
        BackgroundLayer& layer = background.layer(i);
        render_mesh(background.scene().mesh(layer.mesh_id()), background.scene());
    }

    projection().pop();
}

void GenericRenderer::visit(Mesh& mesh) {
    Scene& scene = mesh.scene();
    render_mesh(mesh, scene);
}

}
