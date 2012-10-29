#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include <stdexcept>

#include <map>
#include "object.h"
#include "types.h"
#include "generic/identifiable.h"
#include "generic/visitor.h"
#include "generic/relation.h"
#include "utils/geometry_buffer.h"

namespace kglt {

namespace newmesh {

class VertexData {
public:
    VertexData():
        cursor_position_(-1) {
    }

    void set_texture_coordinate_dimensions(uint32_t coord_index, uint32_t count);

    void move_to_start();
    void move_to(uint16_t index);
    void move_to_end();
    void move_next();

    void done();

    void position(float x, float y, float z);
    void position(const kmVec3& pos);

    void normal(float x, float y, float z);
    void normal(const kmVec3& n);

    void tex_coord0(float u);
    void tex_coord0(float u, float v);
    void tex_coord0(float u, float v, float w);
    void tex_coord0(float x, float y, float z, float w);

    void tex_coord1(float u);
    void tex_coord1(float u, float v);
    void tex_coord1(float u, float v, float w);
    void tex_coord1(float x, float y, float z, float w);

    void tex_coord2(float u);
    void tex_coord2(float u, float v);
    void tex_coord2(float u, float v, float w);
    void tex_coord2(float x, float y, float z, float w);

    void tex_coord3(float u);
    void tex_coord3(float u, float v);
    void tex_coord3(float u, float v, float w);
    void tex_coord3(float x, float y, float z, float w);

    void tex_coord4(float u);
    void tex_coord4(float u, float v);
    void tex_coord4(float u, float v, float w);
    void tex_coord4(float x, float y, float z, float w);

    void diffuse(float r, float g, float b, float a);
    void diffuse(const Colour& colour);

    void specular(float r, float g, float b, float a);
    void specular(const Colour& colour);

    bool has_positions() const;
    bool has_normals() const;
    bool has_texcoord0() const;
    bool has_texcoord1() const;
    bool has_texcoord2() const;
    bool has_texcoord3() const;
    bool has_texcoord4() const;
    bool has_diffuse() const;
    bool has_specular() const;

private:
    uint16_t tex_coord_dimensions_[8];

    struct Vertex {
        kmVec3 position;
        kmVec3 normal;
        kmVec4 tex_coords[8];
        Colour diffuse_;
        Colour specular_;
    };

    std::vector<Vertex> data_;
    int32_t cursor_position_;
};


class IndexData {
public:
    void clear() { indices_.clear(); }
    void reserve(uint16_t size) { indices_.reserve(size); }
    void index(uint16_t idx) { indices_.push_back(idx); }

private:
    std::vector<uint16_t> indices_;
};

typedef uint16_t SubMeshIndex;

class SubMesh {
public:
    typedef std::tr1::shared_ptr<SubMesh> ptr;

    SubMesh(Mesh& parent, MaterialID material, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool uses_shared_vertices=true);

    virtual VertexData& vertex_data();
    virtual IndexData& index_data();
    virtual MaterialID material() const;

private:
    Mesh& parent_;
    MaterialID material_;
    MeshArrangement arrangement_;
    bool uses_shared_data_;
};

class Mesh {
public:
    Mesh();

    VertexData& shared_data() {
        return shared_data_;
    }

    SubMeshIndex new_submesh(MaterialID material, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool uses_shared_vertices=true);
    SubMesh& submesh(SubMeshIndex index);
    void delete_submesh(SubMeshIndex index);

private:
    VertexData shared_data_;
    std::vector<SubMesh::ptr> submeshes_;

    MeshID mesh_template_;
};

class Entity;

class SubEntity {
public:
    SubEntity(Entity& parent, uint16_t idx):
        parent_(parent),
        index_(idx),
        material_(0) {
    }

    MaterialID material() const {
        if(material_) {
            return material_;
        }

        return submesh().material();
    }
    void override_material(MaterialID material) { material_ = material; }

    const VertexData& vertex_data() const { return submesh().vertex_data(); }
    const IndexData& index_data() const { return submesh().index_data(); }

private:
    Entity& parent_;
    uint16_t index_;
    MaterialID material_;

    SubMesh& submesh() { return parent_._mesh_ref().submesh(index_); }
};

class Entity {
public:
    Entity(Scene& scene):
        scene_(scene),
        mesh_(0) {}

    Entity(Scene& scene, MeshID mesh):
        scene_(scene),
        mesh_(mesh) {
    }

    void set_mesh(MeshID mesh) {
        mesh_ = mesh;
    }

    const VertexData& shared_data() const {
        return scene.mesh(mesh_).shared_data();
    }

    const uint16_t subentity_count() const {
        return subentities_.size();
    }

    SubEntity& subentity(uint32_t idx) {
        return *subentities_.at(idx);
    }

private:
    MeshID mesh_;

    friend class SubEntity;

    Mesh& _mesh_ref() { return scene_.mesh(mesh); }
};


}

struct Vertex : public Vec3 {
};

class Triangle {
public:
    Triangle():
        lightmap_id_(0),
        uses_surface_normal_(true) {}

    void set_indexes(uint32_t a, uint32_t b, uint32_t c) {
        idx_[0] = a;
        idx_[1] = b;
        idx_[2] = c;
    }

    void set_uv(uint32_t i, float u, float v) {
        uv_[i].x = u;
        uv_[i].y = v;
    }

    void set_surface_normal(float x, float y, float z) {
        surface_normal_ = Vec3(x, y, z);
        uses_surface_normal_ = true;
    }

    void set_normal(uint32_t i, float x, float y, float z) {
        normals_[i] = Vec3(x, y, z);
        uses_surface_normal_ = false;
    }

    uint32_t index(uint32_t i) { return idx_[i]; }
    Vec2& uv(uint32_t i) { return uv_[i]; }
    Vec3& normal(uint32_t i) {
        if(uses_surface_normal_) {
            return surface_normal_;
        }
        return normals_[i];
    }

private:
    Vec3 surface_normal_;

    uint32_t idx_[3];
    Vec2 uv_[3];
    Vec2 st_[3];
    Vec3 normals_[3];

    kglt::TextureID lightmap_id_;

    bool uses_surface_normal_;
};

class Mesh :
    public Object,
    public generic::Identifiable<MeshID>,
    public Relatable {

public:
    Relation<Mesh, SceneGroup> scene_group;

    VIS_DEFINE_VISITABLE();

    typedef std::tr1::shared_ptr<Mesh> ptr;

    Mesh(Scene* parent, MeshID id=0); //This must be optional for the visitor class to work :(
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
        Mesh* mesh = &parent_as<Mesh>();
        if(!is_submesh_ || !mesh) {
            throw std::logic_error("Attempted to get parent mesh from non-submesh");
        }

        return *mesh;
    }

    void add_vertex(float x, float y, float z);
    Triangle& add_triangle(uint32_t a, uint32_t b, uint32_t c);

    void set_arrangement(MeshArrangement m) { arrangement_ = m; }
    MeshArrangement arrangement() { return arrangement_; }

    std::vector<GeometryBuffer::ptr> to_geometry_buffers();
    void vbo(uint32_t vertex_attributes);

    void done() {}
    void invalidate() { buffer_cache_.clear(); }

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

    void set_branch_selectable(bool value = true) { ///< Sets this node and its children selectable or not
        branch_selectable_ = value;
    }
    bool branch_selectable() const { return branch_selectable_; }

    void apply_material(MaterialID material) { material_ = material; }
    MaterialID material() const { return material_; }

private:
    std::map<uint32_t, uint32_t> vertex_buffer_objects_;

    uint32_t build_vbo(uint32_t vertex_attributes);

    bool is_submesh_;
    bool use_parent_vertices_;

    std::vector<Mesh::ptr> submeshes_;
    std::vector<Vertex> vertices_;
    std::vector<Triangle> triangles_;

    MaterialID material_;

    MeshArrangement arrangement_;

    Colour diffuse_colour_;

    bool depth_test_enabled_;
    bool depth_writes_enabled_;
    bool branch_selectable_;

    virtual void destroy();

    std::vector<GeometryBuffer::ptr> buffer_cache_;
    bool is_dirty_;
};

}

#endif // MESH_H_INCLUDED
