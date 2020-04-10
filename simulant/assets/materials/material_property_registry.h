#pragma once

#include "material_property.h"
#include "constants.h"
#include "material_object.h"

namespace smlt {

BlendType blend_type_from_name(const std::string& v);

const static int MAX_DEFINED_PROPERTIES = 64;

class GenericRenderer;

class MaterialPropertyRegistry:
    public MaterialObject {

public:
    friend class MaterialObject;
    friend class MaterialPass;
    friend class Material;

    /* This is required so that the renderer
     * can access the built-in IDs for performance */
    friend class GenericRenderer;

    MaterialPropertyRegistry();
    MaterialPropertyRegistry(const MaterialPropertyRegistry& rhs) = delete;
    MaterialPropertyRegistry& operator=(const MaterialPropertyRegistry& rhs) = delete;

    virtual ~MaterialPropertyRegistry();

    template<typename T>
    MaterialPropertyID register_property(
        MaterialPropertyType type,
        std::string name,
        const T& default_value
    );

    /* Property IDs should be the primary way to lookup things for performance
     * reasons, but this will allow translation from a name to and ID */
    MaterialPropertyID find_property_id(const std::string& name) const;

    /* Property ids are one-based */
    const MaterialProperty* property(MaterialPropertyID id) const;

    const std::vector<MaterialProperty*> texture_properties() const {
        return texture_properties_;
    }

    const std::vector<MaterialProperty*> custom_properties() const {
        return custom_properties_;
    }

    std::size_t registered_material_object_count() const;

protected:
    void initialize_free_object_ids() {
        free_object_ids_.clear();
        for(uint8_t i = 1u; i < MAX_MATERIAL_PASSES + 1; ++i) {
            free_object_ids_.push_back(i);
        }
    }

    void clear_registered_objects() {
        for(auto& obj: registered_objects_) {
            obj = nullptr;
        }
    }
private:
    /* A list of properties, these are indexed by MateralPropertyID minus 1 (as IDs
     * are 1-indexed) */
    std::vector<MaterialProperty> properties_;
    std::vector<MaterialProperty*> texture_properties_;

    void rebuild_texture_properties() {
        texture_properties_.clear();
        for(auto& prop: properties_) {
            if(prop.type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
                texture_properties_.push_back(&prop);
            }
        }
    }

    std::vector<MaterialProperty*> custom_properties_;
    void rebuild_custom_properties() {
        custom_properties_.clear();
        for(auto& prop: properties_) {
            if(prop.is_custom) {
                custom_properties_.push_back(&prop);
            }
        }
    }

    /* These values are indexed by MaterialObject::object_id which is more performant
     * than an unordered_map */
    MaterialObject* registered_objects_[MAX_MATERIAL_PASSES + 1];

    MaterialPropertyID material_ambient_id_;
    MaterialPropertyID material_diffuse_id_;
    MaterialPropertyID material_specular_id_;
    MaterialPropertyID material_shininess_id_;
    MaterialPropertyID diffuse_map_id_;
    MaterialPropertyID specular_map_id_;
    MaterialPropertyID light_map_id_;
    MaterialPropertyID normal_map_id_;

    MaterialPropertyID blending_enabled_id_;
    MaterialPropertyID texturing_enabled_id_;
    MaterialPropertyID lighting_enabled_id_;
    MaterialPropertyID depth_test_enabled_id_;
    MaterialPropertyID depth_write_enabled_id_;

    MaterialPropertyID shade_model_id_;
    MaterialPropertyID cull_mode_id_;
    MaterialPropertyID polygon_mode_id_;
    MaterialPropertyID point_size_id_;
    MaterialPropertyID colour_material_id_;
    MaterialPropertyID blend_func_id_;

    void register_all_builtin_properties();

    template<typename T>
    MaterialPropertyID register_builtin_property(
        MaterialPropertyType type,
        std::string name,
        const T& default_value
    ) {
        auto ret = register_property(type, name, default_value);
        properties_[ret - 1].is_custom = false;
        rebuild_custom_properties();
        return ret;
    }

    /* Register the MaterialObject. All this does is store the root MaterialObject
     * if it's a root type, and sets the MaterialObject::object_id_ variable */
    void register_object(MaterialObject* obj);
    void unregister_object(MaterialObject* obj);

    std::vector<uint8_t> free_object_ids_;
};

}

#include "material_property.inl.h"

namespace smlt {
template<typename T>
MaterialPropertyID MaterialPropertyRegistry::register_property(
    MaterialPropertyType type,
    std::string name,
    const T& default_value
) {
    MaterialProperty prop(properties_.size() + 1);
    prop.name = name;
    prop.type = type;

    /* Make sure we init entries for all registered objects */
    for(auto object: registered_objects_) {
        if(object && object->object_id()) prop.init_entry(object);
    }

    /* We set the default value in the registry slot (0)
     * in the material property entries */
    prop.set_value(this, default_value);
    properties_.push_back(prop);

    // We keep a list of texture properties as we need
    // to iterate them in the renderer and we need it
    // to be fast!
    rebuild_texture_properties();
    rebuild_custom_properties();

    return prop.id;
}
}

