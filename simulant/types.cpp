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

#include "types.h"
#include "utils/random.h"

namespace smlt {

const std::vector<RenderPriority> RENDER_PRIORITIES = {
    RENDER_PRIORITY_ABSOLUTE_BACKGROUND,
    RENDER_PRIORITY_BACKGROUND,
    RENDER_PRIORITY_DISTANT,
    RENDER_PRIORITY_MAIN,
    RENDER_PRIORITY_NEAR,
    RENDER_PRIORITY_FOREGROUND,
    RENDER_PRIORITY_ABSOLUTE_FOREGROUND
};

VertexFormat::VertexFormat(VertexAttribute position, VertexAttribute normal, VertexAttribute texcoord0,
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

bool VertexFormat::has_texcoordX(uint8_t which) const {
    assert(which < 8);

    return (which == 0) ? has_texcoord0() :
           (which == 1) ? has_texcoord1() :
           (which == 2) ? has_texcoord2() :
           (which == 3) ? has_texcoord3() :
           (which == 4) ? has_texcoord4() :
           (which == 5) ? has_texcoord5() :
           (which == 6) ? has_texcoord6() :
           (which == 7) ? has_texcoord7() : false;
}

VertexAttribute VertexFormat::texcoordX_attribute(uint8_t which) const {
    assert(which < 8);

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

void VertexFormat::recalc_stride_and_offsets() {
    /* The order here is designed to match the Dreamcast PVR vertex (at least at the start)
     * on the Dreamcast we add 4 bytes of padding at the end of the vertex
     * so that the default vertex arrangement is 32 bytes. */

    texcoord0_offset_ = position_offset_ + vertex_attribute_size(position_attribute_);
    diffuse_offset_ = texcoord0_offset_ + vertex_attribute_size(texcoord0_attribute_);

    normal_offset_ = diffuse_offset_ + vertex_attribute_size(diffuse_attribute_);
    texcoord1_offset_ = normal_offset_ + vertex_attribute_size(normal_attribute_);
    texcoord2_offset_ = texcoord1_offset_ + vertex_attribute_size(texcoord1_attribute_);
    texcoord3_offset_ = texcoord2_offset_ + vertex_attribute_size(texcoord2_attribute_);
    texcoord4_offset_ = texcoord3_offset_ + vertex_attribute_size(texcoord3_attribute_);
    texcoord5_offset_ = texcoord4_offset_ + vertex_attribute_size(texcoord4_attribute_);
    texcoord6_offset_ = texcoord5_offset_ + vertex_attribute_size(texcoord5_attribute_);
    texcoord7_offset_ = texcoord6_offset_ + vertex_attribute_size(texcoord6_attribute_);
    specular_offset_ = texcoord7_offset_ + vertex_attribute_size(texcoord7_attribute_);

    // On the Dreamcast with the default vertex arrangement, this should be 32 bytes
    stride_ = round_to_bytes(specular_offset_ + vertex_attribute_size(specular_attribute_), BUFFER_STRIDE_ALIGNMENT);
}

AttributeOffset VertexFormat::position_offset(bool check) const {
    if(check && !has_positions()) { return INVALID_ATTRIBUTE_OFFSET; }
    return 0;
}

AttributeOffset VertexFormat::normal_offset(bool check) const {
    if(check && !has_normals()) { return INVALID_ATTRIBUTE_OFFSET; }
    return normal_offset_;
}

AttributeOffset VertexFormat::texcoord0_offset(bool check) const {
    if(check && !has_texcoord0()) { return INVALID_ATTRIBUTE_OFFSET; }
    return texcoord0_offset_;
}

AttributeOffset VertexFormat::texcoord1_offset(bool check) const {
    if(check && !has_texcoord1()) { return INVALID_ATTRIBUTE_OFFSET; }
    return texcoord1_offset_;
}

AttributeOffset VertexFormat::texcoord2_offset(bool check) const {
    if(check && !has_texcoord2()) { return INVALID_ATTRIBUTE_OFFSET; }
    return texcoord2_offset_;
}

AttributeOffset VertexFormat::texcoord3_offset(bool check) const {
    if(check && !has_texcoord3()) { return INVALID_ATTRIBUTE_OFFSET; }
    return texcoord3_offset_;
}

AttributeOffset VertexFormat::texcoord4_offset(bool check) const {
    if(check && !has_texcoord4()) { return INVALID_ATTRIBUTE_OFFSET; }
    return texcoord4_offset_;
}

AttributeOffset VertexFormat::texcoord5_offset(bool check) const {
    if(check && !has_texcoord5()) { return INVALID_ATTRIBUTE_OFFSET; }
    return texcoord5_offset_;
}

AttributeOffset VertexFormat::texcoord6_offset(bool check) const {
    if(check && !has_texcoord6()) { return INVALID_ATTRIBUTE_OFFSET; }
    return texcoord6_offset_;
}

AttributeOffset VertexFormat::texcoord7_offset(bool check) const {
    if(check && !has_texcoord7()) { return INVALID_ATTRIBUTE_OFFSET; }
    return texcoord7_offset_;
}

AttributeOffset VertexFormat::texcoordX_offset(uint8_t which, bool check) const {
    assert(which < 8);

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

AttributeOffset VertexFormat::diffuse_offset(bool check) const {
    if(check && !has_diffuse()) { return INVALID_ATTRIBUTE_OFFSET; }
    return diffuse_offset_;
}

AttributeOffset VertexFormat::specular_offset(bool check) const {
    if(check && !has_specular()) { return INVALID_ATTRIBUTE_OFFSET; }
    return specular_offset_;
}

VertexAttributeProperty::VertexAttributeProperty(VertexFormat *spec, VertexAttribute VertexFormat::*attr):
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
