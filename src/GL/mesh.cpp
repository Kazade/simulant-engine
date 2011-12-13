
#include "mesh.h"

namespace GL {

Vertex& Mesh::vertex(uint32_t v) {
    return vertices_[v];
}

Triangle& Mesh::triangle(uint32_t t) {
    return triangles_[t];
}

TextureID& Mesh::texture(uint32_t l) {
    return textures_[l];
}

void Mesh::apply_texture(uint32_t level, TextureID tex) {
    //FIXME: Apply to all triangles

    assert(level < MAX_TEXTURES_PER_MESH);
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
    return triangles_[triangles_.size() - 1];
}

}
