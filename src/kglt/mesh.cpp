
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
    
    struct Element {
        float v1_pos[3];
        float v1_tex_1[2];
        float v2_pos[3];
        float v2_tex_1[2];
        float v3_pos[3];
        float v3_tex_1[2];
    };
    
    uint32_t size_in_bytes = triangles_.size() * sizeof(Element);
    
    glBufferData(GL_ARRAY_BUFFER, size_in_bytes, NULL, GL_STATIC_DRAW);
    
    Element tmp;
    
    uint32_t i = 0;
    for(Triangle& tri: triangles_) {
        uint32_t offset = sizeof(Element) * i;
        
        Vertex& v1 = vertices_[tri.idx[0]];
        Vertex& v2 = vertices_[tri.idx[1]];
        Vertex& v3 = vertices_[tri.idx[2]];
        
        tmp.v1_pos[0] = v1.x;
        tmp.v1_pos[1] = v1.y;
        tmp.v1_pos[2] = v1.z;
        
        tmp.v1_tex_1[0] = tri.uv[0].x;
        tmp.v1_tex_1[1] = tri.uv[0].y;

        tmp.v2_pos[0] = v2.x;
        tmp.v2_pos[1] = v2.y;
        tmp.v2_pos[2] = v2.z;
        
        tmp.v2_tex_1[0] = tri.uv[1].x;
        tmp.v2_tex_1[1] = tri.uv[1].y;
        
        tmp.v3_pos[0] = v3.x;
        tmp.v3_pos[1] = v3.y;
        tmp.v3_pos[2] = v3.z;
        
        tmp.v3_tex_1[0] = tri.uv[2].x;
        tmp.v3_tex_1[1] = tri.uv[2].y;        
        
        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(tmp), &tmp.v1_pos[0]);
    }
}

}
