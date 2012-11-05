#include <stdexcept>
#include "vertex_data.h"

namespace kglt {

void VertexData::check_or_add_attribute(AttributeBitMask attr) {
    if(data_.size() > 1 && ((enabled_bitmask_ & attr) != attr)) {
        throw std::logic_error("Attempted to add an attribute that didn't exist on the first vertex");
    }

    enabled_bitmask_ |= attr;
}

VertexData::VertexData():
    enabled_bitmask_(0),
    cursor_position_(0),
    buffer_object_(BUFFER_OBJECT_VERTEX_DATA) {

    //Default to u,v for all tex coords
    for(uint8_t i = 0; i < 8; ++i) {
        set_texture_coordinate_dimensions(i, 2);
    }
}

void VertexData::set_texture_coordinate_dimensions(uint8_t coord_index, uint8_t count) {
    if(coord_index >= 8) {
        throw std::out_of_range("coord_index must be less than 8");
    }

    if(count >= 4) {
        throw std::out_of_range("Texture coordinates can only have up to 4 parts");
    }

    tex_coord_dimensions_[coord_index] = count;
}

void VertexData::clear() {
    data_.clear();
}

void VertexData::position(float x, float y, float z) {
    check_or_add_attribute(BM_POSITIONS);

    if(cursor_position_ == (int32_t) data_.size()) {
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

void VertexData::normal(float x, float y, float z) {
    check_or_add_attribute(BM_NORMALS);

    Vertex& vert = data_.at(cursor_position_);
    vert.normal.x = x;
    vert.normal.y = y;
    vert.normal.z = z;
}

void VertexData::normal(const kmVec3& n) {
    normal(n.x, n.y, n.z);
}

void VertexData::check_texcoord(uint8_t which) {
    switch(which) {
    case 0:
        check_or_add_attribute(BM_TEXCOORD_0);
    break;
    case 1:
        check_or_add_attribute(BM_TEXCOORD_1);
    break;
    case 2:
        check_or_add_attribute(BM_TEXCOORD_2);
    break;
    case 3:
        check_or_add_attribute(BM_TEXCOORD_3);
    break;
    case 4:
        check_or_add_attribute(BM_TEXCOORD_4);
    break;
    default:
        assert(0 && "Not implemented");
    }
}

void VertexData::tex_coordX(uint8_t which, float u) {
    check_texcoord(which);

    //FIXME: throw an exception
    assert(tex_coord_dimensions_[which] >= 1);

    Vertex& vert = data_.at(cursor_position_);
    vert.tex_coords[which].x = u;
}

void VertexData::tex_coordX(uint8_t which, float u, float v) {
    check_texcoord(which);

    //FIXME: throw an exception
    assert(tex_coord_dimensions_[which] >= 2);

    Vertex& vert = data_.at(cursor_position_);
    vert.tex_coords[which].x = u;
    vert.tex_coords[which].y = v;
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w) {
    check_texcoord(which);

    //FIXME: throw an exception
    assert(tex_coord_dimensions_[which] >= 3);

    Vertex& vert = data_.at(cursor_position_);
    vert.tex_coords[which].x = u;
    vert.tex_coords[which].y = v;
    vert.tex_coords[which].z = w;
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w, float x) {
    check_texcoord(which);

    //FIXME: throw an exception
    assert(tex_coord_dimensions_[which] >= 4);

    Vertex& vert = data_.at(cursor_position_);
    vert.tex_coords[which].x = u;
    vert.tex_coords[which].y = v;
    vert.tex_coords[which].z = w;
    vert.tex_coords[which].w = x;
}

void VertexData::tex_coord0(float u) {
    tex_coordX(0, u);
}

void VertexData::tex_coord0(float u, float v) {
    tex_coordX(0, u, v);
}

void VertexData::tex_coord0(float u, float v, float w) {
    tex_coordX(0, u, v, w);
}

void VertexData::tex_coord0(float u, float v, float w, float x) {
    tex_coordX(0, u, v, w, x);
}

void VertexData::tex_coord1(float u) {
    tex_coordX(1, u);
}

void VertexData::tex_coord1(float u, float v) {
    tex_coordX(1, u, v);
}

void VertexData::tex_coord1(float u, float v, float w) {
    tex_coordX(1, u, v, w);
}

void VertexData::tex_coord1(float u, float v, float w, float x) {
    tex_coordX(1, u, v, w, x);
}

void VertexData::diffuse(float r, float g, float b, float a) {
    check_or_add_attribute(BM_DIFFUSE);

    Vertex& vert = data_.at(cursor_position_);
    vert.diffuse.r = r;
    vert.diffuse.g = g;
    vert.diffuse.b = b;
    vert.diffuse.a = a;
}

void VertexData::diffuse(const Colour& colour) {
    diffuse(colour.r, colour.g, colour.b, colour.a);
}

void VertexData::move_to(uint16_t index) {
    if(index > data_.size()) {
        throw std::out_of_range("Tried to move outside the range of the data");
    }

    cursor_position_ = index;
}

void VertexData::move_next() {
    cursor_position_++;
}

void VertexData::done() {
    assert(glGetError() == GL_NO_ERROR);
    buffer_object_.create(data_.size() * sizeof(Vertex), &data_[0]);
    assert(glGetError() == GL_NO_ERROR);
}

IndexData::IndexData():
    buffer_object_(BUFFER_OBJECT_INDEX_DATA) {

}

void IndexData::done() {
    buffer_object_.create(indices_.size() * sizeof(uint16_t), &indices_[0]);
    assert(glGetError() == GL_NO_ERROR);
}

}
