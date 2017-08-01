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

#include "types.h"
#include "utils/random.h"


namespace smlt {

constexpr uint32_t vertex_attribute_size(VertexAttribute attr) {
    return (attr == VERTEX_ATTRIBUTE_2F) ? sizeof(float) * 2 :
           (attr == VERTEX_ATTRIBUTE_3F) ? sizeof(float) * 3 :
           (attr == VERTEX_ATTRIBUTE_4F) ? sizeof(float) * 4 : 0;
}

VertexSpecification::VertexSpecification(VertexAttribute position, VertexAttribute normal, VertexAttribute texcoord0,
        VertexAttribute texcoord1, VertexAttribute texcoord2, VertexAttribute texcoord3, VertexAttribute texcoord4,
        VertexAttribute texcoord5, VertexAttribute texcoord6, VertexAttribute texcoord7,
        VertexAttribute diffuse, VertexAttribute specular):
    position_attribute_(position),
    normal_attribute_(normal),
    texcoord0_attribute_(texcoord0),
    texcoord1_attribute_(texcoord1),
    texcoord2_attribute_(texcoord2),
    texcoord3_attribute_(texcoord3),
    texcoord4_attribute_(texcoord4),
    texcoord5_attribute_(texcoord5),
    texcoord6_attribute_(texcoord6),
    texcoord7_attribute_(texcoord7),
    diffuse_attribute_(diffuse),
    specular_attribute_(specular) {

    recalc_stride_and_offsets();
}

bool VertexSpecification::has_texcoordX(uint8_t which) const {
    assert(which < MAX_TEXTURE_UNITS);

    return (which == 0) ? has_texcoord0() :
           (which == 1) ? has_texcoord1() :
           (which == 2) ? has_texcoord2() :
           (which == 3) ? has_texcoord3() :
           (which == 4) ? has_texcoord4() :
           (which == 5) ? has_texcoord5() :
           (which == 6) ? has_texcoord6() :
           (which == 7) ? has_texcoord7() : false;
}

const VertexAttribute VertexSpecification::texcoordX_attribute(uint8_t which) const {
    assert(which < MAX_TEXTURE_UNITS);

    switch(which) {
    case 0: return texcoord0_attribute_;
    case 1: return texcoord1_attribute_;
    case 2: return texcoord2_attribute_;
    case 3: return texcoord3_attribute_;
    case 4: return texcoord4_attribute_;
    case 5: return texcoord5_attribute_;
    case 6: return texcoord6_attribute_;
    case 7: return texcoord7_attribute_;
    default:
        return VERTEX_ATTRIBUTE_NONE;
    }
}

void VertexSpecification::recalc_stride_and_offsets() {
    normal_offset_ = vertex_attribute_size(position_attribute_);
    texcoord0_offset_ = normal_offset_ + vertex_attribute_size(normal_attribute_);
    texcoord1_offset_ = texcoord0_offset_ + vertex_attribute_size(texcoord0_attribute_);
    texcoord2_offset_ = texcoord1_offset_ + vertex_attribute_size(texcoord1_attribute_);
    texcoord3_offset_ = texcoord2_offset_ + vertex_attribute_size(texcoord2_attribute_);
    texcoord4_offset_ = texcoord3_offset_ + vertex_attribute_size(texcoord3_attribute_);
    texcoord5_offset_ = texcoord4_offset_ + vertex_attribute_size(texcoord4_attribute_);
    texcoord6_offset_ = texcoord5_offset_ + vertex_attribute_size(texcoord5_attribute_);
    texcoord7_offset_ = texcoord6_offset_ + vertex_attribute_size(texcoord6_attribute_);
    diffuse_offset_ = texcoord7_offset_ + vertex_attribute_size(texcoord7_attribute_);
    specular_offset_ = diffuse_offset_ + vertex_attribute_size(diffuse_attribute_);

    /* Most platforms work better when the data is aligned to 4 byte boundaries */
    auto round_to_four_bytes = [](uint16_t stride) -> uint16_t {
        auto remainder = stride % 4;
        if(remainder == 0) {
            return stride;
        } else {
            return stride + 4 - remainder;
        }
    };

    stride_ = round_to_four_bytes(
        vertex_attribute_size(position_attribute) +
        vertex_attribute_size(normal_attribute) +
        vertex_attribute_size(texcoord0_attribute) +
        vertex_attribute_size(texcoord1_attribute) +
        vertex_attribute_size(texcoord2_attribute) +
        vertex_attribute_size(texcoord3_attribute) +
        vertex_attribute_size(texcoord4_attribute) +
        vertex_attribute_size(texcoord5_attribute) +
        vertex_attribute_size(texcoord6_attribute) +
        vertex_attribute_size(texcoord7_attribute) +
        vertex_attribute_size(diffuse_attribute) +
        vertex_attribute_size(specular_attribute)
    );
}

uint16_t VertexSpecification::position_offset(bool check) const {
    if(check && !has_positions()) { throw std::logic_error("No such attribute"); }
    return 0;
}

uint16_t VertexSpecification::normal_offset(bool check) const {
    if(check && !has_normals()) { throw std::logic_error("No such attribute"); }
    return normal_offset_;
}

uint16_t VertexSpecification::texcoord0_offset(bool check) const {
    if(check && !has_texcoord0()) { throw std::logic_error("No such attribute"); }
    return texcoord0_offset_;
}

uint16_t VertexSpecification::texcoord1_offset(bool check) const {
    if(check && !has_texcoord1()) { throw std::logic_error("No such attribute"); }
    return texcoord1_offset_;
}

uint16_t VertexSpecification::texcoord2_offset(bool check) const {
    if(check && !has_texcoord2()) { throw std::logic_error("No such attribute"); }
    return texcoord2_offset_;
}

uint16_t VertexSpecification::texcoord3_offset(bool check) const {
    if(check && !has_texcoord3()) { throw std::logic_error("No such attribute"); }
    return texcoord3_offset_;
}

uint16_t VertexSpecification::texcoord4_offset(bool check) const {
    if(check && !has_texcoord4()) { throw std::logic_error("No such attribute"); }
    return texcoord4_offset_;
}

uint16_t VertexSpecification::texcoord5_offset(bool check) const {
    if(check && !has_texcoord5()) { throw std::logic_error("No such attribute"); }
    return texcoord5_offset_;
}

uint16_t VertexSpecification::texcoord6_offset(bool check) const {
    if(check && !has_texcoord6()) { throw std::logic_error("No such attribute"); }
    return texcoord6_offset_;
}

uint16_t VertexSpecification::texcoord7_offset(bool check) const {
    if(check && !has_texcoord7()) { throw std::logic_error("No such attribute"); }
    return texcoord7_offset_;
}

uint16_t VertexSpecification::texcoordX_offset(uint8_t which, bool check) const {
    assert(which < MAX_TEXTURE_UNITS);

    switch(which) {
    case 0: return texcoord0_offset(check);
    case 1: return texcoord1_offset(check);
    case 2: return texcoord2_offset(check);
    case 3: return texcoord3_offset(check);
    case 4: return texcoord4_offset(check);
    case 5: return texcoord5_offset(check);
    case 6: return texcoord6_offset(check);
    case 7: return texcoord7_offset(check);
    default:
        return 0;
    }
}

uint16_t VertexSpecification::diffuse_offset(bool check) const {
    if(check && !has_diffuse()) { throw std::logic_error("No such attribute"); }
    return diffuse_offset_;
}

uint16_t VertexSpecification::specular_offset(bool check) const {
    if(check && !has_specular()) { throw std::logic_error("No such attribute"); }
    return specular_offset_;
}

VertexAttributeProperty::VertexAttributeProperty(VertexSpecification *spec, VertexAttribute VertexSpecification::*attr):
    spec_(spec),
    attr_(attr) {

    assert(spec_);
}

VertexAttributeProperty &VertexAttributeProperty::operator=(const VertexAttribute &rhs) {
    assert(spec_);

    spec_->*attr_ = rhs;
    spec_->recalc_stride_and_offsets();
    return (*this);
}




}
