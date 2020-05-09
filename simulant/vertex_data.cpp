//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdexcept>
#include "vertex_data.h"
#include "window.h"
#include "utils/gl_thread_check.h"

namespace smlt {

// Adapted from here: https://github.com/mesa3d/mesa/blob/a5f618a291e67e74c56df235d45c3eb967ebb41f/src/mesa/main/image.c
_S_FORCE_INLINE uint32_t pack_vertex_attribute_vec3_1i(float x, float y, float z) {
    const float w = 0.0f;

    const uint32_t xs = x < 0;
    const uint32_t ys = y < 0;
    const uint32_t zs = z < 0;
    const uint32_t ws = w < 0;

    uint32_t vi =
        ws << 31 | ((uint32_t)(w + (ws << 1)) & 1) << 30 |
        zs << 29 | ((uint32_t)(z * 511 + (zs << 9)) & 511) << 20 |
        ys << 19 | ((uint32_t)(y * 511 + (ys << 9)) & 511) << 10 |
        xs << 9  | ((uint32_t)(x * 511 + (xs << 9)) & 511);

    return vi;
}

_S_FORCE_INLINE Vec3 unpack_vertex_attribute_vec3_1i(uint32_t p) {
    auto unpack = [](int i) -> float {
        struct attr_bits_10 {
            signed int x:10;
        };

        struct attr_bits_10 val;
        val.x = i;
        return (2.0F * (float)val.x + 1.0F) * (1.0F / 1023.0F);
    };

    Vec3 ret;
    ret.x = unpack(p & 0x3ff);
    ret.y = unpack((p >> 10) & 0x3ff);
    ret.z = unpack((p >> 20) & 0x3ff);

    return ret;
}

const VertexSpecification VertexSpecification::DEFAULT = VertexSpecification{
    VERTEX_ATTRIBUTE_3F,  // Position
#ifdef _arch_dreamcast
    /* We enable this only on the Dreamcast as Mesa3D suffers a bug
     * on Linux. But it's on the DC that this matters anyway */
    VERTEX_ATTRIBUTE_PACKED_VEC4_1I, // Normal
#else
    VERTEX_ATTRIBUTE_3F,
#endif
    VERTEX_ATTRIBUTE_2F, // UV
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_NONE,
    VERTEX_ATTRIBUTE_4UB, // Diffuse
    VERTEX_ATTRIBUTE_NONE
};

const VertexSpecification VertexSpecification::POSITION_ONLY = VertexSpecification{
    VERTEX_ATTRIBUTE_3F
};

const VertexSpecification VertexSpecification::POSITION_AND_DIFFUSE = VertexSpecification{
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
    VERTEX_ATTRIBUTE_4UB
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

VertexData::~VertexData() {
    clear();
}

void VertexData::clear(bool release_memory) {
    data_.clear();

    if(release_memory) {
        data_.shrink_to_fit();
    }

    cursor_position_ = 0;
    vertex_count_ = 0;
}

void VertexData::position_checks() {
    if(!vertex_specification_.has_positions()) {
        throw std::logic_error("Vertex data has no position attribute");
    }

    if(cursor_position_ == (int32_t) vertex_count_) {
        push_back();
    } else if(cursor_position_ > (int32_t) vertex_count_) {
        throw std::out_of_range("Cursor moved out of range");
    }
}

void VertexData::position(float x, float y, float z, float w) {
    position_checks();

    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_4F);
    Vec4* out = (Vec4*) &data_[cursor_offset()];
    *out = Vec4(x, y, z, w);
}

void VertexData::position(float x, float y, float z) {
    position_checks();

    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_3F);
    Vec3* out = (Vec3*) &data_[cursor_offset()];
    *out = Vec3(x, y, z);
}

void VertexData::position(float x, float y) {
    position_checks();

    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_2F);
    Vec2* out = (Vec2*) &data_[cursor_offset()];
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
const Vec2* VertexData::position_at<Vec2>(uint32_t idx) const {
    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_2F);
    return ((Vec2*) &data_[idx * stride_]);
}

template<>
const Vec3* VertexData::position_at<Vec3>(uint32_t idx) const {
    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_3F);
    return ((Vec3*) &data_[idx * stride_]);
}

template<>
const Vec4* VertexData::position_at<Vec4>(uint32_t idx) const {
    assert(vertex_specification_.position_attribute == VERTEX_ATTRIBUTE_4F);
    return ((Vec4*) &data_[idx * stride_]);
}

template<>
const Vec2* VertexData::normal_at<Vec2>(uint32_t idx) const {
    assert(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_2F);
    return ((Vec2*) &data_[(idx * stride_) + vertex_specification_.normal_offset()]);
}

template<>
const Vec3* VertexData::normal_at<Vec3>(uint32_t idx) const {
    /* Warning, pointer may not be unique! */

    if(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_3F) {
        return ((Vec3*) &data_[(idx * stride()) + vertex_specification_.normal_offset()]);
    } else {
        assert(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_PACKED_VEC4_1I);

        static Vec3 ret; // This is mega nasty, need to rethink the pointer return
        auto data = (uint32_t*) &data_[(idx * stride()) + vertex_specification_.normal_offset()];
        ret = unpack_vertex_attribute_vec3_1i(*data);
        return &ret;
    }
}

Vec4 VertexData::position_nd_at(uint32_t idx, float defz, float defw) const {
    const auto& attr = vertex_specification_.position_attribute;
    if(attr == VERTEX_ATTRIBUTE_2F) {
        auto v = *position_at<Vec2>(idx);
        return Vec4(v.x, v.y, defz, defw);
    } else if(attr == VERTEX_ATTRIBUTE_3F) {
        auto v = *position_at<Vec3>(idx);
        return Vec4(v.x, v.y, v.z, defw);
    } else {
        return *position_at<Vec4>(idx);
    }
}

void VertexData::normal(float x, float y, float z) {
    auto offset = vertex_specification_.normal_offset();

    if(offset == INVALID_ATTRIBUTE_OFFSET) {
        return;
    }

    uint8_t* ptr = (uint8_t*) &data_[cursor_offset() + offset];

    if(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_3F) {
        Vec3* out = (Vec3*) ptr;
        *out = Vec3(x, y, z);
    } else  {
        assert(vertex_specification_.normal_attribute == VERTEX_ATTRIBUTE_PACKED_VEC4_1I);
        uint32_t* packed = (uint32_t*) ptr;
        *packed = pack_vertex_attribute_vec3_1i(x, y, z);
    }
}

void VertexData::normal(const Vec3 &n) {
    normal(n.x, n.y, n.z);
}

void VertexData::tex_coordX(uint8_t which, float u, float v) {
    auto offset = vertex_specification_.texcoordX_offset(which);

    if(offset == INVALID_ATTRIBUTE_OFFSET) {
        return;
    }

    Vec2* out = (Vec2*) &data_[cursor_offset() + offset];
    *out = Vec2(u, v);
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w) {
    auto offset = vertex_specification_.texcoordX_offset(which);

    if(offset == INVALID_ATTRIBUTE_OFFSET) {
        return;
    }

    Vec3* out = (Vec3*) &data_[cursor_offset() + offset];
    *out = Vec3(u, v, w);
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w, float x) {
    auto offset = vertex_specification_.texcoordX_offset(which);

    if(offset == INVALID_ATTRIBUTE_OFFSET) {
        return;
    }

    Vec4* out = (Vec4*) &data_[cursor_offset() + offset];
    *out = Vec4(u, v, w, x);
}

void VertexData::push_back() {
    vertex_count_++;
    data_.resize(data_.size() + stride_, 0);
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
const Vec2* VertexData::texcoord0_at<Vec2>(uint32_t idx) const {
    assert(vertex_specification_.texcoord0_attribute == VERTEX_ATTRIBUTE_2F);
    return ((Vec2*) &data_[(idx * stride()) + vertex_specification_.texcoord0_offset()]);
}

template<>
const Vec3* VertexData::texcoord0_at<Vec3>(uint32_t idx) const {
    assert(vertex_specification_.texcoord0_attribute == VERTEX_ATTRIBUTE_3F);
    return ((Vec3*) &data_[(idx * stride()) + vertex_specification_.texcoord0_offset()]);
}

template<>
const Vec4* VertexData::texcoord0_at<Vec4>(uint32_t idx) const {
    assert(vertex_specification_.texcoord0_attribute == VERTEX_ATTRIBUTE_4F);
    return ((Vec4*) &data_[(idx * stride()) + vertex_specification_.texcoord0_offset()]);
}

template<>
const Vec2* VertexData::texcoord1_at<Vec2>(uint32_t idx) const {
    assert(vertex_specification_.texcoord1_attribute == VERTEX_ATTRIBUTE_2F);
    return ((Vec2*) &data_[(idx * stride()) + vertex_specification_.texcoord1_offset()]);
}

template<>
const Vec3* VertexData::texcoord1_at<Vec3>(uint32_t idx) const {
    assert(vertex_specification_.texcoord1_attribute == VERTEX_ATTRIBUTE_3F);
    return ((Vec3*) &data_[(idx * stride()) + vertex_specification_.texcoord1_offset()]);
}

template<>
const Vec4* VertexData::texcoord1_at<Vec4>(uint32_t idx) const {
    assert(vertex_specification_.texcoord1_attribute == VERTEX_ATTRIBUTE_4F);
    return ((Vec4*) &data_[(idx * stride()) + vertex_specification_.texcoord1_offset()]);
}

template<>
const Colour* VertexData::diffuse_at(const uint32_t idx) const {
    assert(vertex_specification_.diffuse_attribute == VERTEX_ATTRIBUTE_4F);
    return ((Colour*) &data_[(idx * stride()) + vertex_specification_.diffuse_offset()]);
}

template<>
const uint8_t* VertexData::diffuse_at(const uint32_t idx) const {
    assert(vertex_specification_.diffuse_attribute == VERTEX_ATTRIBUTE_4UB);
    return ((uint8_t*) &data_[(idx * stride()) + vertex_specification_.diffuse_offset()]);
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

void VertexData::diffuse(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    assert(vertex_specification_.diffuse_attribute == VERTEX_ATTRIBUTE_4UB);
    auto offset = vertex_specification_.diffuse_offset();

    if(offset == INVALID_ATTRIBUTE_OFFSET) {
        return;
    }


    uint8_t* out = (uint8_t*) &data_[cursor_offset() + offset];

    /* We store unsigned bytes in bgra format internally as this is faster
     * on some platforms (e.g. Dreamcast) */
    out[0] = b;
    out[1] = g;
    out[2] = r;
    out[3] = a;
}

void VertexData::diffuse(float r, float g, float b, float a) {
    assert(vertex_specification_.diffuse_attribute == VERTEX_ATTRIBUTE_4F);

    auto offset = vertex_specification_.diffuse_offset();

    if(offset == INVALID_ATTRIBUTE_OFFSET) {
        return;
    }

    Vec4* out = (Vec4*) &data_[cursor_offset() + offset];
    *out = Vec4(r, g, b, a);
}

void VertexData::diffuse(const Colour& colour) {
    if(vertex_specification_.diffuse_attribute == VERTEX_ATTRIBUTE_4F) {
        diffuse(colour.r, colour.g, colour.b, colour.a);
    } else {
        const float s = 255.0f;
        diffuse(
            (uint8_t) clamp(colour.r * s, 0, 255),
            (uint8_t) clamp(colour.g * s, 0, 255),
            (uint8_t) clamp(colour.b * s, 0, 255),
            (uint8_t) clamp(colour.a * s, 0, 255)
        );
    }
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
    if(index > (int32_t) vertex_count_) {
        throw std::out_of_range("Tried to move outside the range of the data");
    }

    cursor_position_ = index;
}

uint32_t VertexData::move_next() {
    move_to(cursor_position_ + 1);
    return cursor_position_;
}

void VertexData::reset(VertexSpecification vertex_specification) {
    clear();

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
            auto source = this->position_at<Vec2>(source_idx);
            auto dest = dest_state.position_at<Vec2>(dest_idx);
            Vec2 final = *source + ((*dest - *source) * interp);
            out.position(final);
        }
        break;
        case VERTEX_ATTRIBUTE_3F: {
            auto source = this->position_at<Vec3>(source_idx);
            auto dest = dest_state.position_at<Vec3>(dest_idx);
            Vec3 final = *source + ((*dest - *source) * interp);
            out.position(final);
        }
        break;
        case VERTEX_ATTRIBUTE_4F: {
            auto source = this->position_at<Vec4>(source_idx);
            auto dest = dest_state.position_at<Vec4>(dest_idx);
            Vec4 final = *source + ((*dest - *source) * interp);
            out.position(final);
        }
        break;
        default:
            L_WARN("Ignoring unsupported vertex position type");
    }

    //FIXME: Interpolate normals here
}

void VertexData::reserve(uint32_t size) {
    data_.reserve(size * stride());
}

void VertexData::resize(uint32_t size) {
    data_.resize(size * stride(), 0);
    vertex_count_ = size;
}

void VertexData::done() {
    signal_update_complete_();
    last_updated_ = TimeKeeper::now_in_us();
}

uint64_t VertexData::last_updated() const {
    return last_updated_;
}

bool VertexData::clone_into(VertexData& other) {
    if(vertex_specification_ != other.vertex_specification_) {
        return false;
    }

    other.data_ = this->data_;
    other.vertex_count_ = this->vertex_count_;
    other.stride_ = this->stride_;
    other.cursor_position_ = 0;

    return true;
}

static constexpr uint32_t calc_index_stride(IndexType type) {
    return (type == INDEX_TYPE_16_BIT) ? sizeof(uint16_t) : (type == INDEX_TYPE_8_BIT) ? sizeof(uint8_t) : sizeof(uint32_t);
}

IndexData::IndexData(IndexType type):
    index_type_(type),
    stride_(calc_index_stride(type)){

}

void IndexData::reset() {
    clear();
}

void IndexData::clear(bool release_memory) {
    indices_.clear();
    if(release_memory) {
        indices_.shrink_to_fit();
    }
    count_ = 0;
}

void IndexData::resize(uint32_t size) {
    indices_.resize(size * stride(), 0);
    count_ = size;
}

std::vector<uint32_t> IndexData::all() {
    std::vector<uint32_t> ret;

    ret.reserve(count());
    for(auto idx: *this) {
        ret.push_back(idx);
    }

    return ret;
}

void IndexData::done() {
    signal_update_complete_();
    last_updated_ = TimeKeeper::now_in_us();
}

uint64_t IndexData::last_updated() const {
    return last_updated_;
}

IndexDataIterator::IndexDataIterator(const IndexData *owner, int pos):
    owner_(owner),
    type_(owner->index_type()),
    stride_(owner->stride()) {

    ptr_ = (&owner_->indices_[0]) + (pos * stride_);
}

}
