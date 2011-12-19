#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include <stdexcept>
#include "object.h"
#include "types.h"
#include "object_visitor.h"

namespace kglt {

enum TextureLevel {
    PRIMARY,
    SECONDARY,
    MAX_TEXTURE_LEVELS
};

struct Vertex : public Vec3 {
};

struct Triangle {
    uint32_t idx[3];
    Vec2 uv[3];
    Vec2 st[3];
    kglt::TextureID lightmap_id;

    Triangle():
        lightmap_id(0) {}
};

enum MeshArrangement {
    POINTS,
    TRIANGLES,
    TRIANGLE_FAN
};

class Mesh : public Object {
public:
    typedef std::tr1::shared_ptr<Mesh> ptr;

    Mesh():
        Object(),
        is_submesh_(false),
        use_parent_vertices_(false) {

        //Set all textures to a NullTextureID
        for(uint32_t i = 0; i < MAX_TEXTURE_LEVELS; ++i) {
            textures_[i] = NullTextureID;
        }

        set_arrangement(MeshArrangement::TRIANGLES);
    }

    uint32_t add_submesh(bool use_parent_vertices=false);

    Mesh& submesh(uint32_t m = 0) {
        assert(m < submeshes_.size());
        return *submeshes_[m];
    }

    std::vector<Mesh::ptr>& submeshes() {
        return submeshes_;
    }

    Vertex& vertex(uint32_t v = 0);
    Triangle& triangle(uint32_t t = 0);

    std::vector<Triangle>& triangles() {
        return triangles_;
    }

    std::vector<Vertex>& vertices() {
        if(use_parent_vertices_) {
            if(!is_submesh_) {
                throw std::logic_error("Attempted to use parent vertices on a non-submesh");
            }
            return parent_mesh().vertices_;
        }
        return vertices_;
    }

    Mesh& parent_mesh() {
        Mesh* mesh = dynamic_cast<Mesh*>(parent_);
        if(!is_submesh_ || !mesh) {
            throw std::logic_error("Attempted to get parent mesh from non-submesh");
        }

        return *mesh;
    }

    TextureID& texture(TextureLevel l = PRIMARY);

    void apply_texture(TextureID tex, TextureLevel level=PRIMARY);
    void add_vertex(float x, float y, float z);
    Triangle& add_triangle(uint32_t a, uint32_t b, uint32_t c);

    void accept(ObjectVisitor& visitor) {
        for(Object* child: children_) {
            child->accept(visitor);
        }

        visitor.visit(this);
    }

    void set_arrangement(MeshArrangement m) { arrangement_ = m; }
    MeshArrangement arrangement() { return arrangement_; }

private:
    bool is_submesh_;
    bool use_parent_vertices_;

    std::vector<Mesh::ptr> submeshes_;
    std::vector<Vertex> vertices_;
    std::vector<Triangle> triangles_;
    TextureID textures_[MAX_TEXTURE_LEVELS];

    MeshArrangement arrangement_;
};

}

#endif // MESH_H_INCLUDED
