#include "glee/GLee.h"

#include <boost/format.hpp>

#include "kglt/utils/gl_error.h"
#include "kglt/scene.h"
#include "kglt/shortcuts.h"
#include "selection_renderer.h"

namespace kglt {

const std::string selection_vert_shader_120() {
    const std::string vert_shader = R"(
#version 120

attribute vec3 vertex_position;

uniform vec3 selection_colour;
uniform mat4 modelview_projection_matrix;

varying vec4 fragment_diffuse;

void main() {
    gl_Position = modelview_projection_matrix * vec4(vertex_position, 1.0);
    fragment_diffuse = vec4(selection_colour, 1.0);
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
		shader.relink();		
		
		shader.activate();
		
	} else {
		//Activate the shader
		scene().shader(selection_shader.first).activate();
	}
	
	/*
	 * Do we need to clear the depth buffer? If we don't then a 
	 * selection render done in the second pass will perform faster as
	 * the depth test will disqualify polygons
	 */
	 
	//FIXME: !!!
	//glPushAttrib(GL_CLEAR_COLOR_BIT);
	//glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT); 	
	//glPopAttrib(GL_CLEAR_COLOR_BIT);
	
	r_count = 0;
	g_count = 0; 
	b_count	= 0;
	colour_mesh_lookup_.clear();
	selected_mesh_id_ = 0;
}

void SelectionRenderer::on_finish_render() {	
	uint8_t pixel[3];
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	int32_t mouse_x, mouse_y;
	scene().window().cursor_position(mouse_x, mouse_y);
	
	glReadPixels(mouse_x, viewport[3] - mouse_y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
	
	std::tuple<float, float, float> selected_colour = std::make_tuple(
		(1.0 / 255.0) * pixel[0],
		(1.0 / 255.0) * pixel[1],
		(1.0 / 255.0) * pixel[2]
	);
	
	//L_DEBUG((boost::format("%f, %f, %f") % std::get<0>(selected_colour) % std::get<1>(selected_colour) % std::get<2>(selected_colour)).str());
	
	if(container::contains(colour_mesh_lookup_, selected_colour)) {
		selected_mesh_id_ = colour_mesh_lookup_[selected_colour];
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
}
	
void SelectionRenderer::visit(Mesh* mesh) {
	std::pair<ShaderID, bool> selection_shader = scene().find_shader("selection_shader");
	assert(selection_shader.second);
	
	ShaderProgram& s = scene().shader(selection_shader.first);
	
	b_count++;
	if(b_count == 255) {
		b_count = 0;
		g_count++;
		if(g_count == 255) {
			g_count = 0;
			r_count++;			
			assert(r_count < 255);
		}
	}
	
	std::tuple<float, float, float> current_colour = std::make_tuple(
		(1.0 / 255.0) * r_count, 
		(1.0 / 255.0) * g_count, 
		(1.0 / 255.0) * b_count
	);
	
	colour_mesh_lookup_[current_colour] = scene()._mesh_id_from_mesh_ptr(mesh);
	
	//Bind the NULL texture (e.g. make sure everything is white)
	glBindTexture(GL_TEXTURE_2D, scene().texture(NullTextureID).gl_tex());

    kmMat4 transform;
    kmMat4Identity(&transform);
    check_and_log_error(__FILE__, __LINE__);
    
    //Only draw vertices
	mesh->vbo(VERTEX_ATTRIBUTE_POSITION);
	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, BUFFER_OFFSET(0));
    
	kmMat4 modelview_projection;
    kmMat4Multiply(&modelview_projection, &projection_matrix(), &modelview_stack().top());
	s.set_uniform("modelview_projection_matrix", &modelview_projection);	
	
	kmVec3 colour;
	kmVec3Fill(
		&colour, 
		std::get<0>(current_colour), 
		std::get<1>(current_colour), 
		std::get<2>(current_colour)
	);
	
	s.set_uniform("selection_colour", &colour);
	
	glEnableVertexAttribArray(0);		
	if(mesh->arrangement() == MESH_ARRANGEMENT_POINTS) {
		glDrawArrays(GL_POINTS, 0, mesh->vertices().size());   
	} else if(mesh->arrangement() == MESH_ARRANGEMENT_LINE_STRIP) {
		glDrawArrays(GL_LINE_STRIP, 0, mesh->vertices().size());     	
	} else if(mesh->arrangement() == MESH_ARRANGEMENT_TRIANGLES) {
		glDrawArrays(GL_TRIANGLES, 0, mesh->triangles().size() * 3);
	} else {
		assert(0);
	}
	glDisableVertexAttribArray(0);	
}

}
