#include <SDL_opengl.h>

#include "renderer.h"
#include "mesh.h"

namespace GL {

void Renderer::visit(Mesh* mesh) {
    glPushMatrix();
        glTranslatef(mesh->position().x, mesh->position().y, mesh->position().z);
        glBegin(GL_TRIANGLES);
        for(Mesh& submesh: mesh->submeshes()) {
            for(Triangle& triangle: submesh.triangles()) {
                Vertex v1 = submesh.vertex(triangle.a);
                Vertex v2 = submesh.vertex(triangle.b);
                Vertex v3 = submesh.vertex(triangle.c);

                glTexCoord2f(v1.uv.x, v1.uv.y);
                glVertex3f(v1.x, v1.y, v1.z);

                glTexCoord2f(v2.uv.x, v2.uv.y);
                glVertex3f(v2.x, v2.y, v2.z);

                glTexCoord2f(v3.uv.x, v3.uv.y);
                glVertex3f(v3.x, v3.y, v3.z);
            }
        }
        glEnd();
    glPopMatrix();
}

Renderer::~Renderer() {

}

}
