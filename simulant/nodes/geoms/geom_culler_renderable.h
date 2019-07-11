#pragma once

#include "../../renderers/batching/renderable.h"
#include "../../vertex_data.h"

namespace smlt {

class GeomCuller;

class GeomCullerRenderable {
public:
    GeomCullerRenderable(GeomCuller* owner, MaterialID mat_id, IndexType index_type);

    MeshArrangement arrangement() const {
        return MESH_ARRANGEMENT_TRIANGLES;
    }

    virtual VertexSpecification vertex_specification() const;
    virtual const VertexData* vertex_data() const;
    virtual const IndexData* index_data() const;
    virtual std::size_t index_element_count() const;
    virtual IndexType index_type() const;
    RenderPriority render_priority() const;
    Mat4 final_transformation() const { return Mat4(); }
    const MaterialID material_id() const { return material_id_; }
    bool is_visible() const;
    IndexData& _indices() { return indices_; }
    const AABB transformed_aabb() const;
    const AABB& aabb() const;

private:
    GeomCuller* culler_;
    IndexData indices_;
    std::shared_ptr<Geom> geom_;
    MaterialID material_id_;
};

}
