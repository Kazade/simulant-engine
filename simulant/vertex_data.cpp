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
#include "time_keeper.h"
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

VertexData::VertexData(VertexFormat vertex_specification):
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
    if(!vertex_specification_.attr_count(VERTEX_ATTR_NAME_POSITION)) {
        return;
    }

    if(cursor_position_ > (int32_t) vertex_count_) {
        assert(0 && "Cursor has moved out of range");
        cursor_position_ = (int32_t) vertex_count_;
    }

    if(cursor_position_ == (int32_t) vertex_count_) {
        push_back();
    }
}

void VertexData::position(float x, float y, float z, float w) {
    position_checks();

    assert(vertex_specification_.attr(VERTEX_ATTR_NAME_POSITION)
               ->component_count() == 4);
    assert(vertex_specification_.attr(VERTEX_ATTR_NAME_POSITION)->type ==
           VERTEX_ATTR_TYPE_FLOAT);

    float* out = (float*)&data_[cursor_offset()];
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = w;
}

void VertexData::position(float x, float y, float z) {
    position_checks();

    assert(vertex_specification_.attr(VERTEX_ATTR_NAME_POSITION)
               ->component_count() == 3);
    assert(vertex_specification_.attr(VERTEX_ATTR_NAME_POSITION)->type ==
           VERTEX_ATTR_TYPE_FLOAT);

    float* out = (float*)&data_[cursor_offset()];

    out[0] = x;
    out[1] = y;
    out[2] = z;
}

void VertexData::position(float x, float y) {
    position_checks();

    assert(vertex_specification_.attr(VERTEX_ATTR_NAME_POSITION)
               ->component_count() == 2);
    assert(vertex_specification_.attr(VERTEX_ATTR_NAME_POSITION)->type ==
           VERTEX_ATTR_TYPE_FLOAT);

    float* out = (float*)&data_[cursor_offset()];

    out[0] = x;
    out[1] = y;
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

void VertexData::normal(float x, float y, float z) {
    auto offset =
        (int)vertex_specification_.offset(VERTEX_ATTR_NAME_NORMAL).value_or(-1);

    if(offset == -1) {
        return;
    }

    uint8_t* ptr = (uint8_t*) &data_[cursor_offset() + offset];

    assert(vertex_specification_.attr(VERTEX_ATTR_NAME_NORMAL)
               ->component_count() == 3);

    assert(vertex_specification_.attr(VERTEX_ATTR_NAME_NORMAL)->type ==
           VERTEX_ATTR_TYPE_FLOAT);

    Vec3* out = (Vec3*)ptr;
    *out = Vec3(x, y, z);
}

void VertexData::normal(const Vec3 &n) {
    normal(n.x, n.y, n.z);
}

void VertexData::tex_coordX(uint8_t which, float u, float v) {
    auto offset =
        (int)vertex_specification_
            .offset(
                (VertexAttributeName)((int)VERTEX_ATTR_NAME_TEXCOORD_0 + which))
            .value_or(-1);

    if(offset == -1) {
        return;
    }

    Vec2* out = (Vec2*) &data_[cursor_offset() + offset];
    out->x = u;
    out->y = v;
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w) {
    auto offset =
        (int)vertex_specification_
            .offset(
                (VertexAttributeName)((int)VERTEX_ATTR_NAME_TEXCOORD_0 + which))
            .value_or(-1);

    if(offset == -1) {
        return;
    }

    Vec3* out = (Vec3*)&data_[cursor_offset() + offset];
    out->x = u;
    out->y = v;
    out->z = w;
}

void VertexData::tex_coordX(uint8_t which, float u, float v, float w, float x) {
    auto offset =
        (int)vertex_specification_
            .offset(
                (VertexAttributeName)((int)VERTEX_ATTR_NAME_TEXCOORD_0 + which))
            .value_or(-1);

    if(offset == -1) {
        return;
    }

    Vec4* out = (Vec4*)&data_[cursor_offset() + offset];
    out->x = u;
    out->y = v;
    out->z = w;
    out->w = x;
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

void VertexData::color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    auto offset =
        (int)vertex_specification_.offset(VERTEX_ATTR_NAME_COLOR).value_or(-1);

    if(offset == -1) {
        return;
    }

    uint8_t* out = (uint8_t*) &data_[cursor_offset() + offset];

    if(vertex_specification_.attr(VERTEX_ATTR_NAME_COLOR).value().arrangement ==
       VERTEX_ATTR_ARRANGEMENT_BGRA) {
        out[0] = b;
        out[1] = g;
        out[2] = r;
        out[3] = a;
    } else {
        out[0] = r;
        out[1] = g;
        out[2] = b;
        out[3] = a;
    }
}

void VertexData::color(float r, float g, float b, float a) {
    auto offset =
        (int)vertex_specification_.offset(VERTEX_ATTR_NAME_COLOR).value_or(-1);

    if(offset == -1) {
        return;
    }

    Vec4* out = (Vec4*) &data_[cursor_offset() + offset];
    *out = Vec4(r, g, b, a);
}

void VertexData::color(const Color& c) {
    if(vertex_specification_.attr(VERTEX_ATTR_NAME_COLOR)->type ==
       VERTEX_ATTR_TYPE_FLOAT) {
        color(c.r, c.g, c.b, c.a);
    } else {
        const float s = 255.0f;
        color((uint8_t)clamp(c.r * s, 0, 255), (uint8_t)clamp(c.g * s, 0, 255),
              (uint8_t)clamp(c.b * s, 0, 255), (uint8_t)clamp(c.a * s, 0, 255));
    }
}

void VertexData::move_to_start() {
    move_to(0);
}

void VertexData::move_to_end() {
    move_to(data_.size() / stride());
}

bool VertexData::move_by(int32_t amount) {
    if(int32_t(cursor_position_) + amount >= (int32_t) vertex_count_) {
        return false;
    }

    cursor_position_ += amount;
    return false;
}

bool VertexData::move_to(int32_t index) {
    if(index > (int32_t) vertex_count_) {
        return false;
    }

    cursor_position_ = index;
    return true;
}

uint32_t VertexData::move_next() {
    move_to(cursor_position_ + 1);
    return cursor_position_;
}

void VertexData::reset(VertexFormat vertex_specification) {
    clear();

    vertex_specification_ = vertex_specification;
    stride_ = vertex_specification.stride();
    recalc_attributes();
}

void VertexData::recalc_attributes() {

}

bool VertexData::interp_vertex(uint32_t source_idx, const VertexData &dest_state, uint32_t dest_idx, VertexData &out, uint32_t out_idx, float interp) {
    /*
     * Given a VertexData representing the destination state, this will interpolate
     * the vertex position and normal into the out data of the specified index
     */

    if(out.vertex_specification_ != this->vertex_specification_ || dest_state.vertex_specification_ != this->vertex_specification_) {
        S_ERROR("You cannot interpolate vertices between data with different specifications");
        return false;
    }

    // First, copy all the data from the source to the current out vertex
    for(uint32_t i = 0; i < stride(); ++i) {
        out.data_[(out_idx * stride()) + i] = data_[(source_idx * stride()) + i];
    }

    out.move_to(out_idx);

    // FIXME: Using Vec4 introduces a per-vertex performance cost if the type
    // is actually Vec3, or worse Vec2. We should probably use the appropriate
    // type here.
    auto source =
        attr_as<Vec4>(VERTEX_ATTR_NAME_POSITION, source_idx).value_or(Vec4());
    auto dest =
        attr_as<Vec4>(VERTEX_ATTR_NAME_POSITION, dest_idx).value_or(Vec4());
    auto final = source + ((dest - source) * interp);

    auto ccount = vertex_specification_.attr(VERTEX_ATTR_NAME_POSITION)
                      .value()
                      .component_count();
    if(ccount == 4) {
        out.position(final.x, final.y, final.z, final.w);
    } else if(ccount == 3) {
        out.position(final.x, final.y, final.z);
    } else if(ccount == 2) {
        out.position(final.x, final.y);
    } else {
        return false;
    }

    //FIXME: Interpolate normals here
    return true;
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
    set_dirty(true);
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
    set_dirty(true);
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
