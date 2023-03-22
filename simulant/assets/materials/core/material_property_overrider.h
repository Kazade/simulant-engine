#pragma once

#include <unordered_map>
#include <list>
#include <string>

#include "../property_value.h"
#include "core_material.h"
#include "../../../generic/containers/contiguous_map.h"

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

        properties_.insert(std::make_pair(hsh, PropertyValue<void>(value)));
        on_override(hsh, name, _impl::material_property_lookup<T>::type);
    }

    void set_property_value(const char* name, const Colour& value) {
        set_property_value(name, (const Vec4&) value);
    }

    template<typename T>
    bool property_value(const MaterialPropertyNameHash hsh, const T*& out) const {
        /* Core property fast-path. If it's a core property, check locally
         * then check the parent without doing a hash lookup on the properties
         * list (which is for non-core properties) */
        if(is_core_property(hsh)) {
            auto v = find_core_property(hsh);
            if(v) {
                out = v->get<T>();
                return true;
            } else {
                if(parent_) {
                    return parent_->property_value(hsh, out);
                } else {
                    return core_material_property_value(hsh, out);
                }
            }
        } else {
            auto it = properties_.find(hsh);
            if(it != properties_.end()) {
                out = it->second.get<T>();
                return true;
            } else if(parent_) {
                return parent_->property_value(hsh, out);
            }
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
    /* It would be nice if we could treat core properties and
     * custom properties identically. Unfortunately that leads to
     * poor performance due to lack of cache locality and the
     * hash table lookups. So instead we keep a list of core properties
     * directly embedded in the material and use a massive switch
     * statement to find them. */
    PropertyValue<Vec4> diffuse_property_;
    PropertyValue<Vec4> ambient_property_;
    PropertyValue<Vec4> emission_property_;
    PropertyValue<Vec4> specular_property_;
    PropertyValue<float> shininess_property_;
    PropertyValue<float> point_size_property_;
    PropertyValue<bool> depth_write_enabled_property_;
    PropertyValue<bool> depth_test_enabled_property_;
    PropertyValue<bool> lighting_enabled_property_;
    PropertyValue<bool> textures_enabled_property_;
    PropertyValue<TexturePtr> diffuse_map_property_;
    PropertyValue<TexturePtr> specular_map_property_;
    PropertyValue<TexturePtr> light_map_property_;
    PropertyValue<TexturePtr> normal_map_property_;
    PropertyValue<Mat4> diffuse_map_matrix_property_;
    PropertyValue<Mat4> specular_map_matrix_property_;
    PropertyValue<Mat4> light_map_matrix_property_;
    PropertyValue<Mat4> normal_map_matrix_property_;
    PropertyValue<int32_t> blend_func_property_;
    PropertyValue<int32_t> polygon_mode_property_;
    PropertyValue<int32_t> shade_model_property_;
    PropertyValue<bool> colour_material_property_;
    PropertyValue<int32_t> cull_mode_property_;
    PropertyValue<Vec4> fog_colour_property_;
    PropertyValue<float> fog_density_property_;
    PropertyValue<float> fog_start_property_;
    PropertyValue<float> fog_end_property_;
    PropertyValue<int32_t> fog_mode_property_;

    const BasePropertyValue* find_core_property(const MaterialPropertyNameHash hsh) const {
        switch(hsh) {
            case DIFFUSE_PROPERTY_HASH:
                return diffuse_property_.has_value() ? &diffuse_property_ : nullptr;
            case AMBIENT_PROPERTY_HASH:
                return ambient_property_.has_value() ? &ambient_property_ : nullptr;
            case EMISSION_PROPERTY_HASH:
                return emission_property_.has_value() ? &emission_property_ : nullptr;
            case SPECULAR_PROPERTY_HASH:
                return specular_property_.has_value() ? &specular_property_ : nullptr;
            case SHININESS_PROPERTY_HASH:
                return shininess_property_.has_value() ? &shininess_property_ : nullptr;
            case POINT_SIZE_PROPERTY_HASH:
                return point_size_property_.has_value() ? &point_size_property_ : nullptr;
            case DEPTH_WRITE_ENABLED_PROPERTY_HASH:
                return depth_write_enabled_property_.has_value() ? &depth_write_enabled_property_ : nullptr;
            case DEPTH_TEST_ENABLED_PROPERTY_HASH:
                return depth_test_enabled_property_.has_value() ? &depth_test_enabled_property_ : nullptr;
            case LIGHTING_ENABLED_PROPERTY_HASH:
                return lighting_enabled_property_.has_value() ? &lighting_enabled_property_ : nullptr;
            case TEXTURES_ENABLED_PROPERTY_HASH:
                return textures_enabled_property_.has_value() ? &textures_enabled_property_ : nullptr;
            case DIFFUSE_MAP_PROPERTY_HASH:
                return diffuse_map_property_.has_value() ? &diffuse_map_property_ : nullptr;
            case SPECULAR_MAP_PROPERTY_HASH:
                return specular_map_property_.has_value() ? &specular_map_property_ : nullptr;
            case LIGHT_MAP_PROPERTY_HASH:
                return light_map_property_.has_value() ? &light_map_property_ : nullptr;
            case NORMAL_MAP_PROPERTY_HASH:
                return normal_map_property_.has_value() ? &normal_map_property_ : nullptr;
            case DIFFUSE_MAP_MATRIX_PROPERTY_HASH:
                return diffuse_map_matrix_property_.has_value() ? &diffuse_map_matrix_property_ : nullptr;
            case SPECULAR_MAP_MATRIX_PROPERTY_HASH:
                return specular_map_matrix_property_.has_value() ? &specular_map_matrix_property_ : nullptr;
            case LIGHT_MAP_MATRIX_PROPERTY_HASH:
                return light_map_matrix_property_.has_value() ? &light_map_matrix_property_ : nullptr;
            case NORMAL_MAP_MATRIX_PROPERTY_HASH:
                return normal_map_matrix_property_.has_value() ? &normal_map_matrix_property_ : nullptr;
            case BLEND_FUNC_PROPERTY_HASH:
                return blend_func_property_.has_value() ? &blend_func_property_ : nullptr;
            case POLYGON_MODE_PROPERTY_HASH:
                return polygon_mode_property_.has_value() ? &polygon_mode_property_ : nullptr;
            case SHADE_MODEL_PROPERTY_HASH:
                return shade_model_property_.has_value() ? &shade_model_property_ : nullptr;
            case COLOUR_MATERIAL_PROPERTY_HASH:
                return colour_material_property_.has_value() ? &colour_material_property_ : nullptr;
            case CULL_MODE_PROPERTY_HASH:
                return cull_mode_property_.has_value() ? &cull_mode_property_ : nullptr;
            case FOG_COLOUR_PROPERTY_HASH:
                return fog_colour_property_.has_value() ? &fog_colour_property_ : nullptr;
            case FOG_DENSITY_PROPERTY_HASH:
                return fog_density_property_.has_value() ? &fog_density_property_ : nullptr;
            case FOG_START_PROPERTY_HASH:
                return fog_start_property_.has_value() ? &fog_start_property_ : nullptr;
            case FOG_END_PROPERTY_HASH:
                return fog_end_property_.has_value() ? &fog_end_property_ : nullptr;
            case FOG_MODE_PROPERTY_HASH:
                return fog_mode_property_.has_value() ? &fog_mode_property_ : nullptr;
            default:
                return nullptr;
        }
    }


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

    std::unordered_map<MaterialPropertyNameHash, PropertyValue<void>> properties_;
};

}
