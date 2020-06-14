

#include "material_property_registry.h"
#include "constants.h"
#include "material_object.h"

namespace smlt {


MaterialObject::MaterialObject(MaterialPropertyRegistry* registry):
    registry_(registry) {

    if(registry == this) {
        object_id_ = 0;
    } else if(registry) {
        registry->register_object(this);
    }
}

MaterialObject::~MaterialObject() {
    if(registry_ && registry_ != this) {
        registry_->unregister_object(this);
    }
}

void MaterialObject::set_property_value(const MaterialPropertyID &id, const bool &value) {
    registry_->properties_[id - 1].set_value(this, value);
}

void MaterialObject::set_property_value(const MaterialPropertyID &id, const int &value) {
    registry_->properties_[id - 1].set_value(this, value);
}

void MaterialObject::set_property_value(const MaterialPropertyID &id, const float &value) {
    registry_->properties_[id - 1].set_value(this, value);
}

void MaterialObject::set_property_value(const MaterialPropertyID &id, const Vec3 &value) {
    registry_->properties_[id - 1].set_value(this, value);
}

void MaterialObject::set_property_value(const MaterialPropertyID &id, const Vec4 &value) {
    registry_->properties_[id - 1].set_value(this, value);
}

void MaterialObject::set_property_value(const MaterialPropertyID &id, const Mat3 &value) {
    registry_->properties_[id - 1].set_value(this, value);
}

void MaterialObject::set_property_value(const MaterialPropertyID &id, const TextureUnit &value) {
    registry_->properties_[id - 1].set_value(this, value);
}

void MaterialObject::set_property_value(const MaterialPropertyID &id, const Vec2 &value) {
    registry_->properties_[id - 1].set_value(this, value);
}

void MaterialObject::set_property_value(const MaterialPropertyID& id, const TexturePtr& texture) {
    TextureUnit unit(texture);
    set_property_value(id, unit);
}

const MaterialPropertyValue* MaterialObject::property_value(MaterialPropertyID id) const {
    const auto& property = registry_->properties_[id - 1];
    return property.value(this);
}

MaterialPropertyValue* MaterialObject::property_value(MaterialPropertyID id) {
    auto& property = registry_->properties_[id - 1];
    return property.value(this);
}

const MaterialPropertyValue* MaterialObject::property_value(const std::string& name) const {
    return property_value(registry_->find_property_id(name));
}

void MaterialObject::set_specular(const Colour &colour) {
    set_property_value(registry_->material_specular_id_, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void MaterialObject::set_ambient(const Colour &colour) {
    set_property_value(registry_->material_ambient_id_, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void MaterialObject::set_emission(const Colour& colour) {
    set_property_value(registry_->material_emission_id_, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void MaterialObject::set_diffuse(const Colour &colour) {
    set_property_value(registry_->material_diffuse_id_, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void MaterialObject::set_shininess(float shininess) {
    // OpenGL expects shininess to range between 0 and 128 otherwise it throws
    // an invalid_value error
    set_property_value(registry_->material_shininess_id_, clamp(shininess, 0, 128));
}

void MaterialObject::set_diffuse_map(TexturePtr texture) {
    set_property_value(registry_->diffuse_map_id_, texture);
}

void MaterialObject::set_light_map(TexturePtr texture) {
    set_property_value(registry_->light_map_id_, texture);
}

const TextureUnit* MaterialObject::diffuse_map() const {
    return &property_value(registry_->diffuse_map_id_)->value<TextureUnit>();
}

const TextureUnit* MaterialObject::light_map() const {
    return &property_value(registry_->light_map_id_)->value<TextureUnit>();
}

const TextureUnit* MaterialObject::normal_map() const {
    return &property_value(registry_->normal_map_id_)->value<TextureUnit>();
}

const TextureUnit* MaterialObject::specular_map() const {
    return &property_value(registry_->specular_map_id_)->value<TextureUnit>();
}

TextureUnit* MaterialObject::diffuse_map() {
    return &property_value(registry_->diffuse_map_id_)->value<TextureUnit>();
}

TextureUnit* MaterialObject::light_map() {
    return &property_value(registry_->light_map_id_)->value<TextureUnit>();
}

TextureUnit* MaterialObject::normal_map() {
    return &property_value(registry_->normal_map_id_)->value<TextureUnit>();
}

TextureUnit* MaterialObject::specular_map() {
    return &property_value(registry_->specular_map_id_)->value<TextureUnit>();
}

const Colour& MaterialObject::specular() const {
    // FIXME: Naughty cast from Vec4& -> Colour&
    return (const Colour&) property_value(registry_->material_specular_id_)->value<Vec4>();
}

const Colour& MaterialObject::ambient() const {
    return (const Colour&) property_value(registry_->material_ambient_id_)->value<Vec4>();
}

const Colour& MaterialObject::emission() const {
    return (const Colour&) property_value(registry_->material_emission_id_)->value<Vec4>();
}

const Colour& MaterialObject::diffuse() const {
    return (const Colour&) property_value(registry_->material_diffuse_id_)->value<Vec4>();
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
