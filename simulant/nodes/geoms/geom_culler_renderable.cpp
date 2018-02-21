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

    indices_.signal_update_complete().connect([&]() {
        index_buffer_dirty_ = true;
    });
}

void GeomCullerRenderable::prepare_buffers(Renderer *renderer) {
    // Make sure the owner culler has a chance to do whatever it needs to
    culler_->_prepare_buffers(renderer);

    if(!index_buffer_) {
        index_buffer_ = renderer->hardware_buffers->allocate(
            indices_.data_size(),
            HARDWARE_BUFFER_VERTEX_ARRAY_INDICES,
            SHADOW_BUFFER_DISABLED
        );
    }

    if(index_buffer_dirty_) {
        index_buffer_dirty_ = false;
        if(indices_.data_size() > index_buffer_->size()) {
            index_buffer_->resize(indices_.data_size());
        }
        index_buffer_->upload(indices_);
    }
}

VertexSpecification GeomCullerRenderable::vertex_attribute_specification() const {
    return culler_->_vertex_data()->specification();
}

HardwareBuffer *GeomCullerRenderable::vertex_attribute_buffer() const {
    return culler_->_vertex_attribute_buffer();
}

HardwareBuffer *GeomCullerRenderable::index_buffer() const {
    return index_buffer_.get();
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

const bool GeomCullerRenderable::is_visible() const {
    return culler_->geom_->is_visible();
}

const AABB GeomCullerRenderable::transformed_aabb() const {
    return aabb();
}

const AABB& GeomCullerRenderable::aabb() const {
    return culler_->geom_->aabb();
}



}
