
#include "constants.h"
#include "material_object.h"

namespace smlt {


MaterialObject::MaterialObject(MaterialObject* parent):
    MaterialPropertyOverrider(parent) {

}

MaterialObject::~MaterialObject() {

}

void MaterialObject::set_specular(const Colour &colour) {
    override_property_value(SPECULAR_PROPERTY, (const Vec4&) colour);
}

void MaterialObject::set_ambient(const Colour &colour) {
    override_property_value(AMBIENT_PROPERTY, (const Vec4&) colour);
}

void MaterialObject::set_emission(const Colour& colour) {
    override_property_value(EMISSION_PROPERTY, (const Vec4&) colour);
}

void MaterialObject::set_diffuse(const Colour &colour) {
    override_property_value(DIFFUSE_PROPERTY, (const Vec4&) colour);
}

void MaterialObject::set_shininess(float shininess) {
    // OpenGL expects shininess to range between 0 and 128 otherwise it throws
    // an invalid_value error
    override_property_value(SHININESS_PROPERTY, clamp(shininess, 0, 128));
}

void MaterialObject::set_diffuse_map(TexturePtr texture) {
    override_property_value(DIFFUSE_MAP_PROPERTY, texture);
}

void MaterialObject::set_light_map(TexturePtr texture) {
    override_property_value(LIGHT_MAP_PROPERTY, texture);
}

const TextureUnit* MaterialObject::diffuse_map() const {
    return nullptr;
}

const TextureUnit* MaterialObject::light_map() const {
    return nullptr;
}

const TextureUnit* MaterialObject::normal_map() const {
    return nullptr;
}

const TextureUnit* MaterialObject::specular_map() const {
    return nullptr;
}

TextureUnit* MaterialObject::diffuse_map() {
    return nullptr;
}

TextureUnit* MaterialObject::light_map() {
    return nullptr;
}

TextureUnit* MaterialObject::normal_map() {
    return nullptr;
}

TextureUnit* MaterialObject::specular_map() {
    return nullptr;
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
    float f;
    fetch_property_value(POINT_SIZE_PROPERTY, &f);
    return f;
}

void MaterialObject::set_polygon_mode(PolygonMode mode) {
    set_property_value(registry_->polygon_mode_id_, (int) mode);
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
