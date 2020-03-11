#pragma once

#include "../../types.h"
#include "constants.h"

namespace smlt {

class MaterialPropertyRegistry;
class MaterialPropertyValue;

class MaterialObject {
public:
    friend class Material;
    friend struct MaterialProperty;
    friend class MaterialPropertyRegistry;

    MaterialObject(MaterialPropertyRegistry* registry);
    MaterialObject(const MaterialObject&) = delete;
    MaterialObject& operator=(MaterialObject&) = delete;

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

    int8_t object_id() const { return object_id_; }
private:

    MaterialPropertyRegistry* registry_ = nullptr;
    int8_t object_id_ = -1;
};

}

