#pragma once

#include <unordered_map>
#include <list>
#include <string>

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
    MaterialPropertyOverrider() = default;
    MaterialPropertyOverrider(const MaterialPropertyOverrider* parent):
        parent_(parent) {}

    void set_property_value(const char* name, const bool& value);
    void set_property_value(const char* name, const float& value);
    void set_property_value(const char* name, const int32_t& value);
    void set_property_value(const char* name, const long& value) {
        set_property_value(name, (int32_t) value);
    }

    void set_property_value(const char* name, const Colour& value);
    void set_property_value(const char* name, const Vec4& value);
    void set_property_value(const char* name, const Vec3& value);
    void set_property_value(const char* name, const Vec2& value);
    void set_property_value(const char* name, const Mat3& value);
    void set_property_value(const char* name, const Mat4& value);
    void set_property_value(const char* name, const TexturePtr& value);

    bool property_value(const MaterialPropertyNameHash hsh, const bool*& out) const;
    bool property_value(const MaterialPropertyNameHash hsh, const float*& out) const;
    bool property_value(const MaterialPropertyNameHash hsh, const int32_t *&out) const;

    bool property_value(const MaterialPropertyNameHash hsh, const Colour*& out) const;
    bool property_value(const MaterialPropertyNameHash hsh, const Vec2*& out) const;
    bool property_value(const MaterialPropertyNameHash hsh, const Vec3*& out) const;

    bool property_value(const MaterialPropertyNameHash hsh, const Vec4*& out) const;

    bool property_value(const MaterialPropertyNameHash hsh, const Mat3*& out) const;
    bool property_value(const MaterialPropertyNameHash hsh, const Mat4*& out) const;
    bool property_value(const MaterialPropertyNameHash hsh, const TexturePtr*& out) const;

    /* Helpers for std::string */
    template<typename T>
    void set_property_value(const std::string& str, const T& v) {
        set_property_value(str.c_str(), v);
    }

    template<typename T>
    bool property_value(const std::string& str, const T*& out) const {
        return property_value(str.c_str(), out);
    }

    template<typename T>
    bool property_value(const char* name, const T*& out) const {
        auto hsh = material_property_hash(name);
        return property_value(hsh, out);
    }

    bool clear_override(const char* name) {
        return clear_override(material_property_hash(name));
    }

    bool property_type(const char* property_name, MaterialPropertyType* type) const;

protected:
    virtual void on_override(
        MaterialPropertyNameHash hsh,
        const char* name,
        MaterialPropertyType type
    ) {
        _S_UNUSED(hsh);
        _S_UNUSED(name);
        _S_UNUSED(type);
    }

    virtual void on_clear_override(MaterialPropertyNameHash hsh) { _S_UNUSED(hsh); }

    /* If we have a parent, then we can't override unless the property has
     * been defined on the parent - or it's a core property */
    bool check_existance(const MaterialPropertyNameHash hsh) const;
    bool check_existance(const char* property_name) const;
    bool clear_override(const unsigned hsh);

    const MaterialPropertyOverrider* parent_ = nullptr;

    std::unordered_map<MaterialPropertyNameHash, MaterialPropertyType> all_overrides_;
    std::unordered_map<MaterialPropertyNameHash, int32_t> int_properties_;
    std::unordered_map<MaterialPropertyNameHash, float> float_properties_;
    std::unordered_map<MaterialPropertyNameHash, bool> bool_properties_;
    std::unordered_map<MaterialPropertyNameHash, Vec2> vec2_properties_;
    std::unordered_map<MaterialPropertyNameHash, Vec3> vec3_properties_;
    std::unordered_map<MaterialPropertyNameHash, Vec4> vec4_properties_;
    std::unordered_map<MaterialPropertyNameHash, Mat3> mat3_properties_;
    std::unordered_map<MaterialPropertyNameHash, Mat4> mat4_properties_;
    std::unordered_map<MaterialPropertyNameHash, TexturePtr> texture_properties_;
};

}
