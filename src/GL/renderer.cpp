#include <SDL_opengl.h>

#include "kazmath/mat4.h"

#include "scene.h"
#include "renderer.h"
#include "mesh.h"

namespace GL {

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
    kmMat4GetForwardVec3(&forward, &rot_mat);
    kmMat4GetUpVec3(&up, &rot_mat);

    gluLookAt(pos.x, pos.y, pos.z,
              pos.x + forward.x, pos.y + forward.y, pos.z + forward.z,
              up.x, up.y, up.z);
}

void Renderer::visit(Mesh* mesh) {
    GL::TextureID tex = mesh->texture(PRIMARY);
    if(tex != NullTextureID) {
        glBindTexture(GL_TEXTURE_2D, scene_->texture(tex).gl_tex());
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glPushMatrix();
        //FIXME: should be absolute position
        //glTranslatef(mesh->position().x, mesh->position().y, mesh->position().z);
        if(mesh->arrangement() == MeshArrangement::POINTS) {
            glBegin(GL_POINTS);
            for(Vertex& vert: mesh->vertices()) {
                glVertex3f(vert.x, vert.y, vert.z);
            }
            glEnd();
        } else {
//            std::cout << "Rendering mesh: " << mesh->id() << std::endl;
            glBegin(GL_TRIANGLES);
            for(Triangle& triangle: mesh->triangles()) {
                for(int i = 0; i < 3; ++i) {
                    Vertex v1 = mesh->vertex(triangle.idx[i]);
                    glTexCoord2f(triangle.uv[i].x, triangle.uv[i].y);
                    glVertex3f(v1.x, v1.y, v1.z);
                }
            }
            glEnd();
        }
    glPopMatrix();
}

Renderer::~Renderer() {

}

}
