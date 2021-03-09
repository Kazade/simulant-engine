
#include "constants.h"
#include "material_object.h"

namespace smlt {


MaterialObject::MaterialObject(MaterialObject* parent):
    MaterialPropertyOverrider(parent) {

}

MaterialObject::~MaterialObject() {

}

void MaterialObject::set_specular(const Colour &colour) {
    set_property_value(SPECULAR_PROPERTY_NAME, (const Vec4&) colour);
}

void MaterialObject::set_ambient(const Colour &colour) {
    set_property_value(AMBIENT_PROPERTY_NAME, (const Vec4&) colour);
}

void MaterialObject::set_emission(const Colour& colour) {
    set_property_value(EMISSION_PROPERTY_NAME, (const Vec4&) colour);
}

void MaterialObject::set_diffuse(const Colour &colour) {
    set_property_value(DIFFUSE_PROPERTY_NAME, (const Vec4&) colour);
}

void MaterialObject::set_shininess(float shininess) {
    // OpenGL expects shininess to range between 0 and 128 otherwise it throws
    // an invalid_value error
    set_property_value(SHININESS_PROPERTY_NAME, clamp(shininess, 0, 128));
}

void MaterialObject::set_diffuse_map(TexturePtr texture) {
    set_property_value(DIFFUSE_MAP_PROPERTY_NAME, texture);
}

void MaterialObject::set_light_map(TexturePtr texture) {
    set_property_value(LIGHT_MAP_PROPERTY_NAME, texture);
}

void MaterialObject::set_diffuse_map_matrix(const Mat4& mat) {
    set_property_value(DIFFUSE_MAP_MATRIX_PROPERTY_NAME, mat);
}

void MaterialObject::set_light_map_matrix(const Mat4& mat) {
    set_property_value(LIGHT_MAP_MATRIX_PROPERTY_NAME, mat);
}

void MaterialObject::set_normal_map_matrix(const Mat4& mat) {
    set_property_value(NORMAL_MAP_MATRIX_PROPERTY_NAME, mat);
}

void MaterialObject::set_specular_map_matrix(const Mat4& mat) {
    set_property_value(SPECULAR_MAP_MATRIX_PROPERTY_NAME, mat);
}

const TexturePtr& MaterialObject::diffuse_map() const {
    const TexturePtr* ptr = nullptr;
    bool ok = property_value(DIFFUSE_MAP_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const TexturePtr& MaterialObject::light_map() const {
    const TexturePtr* ptr = nullptr;
    bool ok = property_value(LIGHT_MAP_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const TexturePtr& MaterialObject::normal_map() const {
    const TexturePtr* ptr = nullptr;
    bool ok = property_value(NORMAL_MAP_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const TexturePtr& MaterialObject::specular_map() const {
    const TexturePtr* ptr = nullptr;
    bool ok = property_value(SPECULAR_MAP_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const Mat4& MaterialObject::diffuse_map_matrix() const {
    const Mat4* ptr = nullptr;
    bool ok = property_value(DIFFUSE_MAP_MATRIX_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const Mat4& MaterialObject::light_map_matrix() const {
    const Mat4* ptr = nullptr;
    bool ok = property_value(LIGHT_MAP_MATRIX_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const Mat4& MaterialObject::normal_map_matrix() const {
    const Mat4* ptr = nullptr;
    bool ok = property_value(NORMAL_MAP_MATRIX_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const Mat4& MaterialObject::specular_map_matrix() const {
    const Mat4* ptr = nullptr;
    bool ok = property_value(SPECULAR_MAP_MATRIX_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}


const Colour& MaterialObject::specular() const {
    // FIXME: Naughty cast from Vec4& -> Colour&
    const Colour* ptr = nullptr;
    bool ok = property_value(SPECULAR_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const Colour& MaterialObject::ambient() const {
    const Colour* ptr = nullptr;
    bool ok = property_value(AMBIENT_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const Colour& MaterialObject::emission() const {
    const Colour* ptr = nullptr;
    bool ok = property_value(EMISSION_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const Colour& MaterialObject::diffuse() const {
    const Colour* ptr = nullptr;
    bool ok = property_value(DIFFUSE_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

float MaterialObject::shininess() const {
    const float* ptr = nullptr;
    bool ok = property_value(SHININESS_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

bool MaterialObject::is_blending_enabled() const {
    return blend_func() != BLEND_NONE;
}

void MaterialObject::set_blend_func(BlendType b) {
    set_property_value(BLEND_FUNC_PROPERTY_NAME, (const EnumType&) b);
}

BlendType MaterialObject::blend_func() const {
    const BlendType* ptr = nullptr;
    bool ok = property_value(BLEND_FUNC_PROPERTY_HASH, (const EnumType*&) ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

void MaterialObject::set_depth_write_enabled(bool v) {
    set_property_value(DEPTH_WRITE_ENABLED_PROPERTY_NAME, v);
}

bool MaterialObject::is_depth_write_enabled() const {
    const bool* ptr = nullptr;
    bool ok = property_value(DEPTH_WRITE_ENABLED_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

void MaterialObject::set_cull_mode(CullMode mode) {
    set_property_value(CULL_MODE_PROPERTY_NAME, (const EnumType&) mode);
}

CullMode MaterialObject::cull_mode() const {
    const CullMode* ptr = nullptr;
    bool ok = property_value(CULL_MODE_PROPERTY_HASH, (const EnumType*&) ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

void MaterialObject::set_depth_test_enabled(bool v) {
    set_property_value(DEPTH_TEST_ENABLED_PROPERTY_NAME, v);
}

bool MaterialObject::is_depth_test_enabled() const {
    const bool* ptr = nullptr;
    bool ok = property_value(DEPTH_TEST_ENABLED_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

void MaterialObject::set_lighting_enabled(bool v) {
    set_property_value(LIGHTING_ENABLED_PROPERTY_NAME, v);
}

bool MaterialObject::is_lighting_enabled() const {
    const bool* ptr = nullptr;
    bool ok = property_value(LIGHTING_ENABLED_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

void MaterialObject::set_texturing_enabled(bool v) {
    set_property_value(TEXTURING_ENABLED_PROPERTY_NAME, v);
}

bool MaterialObject::is_texturing_enabled() const {
    const bool* ptr = nullptr;
    bool ok = property_value(TEXTURING_ENABLED_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

float MaterialObject::point_size() const {
    const float* ptr = nullptr;
    bool ok = property_value(POINT_SIZE_PROPERTY_NAME, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

void MaterialObject::set_polygon_mode(PolygonMode mode) {
    set_property_value(POLYGON_MODE_PROPERTY_NAME, (const EnumType&) mode);
}

PolygonMode MaterialObject::polygon_mode() const {
    const PolygonMode* ptr = nullptr;
    bool ok = property_value(POLYGON_MODE_PROPERTY_HASH, (const EnumType*&) ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

void MaterialObject::set_shade_model(ShadeModel model) {
    set_property_value(SHADE_MODEL_PROPERTY_NAME, (const EnumType&) model);
}

ShadeModel MaterialObject::shade_model() const {
    const ShadeModel* ptr = nullptr;
    bool ok = property_value(SHADE_MODEL_PROPERTY_HASH, (const EnumType*&) ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

ColourMaterial MaterialObject::colour_material() const {
    const ColourMaterial* ptr = nullptr;
    bool ok = property_value(COLOUR_MATERIAL_PROPERTY_HASH, (const EnumType*&) ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

void MaterialObject::set_colour_material(ColourMaterial cm) {
    set_property_value(COLOUR_MATERIAL_PROPERTY_NAME, (const EnumType&) cm);
}


}
