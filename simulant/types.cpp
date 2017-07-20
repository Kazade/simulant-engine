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

uint32_t vertex_attribute_size(VertexAttribute attr) {
    switch(attr) {
    case VERTEX_ATTRIBUTE_NONE: return 0;
    case VERTEX_ATTRIBUTE_2F: return sizeof(float) * 2;
    case VERTEX_ATTRIBUTE_3F:  return sizeof(float) * 3;
    case VERTEX_ATTRIBUTE_4F: return sizeof(float) * 4;
    default:
        return 0;
    }
}

VertexSpecification::VertexSpecification(VertexAttribute position, VertexAttribute normal, VertexAttribute texcoord0,
        VertexAttribute texcoord1, VertexAttribute texcoord2, VertexAttribute texcoord3, VertexAttribute texcoord4,
        VertexAttribute texcoord5, VertexAttribute texcoord6, VertexAttribute texcoord7,
        VertexAttribute diffuse, VertexAttribute specular):
    position_attribute(position),
    normal_attribute(normal),
    texcoord0_attribute(texcoord0),
    texcoord1_attribute(texcoord1),
    texcoord2_attribute(texcoord2),
    texcoord3_attribute(texcoord3),
    texcoord4_attribute(texcoord4),
    texcoord5_attribute(texcoord5),
    texcoord6_attribute(texcoord6),
    texcoord7_attribute(texcoord7),
    diffuse_attribute(diffuse),
    specular_attribute(specular) {

    recalc_stride();
}

bool VertexSpecification::has_texcoordX(uint8_t which) const {
    assert(which < MAX_TEXTURE_UNITS);

    switch(which) {
    case 0: return has_texcoord0();
    case 1: return has_texcoord1();
    case 2: return has_texcoord2();
    case 3: return has_texcoord3();
    case 4: return has_texcoord4();
    case 5: return has_texcoord5();
    case 6: return has_texcoord6();
    case 7: return has_texcoord7();
    default:
        return false;
    }
}

const VertexAttribute VertexSpecification::texcoordX_attribute(uint8_t which) const {
    assert(which < MAX_TEXTURE_UNITS);

    switch(which) {
    case 0: return texcoord0_attribute;
    case 1: return texcoord1_attribute;
    case 2: return texcoord2_attribute;
    case 3: return texcoord3_attribute;
    case 4: return texcoord4_attribute;
    case 5: return texcoord5_attribute;
    case 6: return texcoord6_attribute;
    case 7: return texcoord7_attribute;
    default:
        return VERTEX_ATTRIBUTE_NONE;
    }
}

void VertexSpecification::recalc_stride() {
    stride_ = (
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

uint32_t VertexSpecification::position_offset(bool check) const {
    if(check && !has_positions()) { throw std::logic_error("No such attribute"); }
    return 0;
}

uint32_t VertexSpecification::normal_offset(bool check) const {
    if(check && !has_normals()) { throw std::logic_error("No such attribute"); }
    return vertex_attribute_size(position_attribute);
}

uint32_t VertexSpecification::texcoord0_offset(bool check) const {
    if(check && !has_texcoord0()) { throw std::logic_error("No such attribute"); }
    return normal_offset(false) + vertex_attribute_size(normal_attribute);
}

uint32_t VertexSpecification::texcoord1_offset(bool check) const {
    if(check && !has_texcoord1()) { throw std::logic_error("No such attribute"); }
    return texcoord0_offset(false) + vertex_attribute_size(texcoord0_attribute);
}

uint32_t VertexSpecification::texcoord2_offset(bool check) const {
    if(check && !has_texcoord2()) { throw std::logic_error("No such attribute"); }
    return texcoord1_offset(false) + vertex_attribute_size(texcoord1_attribute);
}

uint32_t VertexSpecification::texcoord3_offset(bool check) const {
    if(check && !has_texcoord3()) { throw std::logic_error("No such attribute"); }
    return texcoord2_offset(false) + vertex_attribute_size(texcoord2_attribute);
}

uint32_t VertexSpecification::texcoord4_offset(bool check) const {
    if(check && !has_texcoord4()) { throw std::logic_error("No such attribute"); }
    return texcoord3_offset(false) + vertex_attribute_size(texcoord3_attribute);
}

uint32_t VertexSpecification::texcoord5_offset(bool check) const {
    if(check && !has_texcoord5()) { throw std::logic_error("No such attribute"); }
    return texcoord4_offset(false) + vertex_attribute_size(texcoord4_attribute);
}

uint32_t VertexSpecification::texcoord6_offset(bool check) const {
    if(check && !has_texcoord6()) { throw std::logic_error("No such attribute"); }
    return texcoord5_offset(false) + vertex_attribute_size(texcoord5_attribute);
}

uint32_t VertexSpecification::texcoord7_offset(bool check) const {
    if(check && !has_texcoord7()) { throw std::logic_error("No such attribute"); }
    return texcoord6_offset(false) + vertex_attribute_size(texcoord6_attribute);
}

uint32_t VertexSpecification::texcoordX_offset(uint8_t which, bool check) const {
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

uint32_t VertexSpecification::diffuse_offset(bool check) const {
    if(check && !has_diffuse()) { throw std::logic_error("No such attribute"); }
    return texcoord3_offset(false) + vertex_attribute_size(texcoord3_attribute);
}

uint32_t VertexSpecification::specular_offset(bool check) const {
    if(check && !has_specular()) { throw std::logic_error("No such attribute"); }
    return diffuse_offset(false) + vertex_attribute_size(diffuse_attribute);
}




}
