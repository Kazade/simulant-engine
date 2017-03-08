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

Radians to_radians(const Degrees& degrees) {
    return Radians(kmDegreesToRadians(degrees.value));
}

Degrees to_degrees(const Radians& radians) {
    return Degrees(kmRadiansToDegrees(radians.value));
}

bool operator==(const Vec2& lhs, const Vec2& rhs) {
    auto ret = kmVec2AreEqual(&lhs, &rhs);
    return ret;
}

bool operator!=(const Vec2& lhs, const Vec2& rhs) {
    return !(lhs == rhs);
}

smlt::Vec2 operator*(float lhs, const smlt::Vec2& rhs) {
    smlt::Vec2 result;
    kmVec2Scale(&result, &rhs, lhs);
    return result;
}

smlt::Vec3 operator*(float lhs, const smlt::Vec3& rhs) {
    smlt::Vec3 result;
    kmVec3Scale(&result, &rhs, lhs);
    return result;
}

smlt::Vec3 operator/(float lhs, const smlt::Vec3& rhs) {
    smlt::Vec3 result;
    kmVec3Scale(&result, &rhs, 1.0 / lhs);
    return result;
}

Vec3 Vec3::random_deviant(const Degrees& angle, const Vec3 up) const {
    //Lovingly adapted from ogre
    Vec3 new_up = (up == Vec3()) ? perpendicular() : up;

    Quaternion q;
    kmQuaternionRotationAxisAngle(&q, this, random_gen::random_float(0, 1) * (PI * 2.0));
    kmQuaternionMultiplyVec3(&new_up, &q, &new_up);
    kmQuaternionRotationAxisAngle(&q, &new_up, Radians(angle).value);

    Vec3 ret;
    kmQuaternionMultiplyVec3(&ret, &q, this);

    return ret;
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

Quaternion Quaternion::look_rotation(const Vec3& direction, const Vec3& up=Vec3(0, 1, 0)) {
    Quaternion res;
    kmQuaternionLookRotation(&res, &direction, &up);
    kmQuaternionNormalize(&res, &res);
    return res;
}

Quaternion::Quaternion(const Degrees &degrees, const Vec3 &axis) {
    kmQuaternionRotationAxisAngle(
        this,
        &axis,
        kmDegreesToRadians(degrees.value)
    );
}

Quaternion::Quaternion(const Mat3& rot_matrix) {
    /* FIXME: This should be in kazmath */

    float m12 = rot_matrix.mat[7];
    float m21 = rot_matrix.mat[5];
    float m02 = rot_matrix.mat[6];
    float m20 = rot_matrix.mat[2];
    float m10 = rot_matrix.mat[1];
    float m01 = rot_matrix.mat[3];
    float m00 = rot_matrix.mat[0];
    float m11 = rot_matrix.mat[4];
    float m22 = rot_matrix.mat[8];
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


void Mat4::extract_rotation_and_translation(Quaternion& rotation, Vec3& translation) {
    Mat3 rot;
    kmMat4ExtractRotationMat3(this, &rot);
    kmQuaternionRotationMatrix(&rotation, &rot);
    kmMat4ExtractTranslationVec3(this, &translation);
}

Mat3 Mat3::from_rotation_x(float pitch) {
    Mat3 ret;
    kmMat3FromRotationX(&ret, pitch);
    return ret;
}

Mat3 Mat3::from_rotation_y(float yaw) {
    Mat3 ret;
    kmMat3FromRotationY(&ret, yaw);
    return ret;
}

Mat3 Mat3::from_rotation_z(float roll) {
    Mat3 ret;
    kmMat3FromRotationZ(&ret, roll);
    return ret;
}

Mat4 Mat4::as_scaling(float s) {
    Mat4 ret;
    kmMat4Scaling(&ret, s, s, s);
    return ret;
}

Mat4 Mat4::from_lookat(const Vec3& eye, const Vec3& target, const Vec3& up) {
    Mat4 ret;
    kmMat4LookAt(&ret, &eye, &target, &up);
    return ret;
}

Degrees::Degrees(const Radians &rhs) {
    value = kmRadiansToDegrees(rhs.value);
}

Radians::Radians(const Degrees &rhs) {
    value = kmDegreesToRadians(rhs.value);
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

}
