#ifndef __ANDROID__
	#include <GL/glew.h>
#else
	#include <GLES3/gl3.h>
#endif

#include "../kazbase/logging.h"
#include "geometry_buffer.h"
#include "../utils/gl_error.h"

namespace kglt {

GeometryBuffer::GeometryBuffer(MeshArrangement arrangement, uint32_t attributes):
    arrangement_(arrangement),
    attributes_(attributes),
    stride_(0),
    material_(0),
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

void GeometryBuffer::set_vertex(uint32_t vertex, const GeometryBufferEntry& values) {
    uint32_t start_index = vertex * floats_per_vertex();

    for(VertexAttribute attr: {
        VERTEX_ATTRIBUTE_POSITION,
        VERTEX_ATTRIBUTE_TEXCOORD_1,
        VERTEX_ATTRIBUTE_NORMAL,
        VERTEX_ATTRIBUTE_DIFFUSE}) {

        if(has_attribute(attr)) {
            uint32_t float_index = start_index + (offset(attr) / sizeof(float)); //Find the first index

            if(attr == VERTEX_ATTRIBUTE_POSITION) {
                buffer_.at(float_index) = values.position.x;
                buffer_.at(float_index + 1) = values.position.y;
                buffer_.at(float_index + 2) = values.position.z;
            } else if(attr == VERTEX_ATTRIBUTE_TEXCOORD_1) {
                buffer_.at(float_index) = values.texcoord_1.x;
                buffer_.at(float_index + 1) = values.texcoord_1.y;
            } else if(attr == VERTEX_ATTRIBUTE_NORMAL) {
                buffer_.at(float_index) = values.normal.x;
                buffer_.at(float_index + 1) = values.normal.y;
            } else if(attr == VERTEX_ATTRIBUTE_DIFFUSE) {
                buffer_.at(float_index) = values.diffuse.r;
                buffer_.at(float_index + 1) = values.diffuse.g;
                buffer_.at(float_index + 2) = values.diffuse.b;
                buffer_.at(float_index + 3) = values.diffuse.a;
            }
        }
    }
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
        GLCheck(glGenBuffers, 1, &vertex_buffer_);
        GLCheck(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_);
        if(!buffer_.empty()) {
            GLCheck(glBufferData, GL_ARRAY_BUFFER, buffer_.size() * sizeof(float), &buffer_[0], GL_STATIC_DRAW);
        } else {
            L_WARN("Tried to create a VBO with no data");
        }
    } else {
        GLCheck(glBindBuffer, GL_ARRAY_BUFFER, vertex_buffer_);
        //GLCheck(glBufferData, GL_ARRAY_BUFFER, buffer_.size() * sizeof(float), &buffer_[0], GL_STATIC_DRAW);
    }

    return vertex_buffer_;
}

}
