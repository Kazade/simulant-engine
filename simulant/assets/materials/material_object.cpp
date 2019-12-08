
#include "material_object.h"
#include "constants.h"

namespace smlt {

MaterialObject::MaterialObject(MaterialPropertyRegistry* registry, MaterialObjectType type):
    registry_(registry) {

    registry->register_object(this, type);
}

MaterialObject::~MaterialObject() {
    registry_->unregister_object(this);
}

void MaterialObject::set_property_value(const MaterialPropertyID& id, const TexturePtr& texture) {
    TextureUnit unit(texture);
    set_property_value(id, unit);
}

const MaterialPropertyValue* MaterialObject::property_value(MaterialPropertyID id) const {
    auto index = id - 1;
    if((uint8_t) index < property_values_.size() && property_values_[index].is_active) {
        return &property_values_[index].value;
    } else if(registry_->root_ == this) {
        /* If the root is this, and we have no value, then fallback to whatever
         * the specified default was */
        return &registry_->property(id)->default_value;
    } else {
        assert(this != registry_->root_);
        return registry_->root_->property_value(id);
    }
}

const MaterialPropertyValue* MaterialObject::property_value(const std::string& name) const {
    return property_value(registry_->find_property_id(name));
}

void MaterialObject::set_specular(const Colour &colour) {
    set_property_value(SPECULAR_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void MaterialObject::set_ambient(const Colour &colour) {
    set_property_value(AMBIENT_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void MaterialObject::set_diffuse(const Colour &colour) {
    set_property_value(DIFFUSE_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void MaterialObject::set_shininess(float shininess) {
    // OpenGL expects shininess to range between 0 and 128 otherwise it throws
    // an invalid_value error
    set_property_value(SHININESS_PROPERTY, clamp(shininess, 0, 128));
}

void MaterialObject::set_diffuse_map(TexturePtr texture) {
    set_property_value(registry_->diffuse_map_id_, texture);
}

void MaterialObject::set_light_map(TexturePtr texture) {
    set_property_value(registry_->light_map_id_, texture);
}

TextureUnit MaterialObject::diffuse_map() const {
    return property_value(registry_->diffuse_map_id_)->value<TextureUnit>();
}

TextureUnit MaterialObject::light_map() const {
    return property_value(registry_->light_map_id_)->value<TextureUnit>();
}

TextureUnit MaterialObject::normal_map() const {
    return property_value(registry_->normal_map_id_)->value<TextureUnit>();
}

TextureUnit MaterialObject::specular_map() const {
    return property_value(registry_->specular_map_id_)->value<TextureUnit>();
}

Colour MaterialObject::specular() const {
    auto v = property_value(registry_->material_specular_id_)->value<Vec4>();
    return Colour(v.x, v.y, v.z, v.w);
}

Colour MaterialObject::ambient() const {
    auto v = property_value(registry_->material_ambient_id_)->value<Vec4>();
    return Colour(v.x, v.y, v.z, v.w);
}

Colour MaterialObject::diffuse() const {
    auto v = property_value(registry_->material_diffuse_id_)->value<Vec4>();
    return Colour(v.x, v.y, v.z, v.w);
}

float MaterialObject::shininess() const {
    return property_value(registry_->material_shininess_id_)->value<float>();
}

bool MaterialObject::is_blending_enabled() const {
    return (BlendType) property_value(registry_->blend_func_id_)->value<int>() == BLEND_NONE;
}

void MaterialObject::set_blend_func(BlendType b) {
    set_property_value(registry_->blend_func_id_, (int) b);
}

BlendType MaterialObject::blend_func() const {
    return (BlendType) property_value(registry_->blend_func_id_)->value<int>();
}

void MaterialObject::set_depth_write_enabled(bool v) {
    set_property_value(registry_->depth_write_enabled_id_, v);
}

bool MaterialObject::is_depth_write_enabled() const {
    return property_value(registry_->depth_write_enabled_id_)->value<bool>();
}

void MaterialObject::set_cull_mode(CullMode mode) {
    set_property_value(registry_->cull_mode_id_, (int) mode);
}

CullMode MaterialObject::cull_mode() const {
    return (CullMode) property_value(registry_->cull_mode_id_)->value<int>();
}

void MaterialObject::set_depth_test_enabled(bool v) {
    set_property_value(registry_->depth_test_enabled_id_, v);
}

bool MaterialObject::is_depth_test_enabled() const {
    return property_value(registry_->depth_test_enabled_id_)->value<bool>();
}

void MaterialObject::set_lighting_enabled(bool v) {
    set_property_value(registry_->lighting_enabled_id_, v);
}

bool MaterialObject::is_lighting_enabled() const {
    return property_value(registry_->lighting_enabled_id_)->value<bool>();
}

void MaterialObject::set_texturing_enabled(bool v) {
    set_property_value(registry_->texturing_enabled_id_, v);
}

bool MaterialObject::is_texturing_enabled() const {
    return property_value(registry_->texturing_enabled_id_)->value<bool>();
}

float MaterialObject::point_size() const {
    return property_value(registry_->point_size_id_)->value<float>();
}

PolygonMode MaterialObject::polygon_mode() const {
    return (PolygonMode) property_value(registry_->polygon_mode_id_)->value<int>();
}

void MaterialObject::set_shade_model(ShadeModel model) {
    set_property_value(registry_->shade_model_id_, (int) model);
}

ShadeModel MaterialObject::shade_model() const {
    return (ShadeModel) property_value(registry_->shade_model_id_)->value<int>();
}

ColourMaterial MaterialObject::colour_material() const {
    return (ColourMaterial) property_value(registry_->colour_material_id_)->value<int>();
}

void MaterialObject::set_colour_material(ColourMaterial cm) {
    set_property_value(registry_->colour_material_id_, (int) cm);
}

const MaterialPropertyRegistry* MaterialObject::registry() const {
    return registry_;
}

}
