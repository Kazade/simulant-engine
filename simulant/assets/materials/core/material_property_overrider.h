#pragma once

#include <unordered_map>
#include <list>
#include <string>

#include "core_material.h"
#include "../../../generic/containers/contiguous_map.h"

namespace smlt {

/* All materials and passes inherit the properties and
 * values of the core material. Overriders allow two things:
 *
 * 1. Overriding the value of the core material
 * 2. Adding additional property values (e.g. shader uniforms)
 */

namespace _impl {
    template<typename T>
    struct material_property_lookup;

    template<>
    struct material_property_lookup<bool> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_BOOL;
    };

    template<>
    struct material_property_lookup<int32_t> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_INT;
    };

    template<>
    struct material_property_lookup<float> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_FLOAT;
    };

    template<>
    struct material_property_lookup<Vec2> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_VEC2;
    };

    template<>
    struct material_property_lookup<Vec3> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_VEC3;
    };

    template<>
    struct material_property_lookup<Vec4> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_VEC4;
    };

    template<>
    struct material_property_lookup<Mat3> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_MAT3;
    };

    template<>
    struct material_property_lookup<Mat4> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_MAT4;
    };

    template<>
    struct material_property_lookup<TexturePtr> {
        const static MaterialPropertyType type = MATERIAL_PROPERTY_TYPE_TEXTURE;
    };
}

bool valid_name(const char* name);

class MaterialPropertyOverrider {
public:
    MaterialPropertyOverrider() = default;
    MaterialPropertyOverrider(const MaterialPropertyOverrider* parent):
        parent_(parent) {}

    template<typename T>
    void set_property_value(const char* name, const T& value) {
        if(!valid_name(name)) {
            S_WARN("Ignoring invalid property name: {0}", name);
            return;
        }

        if(parent_ && !parent_->check_existance(name)) {
            S_WARN("Ignoring unknown property override for {0}", name);
            return;
        }

        auto hsh = material_property_hash(name);
        clear_override(hsh);

        properties_.insert(std::make_pair(hsh, PropertyValue(value)));
        on_override(hsh, name, _impl::material_property_lookup<T>::type);
    }

    void set_property_value(const char* name, const Colour& value) {
        set_property_value(name, (const Vec4&) value);
    }

    template<typename T>
    bool property_value(const MaterialPropertyNameHash hsh, const T*& out) const {
        auto it = properties_.find(hsh);
        if(it != properties_.end()) {
            out = it->second.get<T>();
            return true;
        } else if(parent_) {
            return parent_->property_value(hsh, out);
        } else if(is_core_property(hsh)) {
            return core_material_property_value(hsh, out);
        } else {
            return false;
        }
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

    struct PropertyValue {
        MaterialPropertyType type;
        uint8_t* data = nullptr;

        PropertyValue() = default;

        template<typename T>
        PropertyValue(T value) {
            set(value);
        }

        PropertyValue(const PropertyValue& rhs) {
            switch(rhs.type) {
                case MATERIAL_PROPERTY_TYPE_BOOL:
                    set(*rhs.get<bool>());
                break;
                    case MATERIAL_PROPERTY_TYPE_INT:
                    set(*rhs.get<int32_t>());
                break;
                    case MATERIAL_PROPERTY_TYPE_FLOAT:
                    set(*rhs.get<float>());
                break;
                case MATERIAL_PROPERTY_TYPE_VEC2:
                    set(*rhs.get<Vec2>());
                break;
                case MATERIAL_PROPERTY_TYPE_VEC3:
                    set(*rhs.get<Vec3>());
                break;
                case MATERIAL_PROPERTY_TYPE_VEC4:
                    set(*rhs.get<Vec4>());
                break;
                case MATERIAL_PROPERTY_TYPE_MAT3:
                    set(*rhs.get<Mat3>());
                break;
                case MATERIAL_PROPERTY_TYPE_MAT4:
                    set(*rhs.get<Mat4>());
                break;
                case MATERIAL_PROPERTY_TYPE_TEXTURE:
                    set(*rhs.get<TexturePtr>());
                break;
            }
        }

        PropertyValue& operator=(const PropertyValue& rhs) = delete;

        ~PropertyValue() {
            if(data) {
                clear();
            }
        }

        void clear() {
            if(!data) {
                return;
            }

            switch(type) {
                case MATERIAL_PROPERTY_TYPE_BOOL:
                    unset<bool>();
                break;
                    case MATERIAL_PROPERTY_TYPE_INT:
                    unset<int32_t>();
                break;
                    case MATERIAL_PROPERTY_TYPE_FLOAT:
                    unset<float>();
                break;
                case MATERIAL_PROPERTY_TYPE_VEC2:
                    unset<Vec2>();
                break;
                case MATERIAL_PROPERTY_TYPE_VEC3:
                    unset<Vec3>();
                break;
                case MATERIAL_PROPERTY_TYPE_VEC4:
                    unset<Vec4>();
                break;
                case MATERIAL_PROPERTY_TYPE_MAT3:
                    unset<Mat3>();
                break;
                case MATERIAL_PROPERTY_TYPE_MAT4:
                    unset<Mat4>();
                break;
                case MATERIAL_PROPERTY_TYPE_TEXTURE:
                    unset<TexturePtr>();
                break;
            }

            data = nullptr;
        }

        template<typename T>
        void set(T value) {
            clear();

            data = (uint8_t*) aligned_alloc(alignof(T), sizeof(T));
            new (data) T(value);
            type = _impl::material_property_lookup<T>::type;
        }

        template<typename T>
        T* get() const {
            return (T*) data;
        }

        template<typename T>
        void unset() {
            ((T*) data)->~T();
            free(data);
            data = nullptr;
        }
    };
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

    std::unordered_map<MaterialPropertyNameHash, PropertyValue> properties_;
};

}
