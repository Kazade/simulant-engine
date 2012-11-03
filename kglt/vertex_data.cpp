#include <stdexcept>
#include "vertex_data.h"

namespace kglt {

void VertexData::check_or_add_attribute(AttributeBitMask attr) {
    if(!data_.empty() && ((enabled_bitmask_ & attr) != attr)) {
        throw std::logic_error("Attempted to add an attribute that didn't exist on the first vertex");
    }

    enabled_bitmask_ |= attr;
}

VertexData::VertexData():
    enabled_bitmask_(0),
    tex_coord_dimensions_{0},
    cursor_position_(0) {
}

void VertexData::position(float x, float y, float z) {
    check_or_add_attribute(BM_POSITIONS);

    if(cursor_position_ == data_.size()) {
        data_.push_back(Vertex());
    }

    Vertex& vert = data_.at(cursor_position_);

    vert.position.x = x;
    vert.position.y = y;
    vert.position.z = z;
}

void VertexData::position(const kmVec3& pos) {
    position(pos.x, pos.y, pos.z);
}

void VertexData::move_next() {
    cursor_position_++;
}

void VertexData::done() {
    //pass
}

}
