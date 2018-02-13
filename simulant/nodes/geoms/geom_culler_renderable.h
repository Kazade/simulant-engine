#pragma once

#include "../../renderers/batching/renderable.h"
#include "../../vertex_data.h"

namespace smlt {

class GeomCuller;

class GeomCullerRenderable : public Renderable {
public:
    GeomCullerRenderable(GeomCuller* owner, MaterialID mat_id, IndexType index_type);

    const MeshArrangement arrangement() const {
        return MESH_ARRANGEMENT_TRIANGLES;
    }

    virtual void prepare_buffers() = 0;

    virtual VertexSpecification vertex_attribute_specification() const;
    virtual HardwareBuffer* vertex_attribute_buffer() const;
    virtual HardwareBuffer* index_buffer() const = 0;
    virtual std::size_t index_element_count() const;
    virtual IndexType index_type() const;
    RenderPriority render_priority() const;
    Mat4 final_transformation() { return Mat4(); }
    const MaterialID material_id() const { return material_id_; }
    const bool is_visible() const;
    IndexData& _indices() { return indices_; }

private:
    GeomCuller* culler_;
    IndexData indices_;
    std::shared_ptr<Geom> geom_;
    MaterialID material_id_;
};

}
