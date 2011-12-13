#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "object.h"
#include "types.h"
#include "object_visitor.h"

#define MAX_TEXTURES_PER_MESH 1

namespace GL {

struct Vertex : public Vec3 {
    Vec2 uv; //Make a vector?
};

struct Triangle {
    uint32_t a, b, c;
};

enum MeshArrangement {
    POINTS,
    TRIANGLES,
    TRIANGLE_FAN
};

class Mesh : public Object {
public:
    Mesh() {
        textures_.resize(MAX_TEXTURES_PER_MESH, 0); //Allow up to 5 textures on a mesh
        set_arrangement(MeshArrangement::TRIANGLES);
    }

    void add_submesh() {
        Mesh new_mesh;
        submeshes_.push_back(new_mesh);
        submeshes_[submeshes_.size() - 1].set_parent(this); //Add to the tree
    }

    Mesh& submesh(uint32_t m = 0) {
        assert(m < submeshes_.size());
        return submeshes_[m];
    }

    std::vector<Mesh>& submeshes() {
        return submeshes_;
    }

    Vertex& vertex(uint32_t v = 0);
    Triangle& triangle(uint32_t t = 0);
    std::vector<Triangle>& triangles() {
        return triangles_;
    }

    std::vector<Vertex>& vertices() {
        return vertices_;
    }

    TextureID& texture(uint32_t l = 0);

    void apply_texture(uint32_t level, TextureID tex);
    void add_vertex(float x, float y, float z, float u=0.0f, float v=0.0f);
    void add_triangle(uint32_t a, uint32_t b, uint32_t c);

    void accept(ObjectVisitor& visitor) {
        for(Object* child: children_) {
            child->accept(visitor);
        }

        visitor.visit(this);
    }

    void set_arrangement(MeshArrangement m) { arrangement_ = m; }
    MeshArrangement arrangement() { return arrangement_; }

private:
    std::vector<Mesh> submeshes_;
    std::vector<Vertex> vertices_;
    std::vector<Triangle> triangles_;
    std::vector<TextureID> textures_;

    MeshArrangement arrangement_;
};

}

#endif // MESH_H_INCLUDED
