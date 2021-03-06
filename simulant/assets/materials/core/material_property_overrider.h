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
    void override_property_value(const char* name, const TexturePtr& value);

    bool fetch_property_value(const char* name, const bool*& out) const;
    bool fetch_property_value(const char* name, const float*& out) const;
    bool fetch_property_value(const char* name, const int32_t *&out) const;
    bool fetch_property_value(const char* name, const Colour*& out) const;
    bool fetch_property_value(const char* name, const Vec2*& out) const;
    bool fetch_property_value(const char* name, const Vec3*& out) const;
    bool fetch_property_value(const char* name, const Vec4*& out) const;
    bool fetch_property_value(const char* name, const Mat3*& out) const;
    bool fetch_property_value(const char* name, const Mat4*& out) const;
    bool fetch_property_value(const char* name, const TexturePtr*& out) const;

    bool clear_override(const char* name) {
        return clear_override(const_hash(name));
    }

    bool property_type(const char* property_name, MaterialPropertyType* type) const;

private:
    /* If we have a parent, then we can't override unless the property has
     * been defined on the parent - or it's a core property */
    bool check_existance(const char* property_name) const;
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
    std::unordered_map<unsigned, TexturePtr> texture_properties_;
};

}
