#pragma once

#include "../interfaces/boundable.h"
#include "../generic/property.h"
#include "../generic/managed.h"

namespace smlt {

class VertexData;
class IndexData;
class Mesh;
class HardwareBuffer;
class Renderer;

class SubMeshInterface:
    public Boundable {

public:
    virtual ~SubMeshInterface() {}

    virtual const MaterialID material_id() const = 0;
    virtual MeshArrangement arrangement() const = 0;

    Property<SubMeshInterface, VertexData, true> vertex_data = { this, &SubMeshInterface::get_vertex_data };
    Property<SubMeshInterface, IndexData, true> index_data = { this, &SubMeshInterface::get_index_data };

private:
    virtual VertexData* get_vertex_data() const = 0;
    virtual IndexData* get_index_data() const = 0;
};

class SubMesh :
    public SubMeshInterface,
    public Managed<SubMesh> {

public:
    SubMesh(
        Mesh* parent,
        const std::string& name,
        MaterialID material,
        MeshArrangement arrangement = MESH_ARRANGEMENT_TRIANGLES,
        IndexType index_type = INDEX_TYPE_16_BIT
    );

    virtual ~SubMesh();

    const MaterialID material_id() const;
    void set_material_id(MaterialID mat);

    MeshArrangement arrangement() const { return arrangement_; }

    const AABB& aabb() const {
        return bounds_;
    }

    void reverse_winding();
    void set_texture_on_material(uint8_t unit, TextureID tex, uint8_t pass=0);

    void _recalc_bounds();

    void generate_texture_coordinates_cube(uint32_t texture=0);

    VertexData* get_vertex_data() const;
    IndexData* get_index_data() const;

    const std::string& name() const { return name_; }

    HardwareBuffer* vertex_buffer() const;
    HardwareBuffer* index_buffer() const { return index_buffer_.get(); }

    void prepare_buffers(Renderer *renderer); // Called by actors to make sure things are up-to-date before rendering

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

public:
    typedef sig::signal<void (SubMeshPtr, MaterialID, MaterialID)> MaterialChangedCallback;

    MaterialChangedCallback& signal_material_changed() {
        return signal_material_changed_;
    }

private:
    friend class Mesh;

    sig::connection material_change_connection_;

    Mesh* parent_;
    std::string name_;

    MaterialPtr material_;
    MeshArrangement arrangement_;

    IndexData* index_data_ = nullptr;

    std::unique_ptr<HardwareBuffer> index_buffer_;

    bool index_buffer_dirty_ = false;

    AABB bounds_;

    sig::connection vrecalc_;
    sig::connection irecalc_;

    MaterialChangedCallback signal_material_changed_;

    bool contributes_to_edge_list_ = true;
};

}
