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

const Vec3 Vec3::NEGATIVE_X = Vec3(-1, 0, 0);
const Vec3 Vec3::POSITIVE_X = Vec3(1, 0, 0);
const Vec3 Vec3::NEGATIVE_Y = Vec3(0, -1, 0);
const Vec3 Vec3::POSITIVE_Y = Vec3(0, 1, 0);
const Vec3 Vec3::POSITIVE_Z = Vec3(0, 0, 1);
const Vec3 Vec3::NEGATIVE_Z = Vec3(0, 0, -1);

Radians to_radians(const Degrees& degrees) {
    return Radians(degrees.value * PI_OVER_180);
}

Degrees to_degrees(const Radians& radians) {
    return Degrees(radians.value * PI_UNDER_180);
}

bool operator==(const Vec2& lhs, const Vec2& rhs) {
    return glm::operator==(lhs, rhs);
}

bool operator!=(const Vec2& lhs, const Vec2& rhs) {
    return !(lhs == rhs);
}

smlt::Vec2 operator*(float lhs, const smlt::Vec2& rhs) {
    smlt::Vec2 result = rhs;
    result *= lhs;
    return result;
}

smlt::Vec3 operator*(float lhs, const smlt::Vec3& rhs) {
    smlt::Vec3 result = rhs;
    result *= lhs;
    return result;
}

smlt::Vec3 operator/(float lhs, const smlt::Vec3& rhs) {
    smlt::Vec3 result = rhs;
    result /= lhs;
    return result;
}

Vec3 Vec3::random_deviant(const Degrees& angle, const Vec3 up) const {
    //Lovingly adapted from ogre
    Vec3 new_up = (up == Vec3()) ? perpendicular() : up;

    Quaternion q(*this, Radians(random_gen::random_float(0, 1) * (PI * 2.0)));

    new_up = new_up * q;

    q = Quaternion(new_up, angle);

    return *this * q;

}

Vec3 Vec3::operator*(const Quaternion &rhs) const {
    return rhs.rotate_vector(*this);
}

Vec3 Vec3::rotated_by(const Mat4 &rot) const {
    auto tmp = rot * Vec4(*this, 0);
    return Vec3(tmp.x, tmp.y, tmp.z);
}

Vec3 Vec3::transformed_by(const Mat4 &trans) const {
    auto tmp = trans * Vec4(*this, 1);
    return Vec3(tmp.x, tmp.y, tmp.z);
}

Vec3 Vec3::perpendicular() const {
    //Lovingly adapted from Ogre
    static const float square_zero = (float)(1e-06 * 1e-06);
    Vec3 perp = this->cross(Vec3(1, 0, 0));

    if(perp.length_squared() < square_zero) {
        //This vector is the X-axis, so use another
        perp = this->cross(Vec3(0, 1, 0));
    }
    return perp.normalized();
}

smlt::Vec3 operator-(const smlt::Vec3& vec) {
    return smlt::Vec3(-vec.x, -vec.y, -vec.z);
}

smlt::Quaternion operator-(const smlt::Quaternion& q) {
    return smlt::Quaternion(-q.x, -q.y, -q.z, -q.w);
}

std::ostream& operator<<(std::ostream& stream, const Vec2& vec) {
    stream << "(" << vec.x << "," << vec.y << ")";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Vec3& vec) {
    stream << "(" << vec.x << "," << vec.y << "," << vec.z << ")";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Vec4& vec) {
    stream << "(" << vec.x << "," << vec.y << "," << vec.z << "," << vec.w << ")";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Quaternion& quat) {
    stream << "(" << quat.x << "," << quat.y << "," << quat.z << "," << quat.w << ")";
    return stream;
}

Quaternion Quaternion::as_look_at(const Vec3& direction, const Vec3& up=Vec3(0, 1, 0)) {
    Mat4 lookat = Mat4::as_look_at(Vec3(), direction, up);
    Quaternion ret = glm::quat_cast(lookat);
    return ret;
}

Quaternion::Quaternion(const Vec3 &axis, const Degrees &degrees) {
    *this = glm::angleAxis(Radians(degrees).value, axis);
}

Quaternion::Quaternion(const Mat3& rot_matrix) {
    /* FIXME: This should be in kazmath */

    float m12 = rot_matrix[7];
    float m21 = rot_matrix[5];
    float m02 = rot_matrix[6];
    float m20 = rot_matrix[2];
    float m10 = rot_matrix[1];
    float m01 = rot_matrix[3];
    float m00 = rot_matrix[0];
    float m11 = rot_matrix[4];
    float m22 = rot_matrix[8];
    float t = m00 + m11 + m22;
    // we protect the division by s by ensuring that s>=1
    if (t >= 0) { // by w
        float s = sqrt(t + 1);
        w = 0.5 * s;
        s = 0.5 / s;
        x = (m21 - m12) * s;
        y = (m02 - m20) * s;
        z = (m10 - m01) * s;
    } else if ((m00 > m11) && (m00 > m22)) { // by x
        float s = sqrt(1 + m00 - m11 - m22);
        x = s * 0.5;
        s = 0.5 / s;
        y = (m10 + m01) * s;
        z = (m02 + m20) * s;
        w = (m21 - m12) * s;
    } else if (m11 > m22) { // by y
        float s = sqrt(1 + m11 - m00 - m22);
        y = s * 0.5;
        s = 0.5 / s;
        x = (m10 + m01) * s;
        z = (m21 + m12) * s;
        w = (m02 - m20) * s;
    } else { // by z
        float s = sqrt(1 + m22 - m00 - m11);
        z = s * 0.5;
        s = 0.5 / s;
        x = (m02 + m20) * s;
        y = (m21 + m12) * s;
        w = (m10 - m01) * s;
    }
}

Vec3 Quaternion::rotate_vector(const Vec3 &v) const {
    Vec3 ret;
    ret = glm::rotate(*this, v);
    return ret;
}

AxisAngle Quaternion::to_axis_angle() const {
    AxisAngle ret;
    ret.axis = glm::axis(*this);
    ret.angle = Degrees(glm::angle(*this));
    return ret;
}

Vec4 Mat4::operator*(const Vec4 &rhs) const {
    return glm::operator*(*this, rhs);
}

void Mat4::extract_rotation_and_translation(Quaternion& rotation, Vec3& translation) const {

    glm::vec3 scale;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(*this, scale, rotation, translation, skew, perspective);
    rotation = glm::conjugate(rotation);
}

Mat4 Mat4::as_rotation_x(const Degrees &angle) {
    return glm::rotate(Mat4(), Radians(angle).value, glm::vec3(1, 0, 0));
}

Mat4 Mat4::as_rotation_y(const Degrees &angle) {
    return glm::rotate(Mat4(), Radians(angle).value, glm::vec3(0, 1, 0));
}

Mat4 Mat4::as_rotation_z(const Degrees &angle) {
    return glm::rotate(Mat4(), Radians(angle).value, glm::vec3(0, 0, 1));
}

Mat3 Mat3::from_rotation_x(const Degrees& angle) {
    return glm::mat3_cast(Quaternion(angle, Degrees(), Degrees()));
}

Mat3 Mat3::from_rotation_y(const Degrees &angle) {
    return glm::mat3_cast(Quaternion(Degrees(), angle, Degrees()));
}

Mat3 Mat3::from_rotation_z(const Degrees& angle) {
    return glm::mat3_cast(Quaternion(Degrees(), Degrees(), angle));
}

Mat3::Mat3(const Mat4 &rhs) {
    Quaternion q;
    Vec3 v;
    rhs.extract_rotation_and_translation(q, v);
    *this = glm::mat3_cast(q);
}

Vec3 Mat3::transform_vector(const Vec3 &v) const {
    return Vec3(glm::vec3(v.x, v.y, v.z) * (glm::mat3x3&) *this);
}

Mat4 Mat4::as_scaling(float s) {
    Mat4 ret = glm::scale(glm::mat4x4(), glm::vec3(s, s, s));
    return ret;
}

Mat4 Mat4::as_translation(const Vec3 &v) {
    Mat4 ret;
    ret = glm::translate((glm::mat4x4) ret, (glm::vec3) v);
    return ret;
}

Plane Mat4::extract_plane(FrustumPlane plane) const {
    float t = 1.0f;
    Plane out;

    switch(plane) {
    case FRUSTUM_PLANE_RIGHT:
        out.n.x = (*this)[3] - (*this)[0];
        out.n.y = (*this)[7] - (*this)[4];
        out.n.z = (*this)[11] - (*this)[8];
        out.d = (*this)[15] - (*this)[12];
        break;
    case FRUSTUM_PLANE_LEFT:
        out.n.x = (*this)[3] + (*this)[0];
        out.n.y = (*this)[7] + (*this)[4];
        out.n.z = (*this)[11] + (*this)[8];
        out.d = (*this)[15] + (*this)[12];
        break;
    case FRUSTUM_PLANE_BOTTOM:
        out.n.x = (*this)[3] + (*this)[1];
        out.n.y = (*this)[7] + (*this)[5];
        out.n.z = (*this)[11] + (*this)[9];
        out.d = (*this)[15] + (*this)[13];
        break;
    case FRUSTUM_PLANE_TOP:
        out.n.x = (*this)[3] - (*this)[1];
        out.n.y = (*this)[7] - (*this)[5];
        out.n.z = (*this)[11] - (*this)[9];
        out.d = (*this)[15] - (*this)[13];
        break;
    case FRUSTUM_PLANE_FAR:
        out.n.x = (*this)[3] - (*this)[2];
        out.n.y = (*this)[7] - (*this)[6];
        out.n.z = (*this)[11] - (*this)[10];
        out.d = (*this)[15] - (*this)[14];
        break;
    case FRUSTUM_PLANE_NEAR:
        out.n.x = (*this)[3] + (*this)[2];
        out.n.y = (*this)[7] + (*this)[6];
        out.n.z = (*this)[11] + (*this)[10];
        out.d = (*this)[15] + (*this)[14];
        break;
    default:
        assert(0 && "Invalid plane index");
    }

    t = sqrtf(out.n.x * out.n.x +
              out.n.y * out.n.y +
              out.n.z * out.n.z);
    out.n.x /= t;
    out.n.y /= t;
    out.n.z /= t;
    out.d /= t;

    return out;
}

Mat4 Mat4::as_look_at(const Vec3& eye, const Vec3& target, const Vec3& up) {
    Mat4 ret = glm::lookAt(eye, target, up);
    return ret;
}

Degrees::Degrees(const Radians &rhs):
    value(rhs.value * PI_UNDER_180) {

}

Radians::Radians(const Degrees &rhs):
    value(rhs.value * PI_OVER_180) {
}

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

VertexSpecification::VertexSpecification(
        VertexAttribute position, VertexAttribute normal, VertexAttribute texcoord0,
        VertexAttribute texcoord1, VertexAttribute texcoord2, VertexAttribute texcoord3,
        VertexAttribute diffuse, VertexAttribute specular):
    position_attribute(position),
    normal_attribute(normal),
    texcoord0_attribute(texcoord0),
    texcoord1_attribute(texcoord1),
    texcoord2_attribute(texcoord2),
    texcoord3_attribute(texcoord3),
    diffuse_attribute(diffuse),
    specular_attribute(specular) {

    recalc_stride();
}

void VertexSpecification::recalc_stride() {
    stride_ = (
                vertex_attribute_size(position_attribute) +
                vertex_attribute_size(normal_attribute) +
                vertex_attribute_size(texcoord0_attribute) +
                vertex_attribute_size(texcoord1_attribute) +
                vertex_attribute_size(texcoord2_attribute) +
                vertex_attribute_size(texcoord3_attribute) +
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

uint32_t VertexSpecification::diffuse_offset(bool check) const {
    if(check && !has_diffuse()) { throw std::logic_error("No such attribute"); }
    return texcoord3_offset(false) + vertex_attribute_size(texcoord3_attribute);
}

uint32_t VertexSpecification::specular_offset(bool check) const {
    if(check && !has_specular()) { throw std::logic_error("No such attribute"); }
    return diffuse_offset(false) + vertex_attribute_size(diffuse_attribute);
}

float smlt::math::lerp(float a, float b, float t) {
    t = (t > 1.0) ? 1.0 : t;
    t = (t < 0.0) ? 0.0 : t;
    return a + ((b - a) * t);
}

Degrees math::lerp_angle(Degrees a, Degrees b, float t) {
    while(a.value > b.value + 180.0) {
        b.value += 360.0;
    }

    while(b.value > a.value + 180.0) {
        b.value -= 360.0;
    }

    return Degrees(lerp(a.value, b.value, t));
}

Vec2 Vec2::rotated_by(Degrees degrees) const {
    // FIXME: Lazy costly indirect way, should just do it manually
    Mat4 rotation_z = Mat4::as_rotation_z(degrees);
    auto result = Vec3(x, y, 0).rotated_by(rotation_z);
    return Vec2(result.x, result.y);
}

Vec3 Plane::project(const Vec3 &p) {
    /* Not sure if there is a more direct way to do this in GLM.. */

    return p - (n.dot(p) + d) * n;
}

float Plane::distance_to(const Vec3 &p) {
    float k1 = d;
    float k2 = (n.x * p.x) + (n.y * p.y) + (n.z * p.z);
    return k2 - k1;
}

PlaneClassification Plane::classify_point(const Vec3 &p) const {
    /* This function will determine if a point is on, in front of, or behind*/
    /* the plane.  First we store the dot product of the plane and the point.*/
    auto distance = n.x * p.x + n.y * p.y + n.z * p.z + d;

    /* Simply put if the dot product is greater than 0 then it is infront of it.*/
    /* If it is less than 0 then it is behind it.  And if it is 0 then it is on it.*/
    if(distance > std::numeric_limits<float>::epsilon()) return PlaneClassification(1);
    if(distance < -std::numeric_limits<float>::epsilon()) return PlaneClassification(-1);

    return PlaneClassification(0);
}

smlt::optional<Vec3> Plane::intersect_planes(const Plane &p1, const Plane &p2, const Plane &p3) {
    const Vec3& n1 = p1.n;
    const Vec3& n2 = p2.n;
    const Vec3& n3 = p3.n;

    auto cross = n2.cross(n3);
    auto denom = n1.dot(cross);

    if(almost_equal(denom, 0.0f)) {
        return smlt::optional<Vec3>();
    }

    auto r1 = n2.cross(n3);
    auto r2 = n3.cross(n1);
    auto r3 = n1.cross(n2);

    r1 *= -p1.d;
    r2 *= p2.d;
    r3 *= p3.d;

    auto ret = r1 - r2 - r3;

    ret *= 1.0 / denom;

    return ret;
}

bool Ray::intersects_aabb(const AABB &aabb) const {
    //http://gamedev.stackexchange.com/a/18459/15125
    Vec3 rdir = this->dir.normalized();
    Vec3 dirfrac(1.0 / rdir.x, 1.0 / rdir.y, 1.0 / rdir.z);

    float t1 = (aabb.min.x - start.x) * dirfrac.x;
    float t2 = (aabb.max.x - start.x) * dirfrac.x;
    float t3 = (aabb.min.y - start.y) * dirfrac.y;
    float t4 = (aabb.max.y - start.y) * dirfrac.y;
    float t5 = (aabb.min.z - start.z) * dirfrac.z;
    float t6 = (aabb.max.z - start.z) * dirfrac.z;

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
