#pragma once

#include "../interfaces/boundable.h"
#include "../generic/property.h"
#include "../generic/managed.h"
#include "../interfaces/nameable.h"

namespace smlt {

class VertexData;
class IndexData;
class Mesh;
class Renderer;

enum MaterialSlot : uint8_t {
    MATERIAL_SLOT0 = 0,
    MATERIAL_SLOT1,
    MATERIAL_SLOT2,
    MATERIAL_SLOT3,
    MATERIAL_SLOT4,
    MATERIAL_SLOT5,
    MATERIAL_SLOT6,
    MATERIAL_SLOT7,
    MATERIAL_SLOT_MAX
};

enum SubmeshType {
    SUBMESH_TYPE_INDEXED,
    SUBMESH_TYPE_RANGED,
};

struct VertexRange {
    uint32_t start;
    uint32_t count;
};

typedef std::vector<VertexRange> VertexRangeList;


class SubMesh:
    public RefCounted<SubMesh>,
    public Nameable {

public:
    /* Indexed submesh constructor */
    SubMesh(Mesh* parent,
        const std::string& name,
        MaterialPtr material,
        std::shared_ptr<IndexData>& index_data,
        MeshArrangement arrangement = MESH_ARRANGEMENT_TRIANGLES
    );

    /* Ranged submesh constructor */
    SubMesh(Mesh* parent,
        const std::string& name,
        MaterialPtr material,
        MeshArrangement arrangement = MESH_ARRANGEMENT_TRIANGLES
    );

    virtual ~SubMesh();

    SubmeshType type() const;

    /**
     * @brief Add a vertex range to this submesh. `count` vertices will be rendered
     * from the `start` index in the mesh vertex data.
     * @param start
     * @param count
     */
    bool add_vertex_range(uint32_t start, uint32_t count);
    void mark_changed();

    const VertexRange* vertex_ranges() const {
        return &vertex_ranges_[0];
    }

    std::size_t vertex_range_count() const {
        return vertex_ranges_.size();
    }

    void remove_all_vertex_ranges();

    void set_material(const MaterialPtr &material);
    void set_material_at_slot(MaterialSlot var, const MaterialPtr& material);

    const MaterialPtr& material() const;
    const MaterialPtr& material_at_slot(MaterialSlot var, bool fallback=false) const;

    MeshArrangement arrangement() const { return arrangement_; }

    bool reverse_winding();

    void generate_texture_coordinates_cube(uint32_t texture=0);

    /* Goes through the indexes in this submesh and changes the diffuse color of the vertices
     * they point to */
    void set_diffuse(const Color &color);

    /*
     * Whether or not this submesh contributes to the adjacency info attached to the mesh
     * which is used for silhouettes
     */
    void set_contributes_to_edge_list(bool v=true) {
        contributes_to_edge_list_ = v;
    }

    bool contributes_to_edge_list() const {
        return contributes_to_edge_list_;
    }

    /*
     * Iterates the submesh indexes and breaks them into triangles (a, b, c). In the case of lines
     * index c will be the same as index a.
     */
    void each_triangle(std::function<void (uint32_t, uint32_t, uint32_t)> cb);

    /** Return the axis-aligned bounding box for the faces making up
     *  this submesh */
    const AABB& aabb() const;

public:
    typedef sig::signal<void (SubMeshPtr, MaterialSlot, AssetID, AssetID)> MaterialChangedCallback;

    MaterialChangedCallback& signal_material_changed() {
        return signal_material_changed_;
    }

private:
    friend class Mesh;

    sig::connection material_change_connection_;

    Mesh* parent_;
    SubmeshType type_;
    sig::connection parent_update_connection_;

    std::array<MaterialPtr, MATERIAL_SLOT_MAX> materials_;

    MeshArrangement arrangement_;

    std::shared_ptr<IndexData> index_data_;
    VertexRangeList vertex_ranges_;

    void _recalc_bounds(AABB& bounds);
    void _recalc_bounds_indexed(AABB& bounds);
    void _recalc_bounds_ranged(AABB& bounds);

    void _each_triangle_indexed(std::function<void (uint32_t, uint32_t, uint32_t)> cb);
    void _each_triangle_ranged(std::function<void (uint32_t, uint32_t, uint32_t)> cb);

    MaterialChangedCallback signal_material_changed_;

    bool contributes_to_edge_list_ = true;

    /* This is updated and maintained by Mesh, it only lives here so
     * there's a fast lookup for each submesh */
    AABB bounds_;

public:
    S_DEFINE_PROPERTY(mesh, &SubMesh::parent_);
    S_DEFINE_PROPERTY(index_data, &SubMesh::index_data_);
};

}
