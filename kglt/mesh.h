#ifndef NEWMESH_H
#define NEWMESH_H

#include <cstdint>
#include <vector>
#include <tr1/unordered_map>
#include <set>

#include "generic/managed.h"
#include "generic/identifiable.h"

#include "kazmath/kazmath.h"
#include "kglt/kazbase/list_utils.h"

#include "loadable.h"
#include "resource.h"
#include "vertex_data.h"
#include "types.h"

namespace kglt {

class ResourceManager;

typedef uint16_t SubMeshIndex;

class MeshInterface {
public:
    virtual ~MeshInterface() {}
    virtual const VertexData& shared_data() const = 0;
};

class SubMeshInterface {
public:
    virtual ~SubMeshInterface() {}

    virtual const VertexData& vertex_data() const = 0;
    virtual const IndexData& index_data() const = 0;
    virtual const MaterialID material_id() const = 0;
    virtual const MeshArrangement arrangement() const = 0;
};

class SubMesh :
    public SubMeshInterface,
    public Managed<SubMesh> {

public:
    SubMesh(Mesh& parent, MaterialID material, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool uses_shared_vertices=true);
    virtual ~SubMesh();

    VertexData& vertex_data();
    IndexData& index_data();

    const VertexData& vertex_data() const;
    const IndexData& index_data() const;

    const MaterialID material_id() const { return material_; }
    void set_material_id(MaterialID mat) { material_ = mat; }

    const MeshArrangement arrangement() const { return arrangement_; }

    const kmAABB& bounds() const {
        return bounds_;
    }

    void recalc_bounds();

    void reverse_winding();
    void transform_vertices(const kmMat4& transformation);
private:
    Mesh& parent_;
    MaterialID material_;
    MeshArrangement arrangement_;
    bool uses_shared_data_;

    VertexData vertex_data_;
    IndexData index_data_;

    kmAABB bounds_;

    sigc::connection vrecalc_;
    sigc::connection irecalc_;
};

class Mesh :
    public MeshInterface,
    public Resource,
    public Loadable,
    public Managed<Mesh>,
    public generic::Identifiable<MeshID> {

public:
    Mesh(ResourceManager* resource_manager, MeshID id);

    VertexData& shared_data() {
        return shared_data_;
    }

    const VertexData& shared_data() const {
        return shared_data_;
    }

    SubMeshIndex new_submesh(MaterialID material, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool uses_shared_vertices=true);
    SubMesh& submesh(SubMeshIndex index);
    void delete_submesh(SubMeshIndex index);
    void clear();

    std::vector<SubMeshIndex> submesh_ids() {
        std::set<SubMeshIndex> keys = container::keys(submeshes_by_index_);
        return std::vector<SubMeshIndex>(keys.begin(), keys.end());
    }

    void enable_debug(bool value);

    void set_material(MaterialID material); ///< Apply material to all submeshes
    void reverse_winding(); ///< Reverse the winding of all submeshes

private:
    VertexData shared_data_;
    std::vector<SubMesh::ptr> submeshes_;
    std::tr1::unordered_map<SubMeshIndex, SubMesh::ptr> submeshes_by_index_;

    SubMeshIndex normal_debug_mesh_;
};

}

#endif // NEWMESH_H
