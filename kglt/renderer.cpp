#include "glee/GLee.h"

#include "utils/gl_error.h"
#include "kazmath/mat4.h"

#include "scene.h"
#include "renderer.h"
#include "mesh.h"
#include "shader.h"
#include "window.h"

namespace kglt {

void Renderer::start_render(Scene* scene) {
    scene_ = scene;

    if(!options_.texture_enabled) {
        glDisable(GL_TEXTURE_2D);
    } else {
        glEnable(GL_TEXTURE_2D);
    }

    if(options_.wireframe_enabled) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(!options_.backface_culling_enabled) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
    }

    glPointSize(options_.point_size);

    kmVec3& pos = scene->camera().position();
    kmQuaternion& rot = scene->camera().rotation();
    kmMat4 rot_mat;
    kmMat4RotationQuaternion(&rot_mat, &rot);

    rot_mat.mat[12] = pos.x;
    rot_mat.mat[13] = pos.y;
    rot_mat.mat[14] = pos.z;

    kmVec3 up;
    kmVec3 forward;
    kmVec3 centre;
    
    kmMat4GetForwardVec3(&forward, &rot_mat);
    
    kmMat4GetUpVec3(&up, &rot_mat);
    kmVec3Add(&centre, &pos, &forward);

    kmMat4* modelview = &modelview_stack_.top();
    kmMat4LookAt(modelview, &pos, &centre, &up);

    scene->viewport().update_opengl();
    scene->viewport().update_projection_matrix(&projection_stack_.top());
}

void Renderer::visit(Mesh* mesh) {
    kglt::TextureID tex = mesh->texture(PRIMARY);
    glBindTexture(GL_TEXTURE_2D, scene_->texture(tex).gl_tex());

    //FIXME: Allow meshes to override the shader
    ShaderProgram& s = scene_->shader(NullShaderID);
    s.activate();
        
    s.bind_attrib(0, "vertex_position");
    s.bind_attrib(1, "vertex_texcoord_1");
    s.set_uniform("texture_1", 0);


    kmMat4 transform;
    kmMat4Identity(&transform);
    check_and_log_error(__FILE__, __LINE__);
        
	mesh->activate_vbo();
	
	uint32_t stride = (sizeof(float) * 3) + (sizeof(float) * 2);
//        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(0));
//        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(sizeof(float) * 3));
	glVertexPointer(3, GL_FLOAT, stride, BUFFER_OFFSET(0));
	glTexCoordPointer(2, GL_FLOAT, stride, BUFFER_OFFSET(sizeof(float) * 3));
	glClientActiveTexture(GL_TEXTURE0);
	
	kmMat4 modelview_projection;
	kmMat4Multiply(&modelview_projection, &projection_stack_.top(), &modelview_stack_.top());
	
	if(s.has_uniform("modelview_projection_matrix")) {
		s.set_uniform("modelview_projection_matrix", &modelview_projection);
	}
	if(s.has_uniform("modelview_matrix")) {
		s.set_uniform("modelview_matrix", &modelview_stack_.top());
	}
	if(s.has_uniform("projection_matrix")) {
		s.set_uniform("projection_matrix", &projection_stack_.top());
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	if(mesh->arrangement() == MeshArrangement::POINTS) {
		glDrawArrays(GL_POINTS, 0, mesh->vertices().size());        
	} else {
		glDrawArrays(GL_TRIANGLES, 0, mesh->triangles().size() * 3);
	}
	glDisableClientState(GL_VERTEX_ARRAY);        
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);        
}

}
