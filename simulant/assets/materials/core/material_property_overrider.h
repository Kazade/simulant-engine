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

    template<typename T>
    bool set_property_value(MaterialPropertyNameHash hsh, const char* name, const T& value) {
        clear_override(hsh);

        auto property_value_ptr =
            MaterialValuePool::get().get_or_create_value(value);

        if(properties_.count(hsh)) {
            properties_.at(hsh) = property_value_ptr;
        } else {
            properties_.insert(std::make_pair(hsh, property_value_ptr));
        }

        on_override(hsh, name, _impl::material_property_lookup<T>::type);
        return true;
    }

    template<typename T>
    bool set_property_value(const char* name, const T& value) {
        if(!valid_name(name)) {
            S_WARN("Ignoring invalid property name: {0}", name);
            return false;
        }

        if(parent_ && !parent_->check_existance(name)) {
            S_WARN("Ignoring unknown property override for {0}", name);
            return false;
        }

        return set_property_value(material_property_hash(name), name, value);
    }

    bool set_property_value(const char* name, const Color& value) {
        if(!valid_name(name)) {
            S_WARN("Ignoring invalid property name: {0}", name);
            return false;
        }

        if(parent_ && !parent_->check_existance(name)) {
            S_WARN("Ignoring unknown property override for {0}", name);
            return false;
        }

        return set_property_value(material_property_hash(name), name, (const Vec4&) value);
    }

    template<typename T>
    bool property_value(const MaterialPropertyNameHash hsh, const T*& out) const {
        auto it = properties_.find(hsh);
        if(it != properties_.end() && it->second) {
            out = it->second.get<T>();
            return true;
        } else if(parent_) {
            return parent_->property_value(hsh, out);
        }
        return false;
    }

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

    ContiguousMap<MaterialPropertyNameHash, MaterialPropertyValuePointer>
        properties_;
};

}
