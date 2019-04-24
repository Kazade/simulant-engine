#include "geom_culler_renderable.h"
#include "../geom.h"
#include "geom_culler.h"
#include "../../hardware_buffer.h"
#include "../../renderers/renderer.h"

namespace smlt {

GeomCullerRenderable::GeomCullerRenderable(GeomCuller *owner, MaterialID mat_id, IndexType index_type):
    culler_(owner),
    indices_(index_type),
    material_id_(mat_id) {
}

VertexSpecification GeomCullerRenderable::vertex_specification() const {
    return culler_->_vertex_data()->specification();
}

const VertexData *GeomCullerRenderable::vertex_data() const {
    return culler_->_vertex_data();
}

const IndexData *GeomCullerRenderable::index_data() const {
    return &indices_;
}

std::size_t GeomCullerRenderable::index_element_count() const {
    return indices_.count();
}

IndexType GeomCullerRenderable::index_type() const {
    return indices_.index_type();
}

RenderPriority GeomCullerRenderable::render_priority() const {
    return culler_->geom_->render_priority();
}

bool GeomCullerRenderable::is_visible() const {
    return culler_->geom_->is_visible();
}

const AABB GeomCullerRenderable::transformed_aabb() const {
    return aabb();
}

const AABB& GeomCullerRenderable::aabb() const {
    return culler_->geom_->aabb();
}



}
