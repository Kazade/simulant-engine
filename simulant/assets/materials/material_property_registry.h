#pragma once

#include "material_property.h"

namespace smlt {

enum MaterialObjectType {
    MATERIAL_OBJECT_TYPE_ROOT,
    MATERIAL_OBJECT_TYPE_LEAF
};

enum BlendType {
    BLEND_NONE,
    BLEND_ADD,
    BLEND_MODULATE,
    BLEND_COLOUR,
    BLEND_ALPHA,
    BLEND_ONE_ONE_MINUS_ALPHA
};

BlendType blend_type_from_name(const std::string& v);

const static int MAX_PASSES = 4;
const static int MAX_DEFINED_PROPERTIES = 64;

class MaterialObject;

class MaterialPropertyRegistry {
public:
    MaterialPropertyRegistry() {
        object_values_.resize(1); // Always make room for the root object
    }

    template<typename T>
    MaterialPropertyID register_property(
        MaterialPropertyType type,
        std::string name,
        const T& default_value
    ) {
        MaterialProperty prop;
        prop.id = properties_.size() + 1;
        prop.name = name;
        prop.type = type;
        prop.default_value.set(default_value);

        properties_.push_back(new_p);

        return new_p.id;

    }

    /* Property IDs should be the primary way to lookup things for performance
     * reasons, but this will allow translation from a name to and ID */
    MaterialPropertyID find_property_id(const std::string& name);

    /* Property ids are one-based */
    MaterialProperty* property(MaterialPropertyID id);

private:
    /* The first time a MaterialObject is registered as a root type, this is
     * set. In debug mode an assertion will raise if you do that again, in non
     * debug mode the root_ will be replaced. Don't do that */
    MaterialObject* root_ = nullptr;

    /* A list of properties, these are indexed by MateralPropertyID minus 1 (as IDs
     * are 1-indexed) */
    std::vector<MaterialProperty> properties_;

    struct MaterialObjectValues {
        bool is_active = true; // Used when unregistering MaterialObjects to prevent shifting in the array
        std::unordered_map<MaterialPropertyID, MaterialPropertyValue> values;
    };

    /* These values are indexed by MaterialObject::object_id which is more performant
     * than an unordered_map. FIXME: Should values be moved onto the MaterialObject for
     * cache locality? */
    std::vector<MaterialObjectValues> object_values_;

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
        return ret;
    }

    /* Register the MaterialObject. All this does is store the root MaterialObject
     * if it's a root type, and sets the MaterialObject::object_id_ variable */
    void register_object(MaterialObject* obj, MaterialObjectType type);


    template<typename T>
    void set_property_value(MaterialObject* obj, MaterialPropertyID id, const T& value) {
        assert(obj->object_id < object_values_.size());
        auto& values = object_values_[obj->object_id];
        assert(values.is_active);
        values.values[id].set(value);
    }

    MaterialPropertyValue* property_value(MaterialObject* obj, MaterialPropertyID id);
};


}
