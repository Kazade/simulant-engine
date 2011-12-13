#include <SDL_opengl.h>

#include "scene.h"
#include "renderer.h"
#include "mesh.h"

namespace GL {

void Renderer::visit(Mesh* mesh) {
    GL::TextureID tex = mesh->texture(0);
    if(tex) {
        glBindTexture(GL_TEXTURE_2D, scene_->texture(tex).gl_tex());
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glPushMatrix();
        //FIXME: should be absolute position
        glTranslatef(mesh->position().x, mesh->position().y, mesh->position().z);
        if(mesh->arrangement() == MeshArrangement::POINTS) {
            glBegin(GL_POINTS);
            for(Vertex& vert: mesh->vertices()) {
                glVertex3f(vert.x, vert.y, vert.z);
            }
            glEnd();
        } else {
            for(Triangle& triangle: mesh->triangles()) {
                if(triangle.tex_id) {
                    GLuint tex_id = scene_->texture(triangle.tex_id).gl_tex();
                    glBindTexture(GL_TEXTURE_2D, tex_id);
                }
                assert(glGetError() == GL_NO_ERROR);

                glBegin(GL_TRIANGLES);
                for(int i = 0; i < 3; ++i) {
                    Vertex v1 = mesh->vertex(triangle.idx[i]);
                    glTexCoord2f(triangle.uv[i].x, triangle.uv[i].y);
                    glVertex3f(v1.x, v1.y, v1.z);
                }
                glEnd();
            }


        }
    glPopMatrix();
}

Renderer::~Renderer() {

}

}
