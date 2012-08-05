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

const std::string get_default_vert_shader_120() {
    const std::string default_vert_shader_120 = R"(
#version 120

attribute vec3 vertex_position;
attribute vec2 vertex_texcoord_1;
attribute vec4 vertex_diffuse;

uniform mat4 modelview_projection_matrix;

varying vec2 fragment_texcoord_1;
varying vec4 fragment_diffuse;

void main() {
    gl_Position = modelview_projection_matrix * vec4(vertex_position, 1.0);
    fragment_texcoord_1 = vertex_texcoord_1;
    fragment_diffuse = vertex_diffuse;
}

)";

    return default_vert_shader_120;
}

const std::string get_default_frag_shader_120() {
    const std::string default_frag_shader_120 = R"(
#version 120

varying vec2 fragment_texcoord_1;
varying vec4 fragment_diffuse;

uniform sampler2D texture_1;

/*
struct light {
    int type;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 position;
};

uniform light lights[8];*/

void main() {
    gl_FragColor = texture2D(texture_1, fragment_texcoord_1.st) * fragment_diffuse;
}

)";
    return default_frag_shader_120;
}

void GenericRenderer::_initialize(Scene& scene) {
    default_shader_ = scene.new_shader();

    ShaderProgram& def = scene.shader(default_shader_); //Create a default shader;

    assert(glGetError() == GL_NO_ERROR);

    def.add_and_compile(SHADER_TYPE_VERTEX, get_default_vert_shader_120());
    def.add_and_compile(SHADER_TYPE_FRAGMENT, get_default_frag_shader_120());
    def.activate();

    //Bind the vertex attributes for the default shader and relink
    def.bind_attrib(0, "vertex_position");
    def.bind_attrib(1, "vertex_texcoord_1");
    def.bind_attrib(2, "vertex_diffuse");
    def.relink();
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

    scene.shader(default_shader_).activate();
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

    kglt::TextureID tex = mesh.texture(PRIMARY);
    if(!options().texture_enabled) {
        tex = NullTextureID; //Turn off the texture
    }
    glBindTexture(GL_TEXTURE_2D, scene.texture(tex).gl_tex());

    //FIXME: Allow meshes to override the shader
    ShaderProgram& s = scene.shader(default_shader_);
    s.activate();

    //s.bind_attrib(2, "vertex_diffuse");
    s.set_uniform("texture_1", 0);

    check_and_log_error(__FILE__, __LINE__);

    mesh.vbo(VERTEX_ATTRIBUTE_POSITION | VERTEX_ATTRIBUTE_TEXCOORD_1 | VERTEX_ATTRIBUTE_DIFFUSE);

    uint32_t stride = (sizeof(float) * 3) + (sizeof(float) * 2) + (sizeof(float) * 4);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(sizeof(float) * 3));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(sizeof(float) * 5));

    glClientActiveTexture(GL_TEXTURE0);

    kmMat4 modelview_projection;
    kmMat4Multiply(&modelview_projection, &projection().top(), &modelview().top());

    if(s.has_uniform("modelview_projection_matrix")) {
        s.set_uniform("modelview_projection_matrix", &modelview_projection);
    }
    if(s.has_uniform("modelview_matrix")) {
        s.set_uniform("modelview_matrix", &modelview().top());
    }
    if(s.has_uniform("projection_matrix")) {
        s.set_uniform("projection_matrix", &projection().top());
    }

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    if(mesh.arrangement() == MESH_ARRANGEMENT_POINTS) {
        glDrawArrays(GL_POINTS, 0, mesh.vertices().size());
    } else if(mesh.arrangement() == MESH_ARRANGEMENT_LINE_STRIP) {
        glDrawArrays(GL_LINE_STRIP, 0, mesh.vertices().size());
    } else if(mesh.arrangement() == MESH_ARRANGEMENT_TRIANGLES) {
        glDrawArrays(GL_TRIANGLES, 0, mesh.triangles().size() * 3);
    } else {
        assert(0);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

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
