#pragma once

#include <unordered_map>
#include <list>
#include <string>

#include "../../../generic/containers/contiguous_map.h"
#include "../property_value.h"
#include "core_material.h"
#include "material_value_pool.h"

namespace smlt {

/* All materials and passes inherit the properties and
 * values of the core material. Overriders allow two things:
 *
 * 1. Overriding the value of the core material
 * 2. Adding additional property values (e.g. shader uniforms)
 */

bool valid_name(const char* name);

class MaterialPropertyOverrider {
public:
    MaterialPropertyOverrider() = default;
    MaterialPropertyOverrider(const MaterialPropertyOverrider* parent):
        parent_(parent) {}

protected:
    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const bool& value) = 0;
    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const float& value) = 0;
    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const int32_t& value) = 0;
    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const Vec2& value) = 0;
    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const Vec3& value) = 0;
    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const Vec4& value) = 0;
    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const Mat3& value) = 0;
    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const Mat4& value) = 0;
    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name,
                                    const TexturePtr& value) = 0;

    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const bool*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const float*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const int32_t*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const Vec2*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const Vec3*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const Vec4*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const Mat3*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const Mat4*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const TexturePtr*& out) const = 0;

    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const Color& value) {
        return set_property_value(
            hsh, name,
            (const Vec4&)value); // FIXME: dirty cast, add to_vec4() to Color
    }

    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                const Color*& out) const {
        return property_value(hsh, (const Vec4*&)out); // FIXME: dirty cast
    }

    /* Helpers for std::string */
    template<typename T>
    void set_property_value(const std::string& str, const T& v) {
        MaterialPropertyNameHash hsh = material_property_hash(str.c_str());
        set_property_value(hsh, str.c_str(), v);
    }

    template<typename T>
    void set_property_value(const char* name, const T& v) {
        MaterialPropertyNameHash hsh = material_property_hash(name);
        set_property_value(hsh, name, v);
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
        return on_clear_override(material_property_hash(name));
    }

    bool clear_override(MaterialPropertyNameHash hsh) {
        return on_clear_override(hsh);
    }

    bool check_existance(const char* property_name) const {
        return on_check_existence(material_property_hash(property_name));
    }

    virtual bool property_type(const char* property_name,
                               MaterialPropertyType* type) const = 0;

    virtual bool on_check_existence(MaterialPropertyNameHash hsh) const = 0;

protected:
    virtual void on_override(MaterialPropertyNameHash hsh, const char* name,
                             MaterialPropertyType type) {
        _S_UNUSED(hsh);
        _S_UNUSED(name);
        _S_UNUSED(type);
    }

    virtual bool on_clear_override(MaterialPropertyNameHash hsh) = 0;
    const MaterialPropertyOverrider* parent_ = nullptr;
};

} // namespace smlt
