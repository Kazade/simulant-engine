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
                                float*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                int32_t*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                Vec2*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                Vec3*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                Vec4*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                Mat3*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                Mat4*& out) const = 0;
    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                TexturePtr*& out) const = 0;

    virtual bool set_property_value(MaterialPropertyNameHash hsh,
                                    const char* name, const Color& value) {
        return set_property_value(
            hsh, name,
            (const Vec4&)value); // FIXME: dirty cast, add to_vec4() to Color
    }

    virtual bool property_value(const MaterialPropertyNameHash hsh,
                                Color*& out) const {
        return property_value(hsh, (Vec4*&)out); // FIXME: dirty cast
    }

    virtual bool check_existence(MaterialPropertyNameHash hsh) const = 0;
    virtual bool clear_override(MaterialPropertyNameHash hsh) = 0;

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

    bool check_existance(const char* property_name) const {
        return check_existence(material_property_hash(property_name));
    }

    virtual bool property_type(const char* property_name,
                               MaterialPropertyType* type) const = 0;

protected:
    const MaterialPropertyOverrider* parent_ = nullptr;
};

}
