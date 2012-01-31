
#include "mesh.h"

namespace kglt {

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

TextureID& Mesh::texture(TextureLevel l) {
    return textures_[l];
}

void Mesh::apply_texture(TextureID tex, TextureLevel level) {
    assert(level < MAX_TEXTURE_LEVELS);
    textures_[level] = tex;
}

void Mesh::add_vertex(float x, float y, float z) {
    Vertex vert;
    vert.x = x;
    vert.y = y;
    vert.z = z;
    vertices_.push_back(vert);
}

Triangle& Mesh::add_triangle(uint32_t a, uint32_t b, uint32_t c) {
    Triangle t;
    t.idx[0] = a;
    t.idx[1] = b;
    t.idx[2] = c;
    triangles_.push_back(t);
    
    update_vbo();
    
    return triangles_[triangles_.size() - 1];
}

uint32_t Mesh::add_submesh(bool use_parent_vertices) {
    Mesh::ptr new_mesh(new Mesh);
    submeshes_.push_back(new_mesh);
    uint32_t id = submeshes_.size() - 1;

    submeshes_[id]->set_parent(this); //Add to the tree
    submeshes_[id]->use_parent_vertices_ = use_parent_vertices;
    submeshes_[id]->is_submesh_ = true;
    return id;
}

void Mesh::update_vbo() {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    
    struct VData {
        float pos[3];
        float uv[2];
    };

    uint32_t size_in_bytes = triangles_.size() * sizeof(VData) * 3;
    
    glBufferData(GL_ARRAY_BUFFER, size_in_bytes, NULL, GL_STATIC_DRAW);
    
    uint32_t i = 0;
    for(Triangle& tri: triangles_) {
        uint32_t offset = sizeof(VData) * i * 3;
        
        VData data;
        for(uint32_t j = 0; j < 3; ++j) {
            Vertex& v = vertices()[tri.idx[j]];

            data.pos[0] = v.x;
            data.pos[1] = v.y;
            data.pos[2] = v.z;
            data.uv[0] = tri.uv[j].x;
            data.uv[1] = tri.uv[j].y;
            
            glBufferSubData(GL_ARRAY_BUFFER, offset + (sizeof(VData) * j), sizeof(VData), &data.pos[0]);
        }
        ++i;
    }
}

}
