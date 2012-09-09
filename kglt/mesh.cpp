#include "glee/GLee.h"
#include "mesh.h"
#include "kazbase/list_utils.h"
#include "scene.h"

namespace kglt {

Mesh::Mesh(Scene* parent, MeshID id):
    Object(parent),
    Identifiable<MeshID>(id),
    is_submesh_(false),
    use_parent_vertices_(false),
    material_(0),
    diffuse_colour_(1.0, 1.0, 1.0, 1.0),
    depth_test_enabled_(true),
    depth_writes_enabled_(true),
    branch_selectable_(true) {

    set_arrangement(MESH_ARRANGEMENT_TRIANGLES);
}

Mesh::~Mesh() {
    for(std::pair<uint32_t, uint32_t> vbo: vertex_buffer_objects_) {
        glDeleteBuffers(1, &vbo.second);
    }
}

void Mesh::destroy() {
    scene().delete_mesh(id());
}

Vertex& Mesh::vertex(uint32_t v) {
    if(use_parent_vertices_) {
        if(!is_submesh_) {
            throw std::logic_error("Attempted to grab parent vertex from a non-submesh");
        }
        return parent_mesh().vertex(v);
    }
    return vertices_[v];
}

Triangle& Mesh::triangle(uint32_t t) {
    return triangles_[t];
}

void Mesh::add_vertex(float x, float y, float z) {
    Vertex vert;
    vert.x = x;
    vert.y = y;
    vert.z = z;
    vertices_.push_back(vert);

    vertex_buffer_objects_.clear(); //Invalidate the vbos
}

Triangle& Mesh::add_triangle(uint32_t a, uint32_t b, uint32_t c) {
    Triangle t;
    t.set_indexes(a, b, c);
    triangles_.push_back(t);

    vertex_buffer_objects_.clear(); //Invalidate the vbos
    return triangles_[triangles_.size() - 1];
}

uint32_t Mesh::add_submesh(bool use_parent_vertices) {
    /*
        FIXME: Using Meshes as submeshes seems dodgy, submeshes are part
        of the mesh and shouldn't be owned by the scene so we are sort of circumventing
        that a bit. I think Submesh should be its own type that doesn't derive Object
    */
    Mesh::ptr new_mesh(new Mesh(&this->scene()));
    submeshes_.push_back(new_mesh);
    uint32_t id = submeshes_.size() - 1;

    submeshes_[id]->set_parent(this); //Add to the tree
    submeshes_[id]->use_parent_vertices_ = use_parent_vertices;
    submeshes_[id]->is_submesh_ = true;
    return id;
}

void Mesh::vbo(uint32_t vertex_attributes) {
    if(!container::contains(vertex_buffer_objects_, vertex_attributes)) {
        //Build the VBO if necessary
        vertex_buffer_objects_[vertex_attributes] = build_vbo(vertex_attributes);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_objects_[vertex_attributes]);
}

uint32_t Mesh::build_vbo(uint32_t vertex_attributes) {
    /**
     * THIS CODE IS A FUCKING MESS
     */
    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

    uint32_t stride = 0;
    if((VERTEX_ATTRIBUTE_POSITION & vertex_attributes) == VERTEX_ATTRIBUTE_POSITION) {
        stride += sizeof(float) * 3;

    }
    if ((VERTEX_ATTRIBUTE_TEXCOORD_1 & vertex_attributes) == VERTEX_ATTRIBUTE_TEXCOORD_1) {
        stride += sizeof(float) * 2;

    }
    if ((VERTEX_ATTRIBUTE_DIFFUSE & vertex_attributes) == VERTEX_ATTRIBUTE_DIFFUSE) {
        stride += sizeof(float) * 4;
    }

    if ((VERTEX_ATTRIBUTE_NORMAL & vertex_attributes) == VERTEX_ATTRIBUTE_NORMAL) {
        stride += sizeof(float) * 3;
    }

    uint32_t total_size = triangles().size() * stride * 3;
    glBufferData(GL_ARRAY_BUFFER, total_size, NULL, GL_STATIC_DRAW);

    uint32_t offset = 0;
    if(arrangement() == MESH_ARRANGEMENT_LINE_STRIP ||
       arrangement() == MESH_ARRANGEMENT_POINTS) {
        total_size = vertices().size() * stride;
        glBufferData(GL_ARRAY_BUFFER, total_size, NULL, GL_STATIC_DRAW);

        for(Vertex& v: vertices()) {
            if((VERTEX_ATTRIBUTE_POSITION & vertex_attributes) == VERTEX_ATTRIBUTE_POSITION) {
                glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float) * 3, (float*) &v);
                offset += sizeof(float) * 3;
            }

            if((VERTEX_ATTRIBUTE_TEXCOORD_1 & vertex_attributes) == VERTEX_ATTRIBUTE_TEXCOORD_1) {
                Vec2 uv;
                uv.x = 1.0; uv.y = 1.0;
                glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float) * 2, (float*) &uv);
                offset += sizeof(float) * 2;
            }

            if((VERTEX_ATTRIBUTE_DIFFUSE & vertex_attributes) == VERTEX_ATTRIBUTE_DIFFUSE) {
                glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float) * 4, (float*) &diffuse_colour_);
                offset += sizeof(float) * 4;
            }

            if((VERTEX_ATTRIBUTE_NORMAL & vertex_attributes) == VERTEX_ATTRIBUTE_NORMAL) {
                Vec3 n;
                kmVec3Fill(&n, 0, 1, 0);
                glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float) * 3, (float*) &n);
                offset += sizeof(float) * 3;
            }
        }
    } else {
        for(Triangle& tri: triangles()) {
            for(uint32_t j = 0; j < 3; ++j) {
                Vertex& v = vertices()[tri.index(j)];
                if((VERTEX_ATTRIBUTE_POSITION & vertex_attributes) == VERTEX_ATTRIBUTE_POSITION) {
                    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float) * 3, (float*) &v);
                    offset += sizeof(float) * 3;
                }
                if((VERTEX_ATTRIBUTE_TEXCOORD_1 & vertex_attributes) == VERTEX_ATTRIBUTE_TEXCOORD_1) {
                    Vec2 uv = tri.uv(j);
                    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float) * 2, (float*) &uv);
                    offset += sizeof(float) * 2;
                }
                if((VERTEX_ATTRIBUTE_DIFFUSE & vertex_attributes) == VERTEX_ATTRIBUTE_DIFFUSE) {
                    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float) * 4, (float*) &diffuse_colour_);
                    offset += sizeof(float) * 4;
                }
                if((VERTEX_ATTRIBUTE_NORMAL & vertex_attributes) == VERTEX_ATTRIBUTE_NORMAL) {
                    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(float) * 3, (float*) &tri.normal(j));
                    offset += sizeof(float) * 3;
                }
            }
        }
    }

    return vertex_buffer;
}

}
