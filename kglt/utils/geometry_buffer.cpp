#include "kazbase/logging/logging.h"
#include "geometry_buffer.h"

namespace kglt {

GeometryBuffer::GeometryBuffer(MeshArrangement arrangement, uint32_t attributes):
    arrangement_(arrangement),
    attributes_(attributes),
    stride_(0),
    material_(0),
    is_dirty_(false),
    vertex_buffer_(0) {

    if((attributes_ & VERTEX_ATTRIBUTE_POSITION) == VERTEX_ATTRIBUTE_POSITION) {
        stride_ += sizeof(float) * 3;
    }

    if((attributes_ & VERTEX_ATTRIBUTE_TEXCOORD_1) == VERTEX_ATTRIBUTE_TEXCOORD_1) {
        stride_ += sizeof(float) * 2;
    }

    if((attributes_ & VERTEX_ATTRIBUTE_NORMAL) == VERTEX_ATTRIBUTE_NORMAL) {
        stride_ += sizeof(float) * 3;
    }

    if((attributes_ & VERTEX_ATTRIBUTE_DIFFUSE) == VERTEX_ATTRIBUTE_DIFFUSE) {
        stride_ += sizeof(float) * 4;
    }
}

void GeometryBuffer::resize(uint32_t vertex_count) {
    uint32_t floats_per_element = stride() / sizeof(float);
    buffer_.resize(floats_per_element * vertex_count);
}

MeshArrangement GeometryBuffer::arrangement() const {
    return arrangement_;
}

uint32_t GeometryBuffer::attributes() const {
    return attributes_;
}

uint32_t GeometryBuffer::stride() const {
    return stride_;
}

MaterialID GeometryBuffer::material() const {
    return material_;
}

float* GeometryBuffer::vertex(uint32_t index) {
    uint32_t floats_per_element = stride() / sizeof(float);
    return &buffer_.at(index * floats_per_element);
}

int32_t GeometryBuffer::offset(VertexAttribute attr) {
    if(!has_attribute(attr)) {
        return -1;
    }

    int32_t offset = 0;
    if(attr == VERTEX_ATTRIBUTE_POSITION) return offset;
    if(has_attribute(VERTEX_ATTRIBUTE_POSITION)) {
        offset += sizeof(float) * 3;
    }

    if(attr == VERTEX_ATTRIBUTE_TEXCOORD_1) return offset;
    if(has_attribute(VERTEX_ATTRIBUTE_TEXCOORD_1)) {
        offset += sizeof(float) * 2;
    }

    if(attr == VERTEX_ATTRIBUTE_NORMAL) return offset;
    if(has_attribute(VERTEX_ATTRIBUTE_NORMAL)) {
        offset += sizeof(float) * 3;
    }

    if(attr == VERTEX_ATTRIBUTE_DIFFUSE) return offset;
    if(has_attribute(VERTEX_ATTRIBUTE_DIFFUSE)) {
        offset += sizeof(float) * 4;
    }

    assert(0 && "Something is wrong");
}

uint32_t GeometryBuffer::count() const {
    return buffer_.size() / (stride() / sizeof(float));
}

GLuint GeometryBuffer::vbo() {
    if(!vertex_buffer_) {
        glGenBuffers(1, &vertex_buffer_);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
        if(!buffer_.empty()) {
            glBufferData(GL_ARRAY_BUFFER, buffer_.size() * sizeof(float), vertex(0), GL_STATIC_DRAW);
        } else {
            L_WARN("Tried to create a VBO with no data");
        }
    } /*else {
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, stride() * count(), start());
    }*/

    return vertex_buffer_;
}

}
