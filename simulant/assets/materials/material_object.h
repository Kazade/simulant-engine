#pragma once

#include "../../types.h"
#include "constants.h"
#include "core/material_property_overrider.h"

namespace smlt {

class MaterialPropertyRegistry;
class MaterialPropertyValue;

class MaterialObject : public MaterialPropertyOverrider {
public:
    friend class Material;

    MaterialObject():
        MaterialPropertyOverrider(nullptr) {}

    MaterialObject(MaterialObject* parent);
    virtual ~MaterialObject();

    /* Built-in properties */
    void set_specular(const Colour& colour);
    void set_ambient(const Colour& colour);
    void set_diffuse(const Colour& colour);
    void set_emission(const Colour& colour);
    void set_shininess(float shininess);
    void set_diffuse_map(TexturePtr texture);
    void set_light_map(TexturePtr texture);

    const Mat4& diffuse_map_matrix() const;
    const Mat4& light_map_matrix() const;
    const Mat4& normal_map_matrix() const;
    const Mat4& specular_map_matrix() const;

    void set_diffuse_map_matrix(const Mat4& mat);
    void set_light_map_matrix(const Mat4& mat);
    void set_normal_map_matrix(const Mat4& mat);
    void set_specular_map_matrix(const Mat4& mat);

    const TexturePtr& diffuse_map() const;
    const TexturePtr& light_map() const;
    const TexturePtr& normal_map() const;
    const TexturePtr& specular_map() const;

    void set_fog_density(float density);
    void set_fog_start(float start);
    void set_fog_end(float end);
    void set_fog_mode(FogMode mode);
    void set_fog_colour(const Colour& colour);

    float fog_density() const;
    float fog_start() const;
    float fog_end() const;
    FogMode fog_mode() const;
    const Colour& fog_colour() const;

    const Colour& specular() const;
    const Colour& ambient() const;
    const Colour& emission() const;
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
    void set_textures_enabled(EnabledTextureMask v);
    int32_t textures_enabled() const;
    float point_size() const;

    void set_polygon_mode(PolygonMode mode);
    PolygonMode polygon_mode() const;

    void set_shade_model(ShadeModel model);
    ShadeModel shade_model() const;

    ColourMaterial colour_material() const;
    void set_colour_material(ColourMaterial cm);

    const MaterialObject* parent_material_object() const {
        return dynamic_cast<const MaterialObject*>(this->parent_);
    }

};

}

