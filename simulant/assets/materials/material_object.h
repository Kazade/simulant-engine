#pragma once

#include "../../types.h"
#include "material_property.h"
#include "constants.h"

namespace smlt {

class MaterialPropertyRegistry;

class MaterialObject {
public:
    friend class MaterialPropertyRegistry;

    MaterialObject(MaterialPropertyRegistry* registry, MaterialObjectType type=MATERIAL_OBJECT_TYPE_LEAF);
    virtual ~MaterialObject();

    template<typename T>
    void set_property_value(const MaterialPropertyID& id, const T& value);

    void set_property_value(const MaterialPropertyID& id, const TexturePtr& texture);

    /* FIXME: Remove. This is slow and we want to encourage using IDs */
    template<typename T>
    void set_property_value(const std::string& name, const T& value);

    const MaterialPropertyValue* property_value(MaterialPropertyID id) const;

    /* FIXME: Remove, this is slow */
    const MaterialPropertyValue* property_value(const std::string& name) const;

    /* Built-in properties */
    void set_specular(const Colour& colour);
    void set_ambient(const Colour& colour);
    void set_diffuse(const Colour& colour);
    void set_shininess(float shininess);
    void set_diffuse_map(TexturePtr texture);
    void set_light_map(TexturePtr texture);
    const TextureUnit& diffuse_map() const;
    const TextureUnit& light_map() const;
    const TextureUnit& normal_map() const;
    const TextureUnit& specular_map() const;
    const Colour& specular() const;
    const Colour& ambient() const;
    const Colour& diffuse() const;
    float shininess() const;
    bool is_blending_enabled() const;
    void set_blend_func(BlendType b);
    BlendType blend_func() const;
    void set_depth_write_enabled(bool v);
    bool is_depth_write_enabled() const;
    void set_cull_mode(CullMode mode);
    CullMode cull_mode() const;
    void set_depth_test_enabled(bool v);
    bool is_depth_test_enabled() const;
    void set_lighting_enabled(bool v);
    bool is_lighting_enabled() const;
    void set_texturing_enabled(bool v);
    bool is_texturing_enabled() const;
    float point_size() const;
    PolygonMode polygon_mode() const;
    void set_shade_model(ShadeModel model);
    ShadeModel shade_model() const;
    ColourMaterial colour_material() const;
    void set_colour_material(ColourMaterial cm);

    const MaterialPropertyRegistry* registry() const;
private:
    struct MaterialObjectValue {
        /* We resize the array to hold any set values indexed by
         * property id, this will leave gaps in the vector which should be
         * classes as "null" and ignored. That's what this flag is for. */
        bool is_active = false;
        MaterialPropertyValue value;

        MaterialObjectValue(bool active, const MaterialPropertyValue& v):
            is_active(active), value(v) {}
    };

    std::vector<MaterialObjectValue> property_values_;
    MaterialPropertyRegistry* registry_ = nullptr;
    int8_t object_id_ = -1;
};

}

#include "material_property_registry.h"

namespace smlt {

template<typename T>
void MaterialObject::set_property_value(const MaterialPropertyID& id, const T& value) {
    auto index = id - 1;

    if((uint8_t) index >= property_values_.size()) {
        for(auto i = property_values_.size(); i <= (uint8_t) index; ++i) {
            MaterialPropertyValue dummy(registry_, i);
            property_values_.push_back(MaterialObjectValue{false, dummy});
        }

        assert(property_values_.size() == id);
    }

    property_values_[index].is_active = true;
    property_values_[index].value.variant_.set(value);
}

template<typename T>
void MaterialObject::set_property_value(const std::string& name, const T& value) {
    set_property_value(registry_->find_property_id(name), value);
}


}
