#include "glee/GLee.h"

#include "kglt/utils/gl_error.h"
#include "kazmath/mat4.h"

#include "kglt/scene.h"
#include "kglt/renderer.h"
#include "kglt/mesh.h"
#include "kglt/shader.h"
#include "kglt/window.h"

namespace kglt {

void GenericRenderer::on_start_render() {
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

    glPointSize(options().point_size);

}

void GenericRenderer::visit(Mesh* mesh) {
    kglt::TextureID tex = mesh->texture(PRIMARY);
    if(!options().texture_enabled) {
		tex = NullTextureID; //Turn off the texture
	}
    glBindTexture(GL_TEXTURE_2D, scene().texture(tex).gl_tex());

    //FIXME: Allow meshes to override the shader
    ShaderProgram& s = scene().shader(NullShaderID);
    s.activate();
                    
    //s.bind_attrib(2, "vertex_diffuse");
    s.set_uniform("texture_1", 0);

    kmMat4 transform;
    kmMat4Identity(&transform);
    check_and_log_error(__FILE__, __LINE__);
                
	mesh->activate_vbo();
	
	uint32_t stride = (sizeof(float) * 3) + (sizeof(float) * 2) + (sizeof(float) * 4);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(sizeof(float) * 3));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(sizeof(float) * 5));

	glClientActiveTexture(GL_TEXTURE0);
	
	kmMat4 modelview_projection;
	kmMat4Multiply(&modelview_projection, &projection_stack().top(), &modelview_stack().top());
	
	if(s.has_uniform("modelview_projection_matrix")) {
		s.set_uniform("modelview_projection_matrix", &modelview_projection);
	}
	if(s.has_uniform("modelview_matrix")) {
		s.set_uniform("modelview_matrix", &modelview_stack().top());
	}
	if(s.has_uniform("projection_matrix")) {
		s.set_uniform("projection_matrix", &projection_stack().top());
	}

	/*glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);*/
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
		
	if(mesh->arrangement() == MeshArrangement::POINTS) {
		glDrawArrays(GL_POINTS, 0, mesh->vertices().size());        
	} else {
		glDrawArrays(GL_TRIANGLES, 0, mesh->triangles().size() * 3);
	}
	
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
		
	/*glDisableClientState(GL_VERTEX_ARRAY);        
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);        */
}

}
