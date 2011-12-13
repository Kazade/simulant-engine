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

            glBegin(GL_TRIANGLES);
            for(Triangle& triangle: mesh->triangles()) {
                    Vertex v1 = mesh->vertex(triangle.a);
                    Vertex v2 = mesh->vertex(triangle.b);
                    Vertex v3 = mesh->vertex(triangle.c);

                    glTexCoord2f(v1.uv.x, v1.uv.y);
                    glVertex3f(v1.x, v1.y, v1.z);

                    glTexCoord2f(v2.uv.x, v2.uv.y);
                    glVertex3f(v2.x, v2.y, v2.z);

                    glTexCoord2f(v3.uv.x, v3.uv.y);
                    glVertex3f(v3.x, v3.y, v3.z);

            }
            glEnd();

        }
    glPopMatrix();
}

Renderer::~Renderer() {

}

}
