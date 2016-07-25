#include "utils/glcompat.h"

#include <stdexcept>
#include "vertex_data.h"
#include "window_base.h"
#include "utils/gl_thread_check.h"

namespace kglt {

const VertexSpecification VertexSpecification::DEFAULT = {
    VERTEX_ATTRIBUTE_3F,
    VERTEX_ATTRIBUTE_3F,
    VERTEX_ATTRIBUTE_2F,
    VERTEX_ATTRIBUTE_2F,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_4F,
    VERTEX_ATTRIBUTE_NONE
};

const VertexSpecification VertexSpecification::POSITION_ONLY = {
    VERTEX_ATTRIBUTE_3F
};

const VertexSpecification VertexSpecification::POSITION_AND_DIFFUSE = {
    VERTEX_ATTRIBUTE_3F,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_4F
};

uint32_t vertex_attribute_size(VertexAttribute attr) {
    switch(attr) {
        case VERTEX_ATTRIBUTE_NONE: return 0;
        case VERTEX_ATTRIBUTE_2F: return sizeof(float) * 2;
        case VERTEX_ATTRIBUTE_3F:  return sizeof(float) * 3;
        case VERTEX_ATTRIBUTE_4F: return sizeof(float) * 4;
        default:
            assert(0 && "Invalid attribute specified");
    }
}

VertexData::VertexData(VertexSpecification vertex_specification):
    cursor_position_(0) {

    reset(vertex_specification);
}

void VertexData::clear() {
    data_.clear();
    cursor_position_ = 0;    
}

void VertexData::position_checks() {
    if(!has_positions()) {
        throw std::logic_error("Vertex data has no position attribute");
    }

    if(cursor_position_ == (int32_t) data_.size()) {
        push_back();
    } else if(cursor_position_ > (int32_t) data_.size()) {
        throw std::out_of_range("Cursor moved out of range");
    }
}

void VertexData::position(float x, float y, float z) {
    position_checks();

    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_3F);
    Vec3* out = (Vec3*) &data_[cursor_position_];
    *out = Vec3(x, y, z);
}

void VertexData::position(float x, float y) {
    position_checks();

    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_2F);
    Vec2* out = (Vec2*) &data_[cursor_position_];
    *out = Vec2(x, y);
}

void VertexData::position(const kmVec2 &pos) {
    position(pos.x, pos.y);
}

void VertexData::position(const kmVec3& pos) {
    position(pos.x, pos.y, pos.z);
}

template<>
Vec2 VertexData::position_at<Vec2>(uint32_t idx) {
    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_2F);
    Vec2 out = *((Vec2*) &data_[idx * stride()]);
    return out;
}

template<>
Vec3 VertexData::position_at<Vec3>(uint32_t idx) {
    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_3F);
    Vec3 out = *((Vec3*) &data_[idx * stride()]);
    return out;
}

template<>
Vec4 VertexData::position_at<Vec4>(uint32_t idx) {
    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_4F);
    Vec4 out = *((Vec4*) &data_[idx * stride()]);
    return out;
}

void VertexData::normal(float x, float y, float z) {
    assert(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_3F);
    Vec3* out = (Vec3*) &data_[cursor_position_ + normal_offset()];
    *out = Vec3(x, y, z);
}

void VertexData::normal(const kmVec3& n) {
    normal(n.x, n.y, n.z);
}

void VertexData::tex_coordX(uint8_t which, float u, float v) {
    uint32_t offset = 0;
    switch(which) {
        case 0: offset = texcoord0_offset(); break;
        case 1: offset = texcoord1_offset(); break;
        case 2: offset = texcoord2_offset(); break;
        case 3: offset = texcoord3_offset(); break;
    default:
        throw std::logic_error("Invalid texture coordinate specified");
    }

    Vec2* out = (Vec2*) &data_[cursor_position_ + offset];
    *out = Vec2(u, v);
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w) {
    uint32_t offset = 0;
    switch(which) {
        case 0: offset = texcoord0_offset(); break;
        case 1: offset = texcoord1_offset(); break;
        case 2: offset = texcoord2_offset(); break;
        case 3: offset = texcoord3_offset(); break;
    default:
        throw std::logic_error("Invalid texture coordinate specified");
    }

    Vec3* out = (Vec3*) &data_[cursor_position_ + offset];
    *out = Vec3(u, v, w);
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w, float x) {
    uint32_t offset = 0;
    switch(which) {
        case 0: offset = texcoord0_offset(); break;
        case 1: offset = texcoord1_offset(); break;
        case 2: offset = texcoord2_offset(); break;
        case 3: offset = texcoord3_offset(); break;
    default:
        throw std::logic_error("Invalid texture coordinate specified");
    }

    Vec4* out = (Vec4*) &data_[cursor_position_ + offset];
    *out = Vec4(u, v, w, x);
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

template<>
Vec2 VertexData::texcoord0_at<Vec2>(uint32_t idx) {
    assert(vertex_specification_.texcoord0_attribute == VERTEX_ATTRIBUTE_2F);
    Vec2 out = *((Vec2*) &data_[(idx * stride()) + texcoord0_offset()]);
    return out;
}

template<>
Vec3 VertexData::texcoord0_at<Vec3>(uint32_t idx) {
    assert(vertex_specification_.texcoord0_attribute == VERTEX_ATTRIBUTE_3F);
    Vec3 out = *((Vec3*) &data_[(idx * stride()) + texcoord0_offset()]);
    return out;
}

template<>
Vec4 VertexData::texcoord0_at<Vec4>(uint32_t idx) {
    assert(vertex_specification_.texcoord0_attribute == VERTEX_ATTRIBUTE_4F);
    Vec4 out = *((Vec4*) &data_[(idx * stride()) + texcoord0_offset()]);
    return out;
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

void VertexData::tex_coord2(float u, float v) {
    tex_coordX(2, u, v);
}

void VertexData::tex_coord2(float u, float v, float w) {
    tex_coordX(2, u, v, w);
}

void VertexData::tex_coord2(float u, float v, float w, float x) {
    tex_coordX(2, u, v, w, x);
}

void VertexData::tex_coord3(float u, float v) {
    tex_coordX(3, u, v);
}

void VertexData::tex_coord3(float u, float v, float w) {
    tex_coordX(3, u, v, w);
}

void VertexData::tex_coord3(float u, float v, float w, float x) {
    tex_coordX(3, u, v, w, x);
}

void VertexData::diffuse(float r, float g, float b, float a) {
    assert(vertex_specification_.diffuse_attribute == VERTEX_ATTRIBUTE_4F);
    Vec4* out = (Vec4*) &data_[cursor_position_ + diffuse_offset()];
    *out = Vec4(r, g, b, a);
}

void VertexData::diffuse(const Colour& colour) {
    diffuse(colour.r, colour.g, colour.b, colour.a);
}

void VertexData::move_to_start() {
    move_to(0);
}

void VertexData::move_to_end() {
    move_to(data_.size());
}

void VertexData::move_by(int32_t amount) {
    cursor_position_ += amount;
}

void VertexData::move_to(int32_t index) {
    int32_t offset = int32_t(stride()) * index;

    if(offset > (int32_t) data_.size()) {
        throw std::out_of_range("Tried to move outside the range of the data");
    }

    cursor_position_ = offset;
}

uint16_t VertexData::move_next() {
    int32_t new_pos = cursor_position_ += stride();

    //cursor_position_ == data_.size() is allowed (see position())
    if(new_pos > (int32_t) data_.size()) {
        throw std::out_of_range("Cursor moved out of range");
    }

    cursor_position_ = new_pos;
    return cursor_position_;
}

void VertexData::reset(VertexSpecification vertex_specification) {
    clear();

    vertex_specification_ = vertex_specification;
    recalc_attributes();
}

void VertexData::recalc_attributes() {

}

VertexAttribute VertexData::attribute_for_type(VertexAttributeType type) const {
    switch(type) {
        case VERTEX_ATTRIBUTE_TYPE_POSITION: return vertex_specification_.position_attribute;
        case VERTEX_ATTRIBUTE_TYPE_NORMAL: return vertex_specification_.normal_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD0: return vertex_specification_.texcoord0_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD1: return vertex_specification_.texcoord1_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD2: return vertex_specification_.texcoord2_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD3: return vertex_specification_.texcoord3_attribute;
        case VERTEX_ATTRIBUTE_TYPE_DIFFUSE: return vertex_specification_.diffuse_attribute;
        case VERTEX_ATTRIBUTE_TYPE_SPECULAR: return vertex_specification_.specular_attribute;
    default:
        throw std::logic_error("Invalid vertex attribute type");
    }
}

void VertexData::done() {
    signal_update_complete_();
}

IndexData::IndexData() {

}

void IndexData::reset() {
    clear();
}

void IndexData::done() {
    signal_update_complete_();
}

}
