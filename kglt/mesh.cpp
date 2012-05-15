#include "glee/GLee.h"
#include "mesh.h"

namespace kglt {

Mesh::Mesh():
    Object(),
    is_submesh_(false),
    use_parent_vertices_(false),
    diffuse_colour_(1.0, 1.0, 1.0, 1.0) {

    glGenBuffers(1, &vertex_buffer_);

    //Set all textures to a NullTextureID
    for(uint32_t i = 0; i < MAX_TEXTURE_LEVELS; ++i) {
        textures_[i] = NullTextureID;
    }

    set_arrangement(MeshArrangement::TRIANGLES);
}

Mesh::~Mesh() {
    glDeleteBuffers(1, &vertex_buffer_);
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
    t.set_indexes(a, b, c);
    triangles_.push_back(t);
    
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

void Mesh::activate_vbo() {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
}
    
void Mesh::update_vbo() {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    
    struct VData {
        float pos[3];
        float uv[2];
        float diffuse[4];
    };

    uint32_t size_in_bytes = triangles().size() * sizeof(VData) * 3;
    
    glBufferData(GL_ARRAY_BUFFER, size_in_bytes, NULL, GL_STATIC_DRAW);
    
    uint32_t offset = 0;
    for(Triangle& tri: triangles()) {        
        for(uint32_t j = 0; j < 3; ++j) {
            Vertex& v = vertices()[tri.index(j)];

            VData data;
            data.pos[0] = v.x;
            data.pos[1] = v.y;
            data.pos[2] = v.z;
            data.uv[0] = tri.uv(j).x;
            data.uv[1] = tri.uv(j).y;
            data.diffuse[0] = diffuse_colour_.r;
            data.diffuse[1] = diffuse_colour_.g;
            data.diffuse[2] = diffuse_colour_.b;
            data.diffuse[3] = diffuse_colour_.a;
            
            glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(VData), &data);
            offset += sizeof(VData);
        }                
    }
}

}
