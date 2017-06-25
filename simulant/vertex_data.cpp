//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdexcept>
#include "vertex_data.h"
#include "window_base.h"
#include "utils/gl_thread_check.h"

namespace smlt {

const VertexSpecification VertexSpecification::DEFAULT = {
    VERTEX_ATTRIBUTE_3F,
    VERTEX_ATTRIBUTE_3F,
    VERTEX_ATTRIBUTE_2F,
    VERTEX_ATTRIBUTE_2F,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
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
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_4F
};

VertexAttribute attribute_for_type(VertexAttributeType type, const VertexSpecification& spec) {
    switch(type) {
        case VERTEX_ATTRIBUTE_TYPE_POSITION: return spec.position_attribute;
        case VERTEX_ATTRIBUTE_TYPE_NORMAL: return spec.normal_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD0: return spec.texcoord0_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD1: return spec.texcoord1_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD2: return spec.texcoord2_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD3: return spec.texcoord3_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD4: return spec.texcoord4_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD5: return spec.texcoord5_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD6: return spec.texcoord6_attribute;
        case VERTEX_ATTRIBUTE_TYPE_TEXCOORD7: return spec.texcoord7_attribute;
        case VERTEX_ATTRIBUTE_TYPE_DIFFUSE: return spec.diffuse_attribute;
        case VERTEX_ATTRIBUTE_TYPE_SPECULAR: return spec.specular_attribute;
    default:
        throw std::logic_error("Invalid vertex attribute type");
    }
}

VertexData::VertexData(VertexSpecification vertex_specification):
    cursor_position_(0) {

    reset(vertex_specification);
}

void VertexData::clear() {
    data_.clear();
    data_.shrink_to_fit();
    cursor_position_ = 0;    
}

void VertexData::position_checks() {
    if(!specification().has_positions()) {
        throw std::logic_error("Vertex data has no position attribute");
    }

    if(cursor_position_ == (int32_t) data_.size()) {
        push_back();
    } else if(cursor_position_ > (int32_t) data_.size()) {
        throw std::out_of_range("Cursor moved out of range");
    }
}

void VertexData::position(float x, float y, float z, float w) {
    position_checks();

    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_4F);
    Vec4* out = (Vec4*) &data_[cursor_position_];
    *out = Vec4(x, y, z, w);
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

void VertexData::position(const Vec2 &pos) {
    position(pos.x, pos.y);
}

void VertexData::position(const Vec3& pos) {
    position(pos.x, pos.y, pos.z);
}

void VertexData::position(const Vec4 &pos) {
    position(pos.x, pos.y, pos.z, pos.w);
}

template<>
Vec2 VertexData::position_at<Vec2>(uint32_t idx) const {
    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_2F);
    Vec2 out = *((Vec2*) &data_[idx * stride()]);
    return out;
}

template<>
Vec3 VertexData::position_at<Vec3>(uint32_t idx) const {
    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_3F);
    Vec3 out = *((Vec3*) &data_[idx * stride()]);
    return out;
}

template<>
Vec4 VertexData::position_at<Vec4>(uint32_t idx) const {
    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_4F);
    Vec4 out = *((Vec4*) &data_[idx * stride()]);
    return out;
}

void VertexData::normal(float x, float y, float z) {
    assert(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_3F);
    Vec3* out = (Vec3*) &data_[cursor_position_ + specification().normal_offset()];
    *out = Vec3(x, y, z);
}

void VertexData::normal(const Vec3 &n) {
    normal(n.x, n.y, n.z);
}

void VertexData::tex_coordX(uint8_t which, float u, float v) {
    uint32_t offset = specification().texcoordX_offset(which);
    Vec2* out = (Vec2*) &data_[cursor_position_ + offset];
    *out = Vec2(u, v);
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w) {
    uint32_t offset = specification().texcoordX_offset(which);
    Vec3* out = (Vec3*) &data_[cursor_position_ + offset];
    *out = Vec3(u, v, w);
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w, float x) {
    uint32_t offset = specification().texcoordX_offset(which);
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
    Vec2 out = *((Vec2*) &data_[(idx * stride()) + specification().texcoord0_offset()]);
    return out;
}

template<>
Vec3 VertexData::texcoord0_at<Vec3>(uint32_t idx) {
    assert(vertex_specification_.texcoord0_attribute == VERTEX_ATTRIBUTE_3F);
    Vec3 out = *((Vec3*) &data_[(idx * stride()) + specification().texcoord0_offset()]);
    return out;
}

template<>
Vec4 VertexData::texcoord0_at<Vec4>(uint32_t idx) {
    assert(vertex_specification_.texcoord0_attribute == VERTEX_ATTRIBUTE_4F);
    Vec4 out = *((Vec4*) &data_[(idx * stride()) + specification().texcoord0_offset()]);
    return out;
}

template<>
Vec2 VertexData::texcoord1_at<Vec2>(uint32_t idx) const {
    assert(vertex_specification_.texcoord1_attribute == VERTEX_ATTRIBUTE_2F);
    Vec2 out = *((Vec2*) &data_[(idx * stride()) + specification().texcoord1_offset()]);
    return out;
}

template<>
Vec3 VertexData::texcoord1_at<Vec3>(uint32_t idx) const {
    assert(vertex_specification_.texcoord1_attribute == VERTEX_ATTRIBUTE_3F);
    Vec3 out = *((Vec3*) &data_[(idx * stride()) + specification().texcoord1_offset()]);
    return out;
}

template<>
Vec4 VertexData::texcoord1_at<Vec4>(uint32_t idx) const {
    assert(vertex_specification_.texcoord1_attribute == VERTEX_ATTRIBUTE_4F);
    Vec4 out = *((Vec4*) &data_[(idx * stride()) + specification().texcoord1_offset()]);
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
    Vec4* out = (Vec4*) &data_[cursor_position_ + specification().diffuse_offset()];
    *out = Vec4(r, g, b, a);
}

void VertexData::diffuse(const Colour& colour) {
    diffuse(colour.r, colour.g, colour.b, colour.a);
}

void VertexData::move_to_start() {
    move_to(0);
}

void VertexData::move_to_end() {
    move_to(data_.size() / stride());
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
    int32_t new_pos = cursor_position_ + stride();

    //cursor_position_ == data_.size() is allowed (see position())
    if(new_pos > (int32_t) data_.size()) {
        throw std::out_of_range("Cursor moved out of range");
    }

    cursor_position_ = new_pos;
    return cursor_position_;
}

void VertexData::reset(VertexSpecification vertex_specification) {
    clear();

    vertex_specification.recalc_stride();
    vertex_specification_ = vertex_specification;
    stride_ = vertex_specification.stride();
    recalc_attributes();
}

void VertexData::recalc_attributes() {

}

void VertexData::interp_vertex(uint32_t source_idx, const VertexData &dest_state, uint32_t dest_idx, VertexData &out, uint32_t out_idx, float interp) {
    /*
     * Given a VertexData representing the destination state, this will interpolate
     * the vertex position and normal into the out data of the specified index
     */

    if(out.vertex_specification_ != this->vertex_specification_ || dest_state.vertex_specification_ != this->vertex_specification_) {
        throw std::logic_error("You cannot interpolate vertices between data with different specifications");
    }

    // First, copy all the data from the source to the current out vertex
    for(uint32_t i = 0; i < stride(); ++i) {
        out.data_[(out_idx * stride()) + i] = data_[(source_idx * stride()) + i];
    }

    out.move_to(out_idx);

    switch(vertex_specification_.position_attribute) {
        case VERTEX_ATTRIBUTE_2F: {
            Vec2 source = this->position_at<Vec2>(source_idx);
            Vec2 dest = dest_state.position_at<Vec2>(dest_idx);
            Vec2 final = source + ((dest - source) * interp);
            out.position(final);
        }
        break;
        case VERTEX_ATTRIBUTE_3F: {
            Vec3 source = this->position_at<Vec3>(source_idx);
            Vec3 dest = dest_state.position_at<Vec3>(dest_idx);
            Vec3 final = source + ((dest - source) * interp);
            out.position(final);
        }
        break;
        case VERTEX_ATTRIBUTE_4F: {
            Vec4 source = this->position_at<Vec4>(source_idx);
            Vec4 dest = dest_state.position_at<Vec4>(dest_idx);
            Vec4 final = source + ((dest - source) * interp);
            out.position(final);
        }
        break;
        default:
            L_WARN("Ignoring unsupported vertex position type");
    }

    //FIXME: Interpolate normals here
}

void VertexData::done() {
    signal_update_complete_();
}

IndexData::IndexData(IndexType type):
    index_type_(type) {

}

void IndexData::each(std::function<void (uint32_t)> cb) {
    auto st = stride();
    for(std::size_t i = 0; i < indices_.size(); i += st) {
        uint32_t v;
        switch(index_type_) {
        case INDEX_TYPE_8_BIT:
            v = indices_[i];
            break;
        case INDEX_TYPE_16_BIT:
            v = *((uint16_t*)&indices_[i]);
            break;
        case INDEX_TYPE_32_BIT:
            v = *((uint32_t*)&indices_[i]);
            break;
        }

        cb(v);
    }
}

void IndexData::reset() {
    clear();
}

std::vector<uint32_t> IndexData::all() {
    std::vector<uint32_t> ret;

    each([&ret](uint32_t v) {
        ret.push_back(v);
    });

    return ret;
}

void IndexData::done() {
    signal_update_complete_();
}

}
