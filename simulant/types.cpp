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
        assert(0 && "Invalid attribute specified");
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
    static const std::array<bool (VertexSpecification::*)() const, 8> LOOKUPS = {{
            &VertexSpecification::has_texcoord0,
            &VertexSpecification::has_texcoord1,
            &VertexSpecification::has_texcoord2,
            &VertexSpecification::has_texcoord3,
            &VertexSpecification::has_texcoord4,
            &VertexSpecification::has_texcoord5,
            &VertexSpecification::has_texcoord6,
            &VertexSpecification::has_texcoord7
    }};

    assert(which < MAX_TEXTURE_UNITS);

    return std::bind(LOOKUPS[which], this)();
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
        throw std::out_of_range("Invalid texcoord");
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
    static const std::array<uint32_t (VertexSpecification::*)(bool) const, 8> LOOKUPS = {
            &VertexSpecification::texcoord0_offset,
            &VertexSpecification::texcoord1_offset,
            &VertexSpecification::texcoord2_offset,
            &VertexSpecification::texcoord3_offset,
            &VertexSpecification::texcoord4_offset,
            &VertexSpecification::texcoord5_offset,
            &VertexSpecification::texcoord6_offset,
            &VertexSpecification::texcoord7_offset
    };

    assert(which < MAX_TEXTURE_UNITS);

    return std::bind(LOOKUPS[which], this, check)();
}

uint32_t VertexSpecification::diffuse_offset(bool check) const {
    if(check && !has_diffuse()) { throw std::logic_error("No such attribute"); }
    return texcoord3_offset(false) + vertex_attribute_size(texcoord3_attribute);
}

uint32_t VertexSpecification::specular_offset(bool check) const {
    if(check && !has_specular()) { throw std::logic_error("No such attribute"); }
    return diffuse_offset(false) + vertex_attribute_size(diffuse_attribute);
}


bool Ray::intersects_aabb(const AABB &aabb) const {
    //http://gamedev.stackexchange.com/a/18459/15125
    Vec3 rdir = this->dir.normalized();
    Vec3 dirfrac(1.0 / rdir.x, 1.0 / rdir.y, 1.0 / rdir.z);

    float t1 = (aabb.min().x - start.x) * dirfrac.x;
    float t2 = (aabb.max().x - start.x) * dirfrac.x;
    float t3 = (aabb.min().y - start.y) * dirfrac.y;
    float t4 = (aabb.max().y - start.y) * dirfrac.y;
    float t5 = (aabb.min().z - start.z) * dirfrac.z;
    float t6 = (aabb.max().z - start.z) * dirfrac.z;

    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
    if(tmax < 0) {
        return false;
    }

    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
        return false;
    }

    return false;
}

bool Ray::intersects_triangle(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3, Vec3 *intersection, Vec3 *normal, float *distance) const {
    Vec3 hit;
    bool ret = glm::intersectLineTriangle(
                (const glm::vec3&) start,
                (const glm::vec3&) dir,
                (const glm::vec3&) v1,
                (const glm::vec3&) v2,
                (const glm::vec3&) v3,
                (glm::vec3&) hit
                );

    if(ret) {
        if(intersection) *intersection = hit;
        if(normal) {
            *normal = (v2 - v1).cross(v3 - v1).normalized();
        }

        if(distance) {
            *distance = (hit - start).length();
        }
    }

    return ret;
}


}
