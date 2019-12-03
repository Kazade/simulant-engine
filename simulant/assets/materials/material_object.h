#pragma once

#include "material_property_registry.h"
#include "../../types.h"

namespace smlt {

class MaterialObject {
public:
    MaterialObject(MaterialPropertyRegistry* registry, MaterialObjectType type=MATERIAL_OBJECT_TYPE_LEAF):
        registry_(registry) {
        registry->register_object(this, type);
    }

    template<typename T>
    void set_property_value(MaterialPropertyID id, const T& value) {
        registry_->set_property_value<T>(this, id, value);
    }

    MaterialPropertyValue* property_value(MaterialPropertyID id) {
        return registry_->property_value(this, id);
    }

    /* Built-in properties */
    void set_specular(const Colour& colour);
    void set_ambient(const Colour& colour);
    void set_diffuse(const Colour& colour);
    void set_shininess(float shininess);
    void set_diffuse_map(TextureID texture_id);
    void set_light_map(TextureID texture_id);
    TextureUnit diffuse_map() const;
    TextureUnit light_map() const;
    TextureUnit normal_map() const;
    TextureUnit specular_map() const;
    Colour specular() const;
    Colour ambient() const;
    Colour diffuse() const;
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

private:
    MaterialPropertyRegistry* registry_ = nullptr;
    int8_t object_id_ = -1;
};

}
