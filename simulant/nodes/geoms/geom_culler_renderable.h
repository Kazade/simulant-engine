#pragma once

#include "../../renderers/batching/renderable.h"
#include "../../vertex_data.h"

namespace smlt {

class GeomCuller;

class GeomCullerRenderable : public Renderable {
public:
    GeomCullerRenderable(GeomCuller* owner, MaterialID mat_id, IndexType index_type);

    MeshArrangement arrangement() const {
        return MESH_ARRANGEMENT_TRIANGLES;
    }

    virtual void prepare_buffers(Renderer* renderer);

    virtual VertexSpecification vertex_attribute_specification() const;
    virtual HardwareBuffer* vertex_attribute_buffer() const;
    virtual HardwareBuffer* index_buffer() const;
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
    std::shared_ptr<HardwareBuffer> index_buffer_;
    GeomCuller* culler_;
    IndexData indices_;
    bool index_buffer_dirty_ = true;
    std::shared_ptr<Geom> geom_;
    MaterialID material_id_;
};

}
