#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include <stdexcept>

#include <map>
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

class Triangle {
public:
    Triangle():
        lightmap_id_(0) {}

    void set_indexes(uint32_t a, uint32_t b, uint32_t c) {
        idx_[0] = a;
        idx_[1] = b;
        idx_[2] = c;
    }

    void set_uv(uint32_t i, float u, float v) {
        uv_[i].x = u;
        uv_[i].y = v;
    }

    uint32_t index(uint32_t i) { return idx_[i]; }
    Vec2& uv(uint32_t i) { return uv_[i]; }

private:
    uint32_t idx_[3];
    Vec2 uv_[3];
    Vec2 st_[3];
    kglt::TextureID lightmap_id_;

};

enum MeshArrangement {
	MESH_ARRANGEMENT_POINTS,
	MESH_ARRANGEMENT_TRIANGLES,
	MESH_ARRANGEMENT_TRIANGLE_FAN,
	MESH_ARRANGEMENT_TRIANGLE_STRIP,
	MESH_ARRANGEMENT_LINES,
	MESH_ARRANGEMENT_LINE_STRIP
};

enum VertexAttribute {
	VERTEX_ATTRIBUTE_POSITION = 1,
	VERTEX_ATTRIBUTE_TEXCOORD_1 = 2,
	VERTEX_ATTRIBUTE_DIFFUSE = 4
};

class Mesh : public Object {
public:
    typedef std::tr1::shared_ptr<Mesh> ptr;

    Mesh();
    ~Mesh();

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
		visitor.pre_visit(this);
		
        for(Object* child: children_) {
            child->accept(visitor);
        }

		if(is_visible()) {
			visitor.visit(this);
		}
        visitor.post_visit(this);
    }

    void set_arrangement(MeshArrangement m) { arrangement_ = m; }
    MeshArrangement arrangement() { return arrangement_; }
	
    void vbo(uint32_t vertex_attributes);

    void done() {}
    void invalidate() { vertex_buffer_objects_.clear(); }

	/*
	 * 	FIXME: This should apply to the triangles, not the mesh itself
	 */
	void set_diffuse_colour(const Colour& colour) { 
		diffuse_colour_ = colour; 
		invalidate();
	}

    bool depth_test_enabled() const { return depth_test_enabled_; }
    bool depth_writes_enabled() const { return depth_writes_enabled_; }

    void enable_depth_test(bool value=true) { depth_test_enabled_ = value; }
    void enable_depth_writes(bool value=true) { depth_writes_enabled_ = value; }

private:
	std::map<uint32_t, uint32_t> vertex_buffer_objects_;

	uint32_t build_vbo(uint32_t vertex_attributes);

    bool is_submesh_;
    bool use_parent_vertices_;

    std::vector<Mesh::ptr> submeshes_;
    std::vector<Vertex> vertices_;
    std::vector<Triangle> triangles_;
    TextureID textures_[MAX_TEXTURE_LEVELS];

    MeshArrangement arrangement_;
    
    Colour diffuse_colour_;

    bool depth_test_enabled_;
    bool depth_writes_enabled_;
};

}

#endif // MESH_H_INCLUDED
