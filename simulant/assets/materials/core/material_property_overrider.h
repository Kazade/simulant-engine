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

    /* Helpers for std::string */
    template<typename T>
    void override_property_value(const std::string& str, const T& v) {
        override_property_value(str.c_str(), v);
    }

    template<typename T>
    bool fetch_property_value(const std::string& str, const T*& out) const {
        return fetch_property_value(str.c_str(), out);
    }

    bool clear_override(const char* name) {
        return clear_override(const_hash(name));
    }

    bool property_type(const char* property_name, MaterialPropertyType* type) const;

    std::vector<std::string> property_names_by_type(MaterialPropertyType type) const {
        if(parent_) {
            return parent_->property_names_by_type(type);
        }

        std::vector<std::string> names;
        for(auto& p: all_overrides_) {
            if(p.second == type) {
                names.push_back(HASHES_TO_NAMES.at(p.first));
            }
        }

        for(auto& p: core_properties()) {
            if(p.second == type) {
                names.push_back(p.first);
            }
        }

        return names;
    }

private:
    /* If we have a parent, then we can't override unless the property has
     * been defined on the parent - or it's a core property */
    bool check_existance(const char* property_name) const;
    bool clear_override(const unsigned hsh);

    MaterialPropertyOverrider* parent_ = nullptr;

    /* We sometimes need to reverse the hashing. Storing a map on every material when
     * the majority of the names will be the same is wasteful. Instead, we store a persistent
     * reverse lookup from hash -> name across all Materials and passes. */
    static std::unordered_map<MaterialPropertyNameHash, std::string> HASHES_TO_NAMES;

    std::unordered_map<MaterialPropertyNameHash, MaterialPropertyType> all_overrides_;
    std::unordered_map<MaterialPropertyNameHash, int> int_properties_;
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
