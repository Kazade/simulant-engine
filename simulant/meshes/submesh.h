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

enum MaterialSlot {
    MATERIAL_SLOT0,
    MATERIAL_SLOT1,
    MATERIAL_SLOT2,
    MATERIAL_SLOT3,
    MATERIAL_SLOT4,
    MATERIAL_SLOT5,
    MATERIAL_SLOT6,
    MATERIAL_SLOT7,
    MATERIAL_SLOT_MAX
};

class SubMesh:
    public RefCounted<SubMesh>,
    public Nameable {

public:
    SubMesh(Mesh* parent,
        const std::string& name,
        MaterialPtr material,
        std::shared_ptr<IndexData>& index_data,
        MeshArrangement arrangement = MESH_ARRANGEMENT_TRIANGLES
    );

    virtual ~SubMesh();

    void set_material(MaterialPtr material);
    void set_material_at_slot(MaterialSlot var, MaterialPtr material);

    MaterialPtr material() const;
    MaterialPtr material_at_slot(MaterialSlot var, bool fallback=false) const;

    MeshArrangement arrangement() const { return arrangement_; }

    void reverse_winding();

    void generate_texture_coordinates_cube(uint32_t texture=0);

    /* Goes through the indexes in this submesh and changes the diffuse colour of the vertices
     * they point to */
    void set_diffuse(const Colour &colour);

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
    typedef sig::signal<void (SubMeshPtr, MaterialSlot, MaterialID, MaterialID)> MaterialChangedCallback;

    MaterialChangedCallback& signal_material_changed() {
        return signal_material_changed_;
    }

private:
    friend class Mesh;

    sig::connection material_change_connection_;

    Mesh* parent_;
    sig::connection parent_update_connection_;


    std::array<MaterialPtr, MATERIAL_SLOT_MAX> materials_;

    MeshArrangement arrangement_;

    std::shared_ptr<IndexData> index_data_;

    void _recalc_bounds(AABB& bounds);

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
