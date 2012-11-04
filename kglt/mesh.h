#ifndef NEWMESH_H
#define NEWMESH_H

#include <cstdint>
#include <vector>

#include "generic/managed.h"
#include "generic/identifiable.h"

#include "vertex_data.h"
#include "types.h"

namespace kglt {

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
    virtual const MaterialID material() const = 0;
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

    const MaterialID material() const { return material_; }
    void set_material(MaterialID mat) { material_ = mat; }

    const MeshArrangement arrangement() const { return arrangement_; }
private:
    Mesh& parent_;
    MaterialID material_;
    MeshArrangement arrangement_;
    bool uses_shared_data_;

    VertexData vertex_data_;
    IndexData index_data_;
};

class Mesh :
    public MeshInterface,
    public Managed<Mesh>,
    public generic::Identifiable<MeshID> {

public:
    Mesh(Scene* scene, MeshID id);

    VertexData& shared_data() {
        return shared_data_;
    }

    const VertexData& shared_data() const {
        return shared_data_;
    }

    SubMeshIndex new_submesh(MaterialID material, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool uses_shared_vertices=true);
    SubMesh& submesh(SubMeshIndex index);
    void delete_submesh(SubMeshIndex index);

    const uint16_t submesh_count() const { return submeshes_.size(); }

    void clear();

private:
    Scene& scene_;

    VertexData shared_data_;
    std::vector<SubMesh::ptr> submeshes_;
};

}

#endif // NEWMESH_H
