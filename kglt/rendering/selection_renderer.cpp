#include "glee/GLee.h"

#include "kglt/scene.h"
#include "kglt/shortcuts.h"
#include "selection_renderer.h"

namespace kglt {

const std::string selection_vert_shader_120() {
    const std::string vert_shader = R"(
#version 120

attribute vec3 vertex_position;
attribute vec4 vertex_diffuse;

uniform mat4 modelview_projection_matrix;

varying vec4 fragment_diffuse;

void main() {
    gl_Position = modelview_projection_matrix * vec4(vertex_position, 1.0);
    fragment_diffuse = vertex_diffuse;
}

)";
    
    return vert_shader;
}

const std::string selection_frag_shader_120() {    
    const std::string frag_shader = R"(
#version 120

varying vec4 fragment_diffuse;

void main() {
    gl_FragColor = fragment_diffuse;
}

)";    
    return frag_shader;
}

void SelectionRenderer::on_start_render() {
	std::pair<ShaderID, bool> selection_shader = scene().find_shader("selection_shader");
	if(!selection_shader.second) {
		//Load the selection shader into the scene
		ShaderProgram& shader = kglt::return_new_shader(scene());
		shader.set_name("selection_shader");
		
		shader.add_and_compile(SHADER_TYPE_VERTEX, selection_vert_shader_120());
		shader.add_and_compile(SHADER_TYPE_FRAGMENT, selection_frag_shader_120());
		shader.activate();
		
		//Bind the vertex attributes for the selection shader and relink
		shader.bind_attrib(0, "vertex_position");
		shader.bind_attrib(1, "vertex_diffuse");
		shader.relink();		
		
	} else {
		//Activate the shader
		scene().shader(selection_shader.first).activate();
	}
	
	/*
	 * Do we need to clear the depth buffer? If we don't then a 
	 * selection render done in the second pass will perform faster as
	 * the depth test will disqualify polygons
	 */
	glClear(GL_COLOR_BUFFER_BIT); 	
}

void SelectionRenderer::on_finish_render() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
	
void SelectionRenderer::visit(Mesh* mesh) {
	//Bind the NULL texture (e.g. make sure everything is white)
	glBindTexture(GL_TEXTURE_2D, scene().texture(NullTextureID).gl_tex());
	    
}

}
