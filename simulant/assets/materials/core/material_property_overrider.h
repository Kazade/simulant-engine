#pragma once

#include <unordered_map>

#include "core_material.h"

namespace smlt {

/* All materials and passes inherit the properties and
 * values of the core material. Overriders allow two things:
 *
 * 1. Overriding the value of the core material
 * 2. Adding additional property values (e.g. shader uniforms)
 */

enum MaterialPropertyType {
    MATERIAL_PROPERTY_TYPE_BOOL,
    MATERIAL_PROPERTY_TYPE_INT,
    MATERIAL_PROPERTY_TYPE_FLOAT,
    MATERIAL_PROPERTY_TYPE_VEC2,
    MATERIAL_PROPERTY_TYPE_VEC3,
    MATERIAL_PROPERTY_TYPE_VEC4,
    MATERIAL_PROPERTY_TYPE_MAT3,
    MATERIAL_PROPERTY_TYPE_MAT4
};

class MaterialPropertyOverrider {
public:
    MaterialPropertyOverrider();
    MaterialPropertyOverrider(const MaterialPropertyOverrider* parent);

    void override_property_value(const char* name, const bool& value);
    void override_property_value(const char* name, const float& value);
    void override_property_value(const char* name, const int32_t& value);
    void override_property_value(const char* name, const Vec4& value);
    void override_property_value(const char* name, const Vec3& value);
    void override_property_value(const char* name, const Vec2& value);
    void override_property_value(const char* name, const Mat3& value);
    void override_property_value(const char* name, const Mat4& value);

    bool fetch_property_value(const char* name, bool* out);
    bool fetch_property_value(const char* name, float* out);
    bool fetch_property_value(const char* name, int32_t* out);
    bool fetch_property_value(const char* name, Colour* out);
    bool fetch_property_value(const char* name, Vec2* out);
    bool fetch_property_value(const char* name, Vec3* out);
    bool fetch_property_value(const char* name, Vec4* out);
    bool fetch_property_value(const char* name, Mat3* out);
    bool fetch_property_value(const char* name, Mat4* out);

    bool clear_override(const char* name) {
        return clear_override(const_hash(name));
    }

private:
    bool clear_override(const unsigned hsh);


    MaterialPropertyOverrider* parent_ = nullptr;

    std::unordered_map<unsigned, MaterialPropertyType> all_overrides_;

    std::unordered_map<unsigned, int> int_properties_;
    std::unordered_map<unsigned, float> float_properties_;
    std::unordered_map<unsigned, bool> bool_properties_;
    std::unordered_map<unsigned, Vec2> vec2_properties_;
    std::unordered_map<unsigned, Vec3> vec3_properties_;
    std::unordered_map<unsigned, Vec4> vec4_properties_;
    std::unordered_map<unsigned, Mat3> mat3_properties_;
    std::unordered_map<unsigned, Mat4> mat4_properties_;

};

}
