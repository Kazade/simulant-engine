
#include "constants.h"
#include "material_object.h"

namespace smlt {


MaterialObject::MaterialObject(MaterialObject* parent):
    MaterialPropertyOverrider(parent) {

}

MaterialObject::~MaterialObject() {

}

void MaterialObject::set_specular_color(const Color& color) {
    set_property_value(SPECULAR_COLOR_PROPERTY_NAME, (const Vec4&)color);
}

void MaterialObject::set_base_color(const Color& color) {
    set_property_value(BASE_COLOR_PROPERTY_NAME, (const Vec4&)color);
}

void MaterialObject::set_specular(float specular) {
    set_property_value(SPECULAR_PROPERTY_NAME, specular);
}

void MaterialObject::set_base_color_map(TexturePtr texture) {
    set_property_value(BASE_COLOR_MAP_PROPERTY_NAME, texture);
}

void MaterialObject::set_light_map(TexturePtr texture) {
    set_property_value(LIGHT_MAP_PROPERTY_NAME, texture);
}

void MaterialObject::set_metallic(float metallic) {
    set_property_value(METALLIC_PROPERTY_NAME, metallic);
}

void MaterialObject::set_roughness(float roughness) {
    set_property_value(ROUGHNESS_PROPERTY_NAME, roughness);
}

void MaterialObject::set_metallic_roughness_map(TexturePtr texture) {
    set_property_value(METALLIC_ROUGHNESS_MAP_PROPERTY_NAME, texture);
}

void MaterialObject::set_normal_map(TexturePtr texture) {
    set_property_value(NORMAL_MAP_PROPERTY_NAME, texture);
}

void MaterialObject::set_base_color_map_matrix(const Mat4& mat) {
    set_property_value(BASE_COLOR_MAP_MATRIX_PROPERTY_NAME, mat);
}

void MaterialObject::set_light_map_matrix(const Mat4& mat) {
    set_property_value(LIGHT_MAP_MATRIX_PROPERTY_NAME, mat);
}

void MaterialObject::set_normal_map_matrix(const Mat4& mat) {
    set_property_value(NORMAL_MAP_MATRIX_PROPERTY_NAME, mat);
}

void MaterialObject::set_metallic_roughness_map_matrix(const Mat4& mat) {
    set_property_value(METALLIC_ROUGHNESS_MAP_MATRIX_PROPERTY_NAME, mat);
}

const TexturePtr& MaterialObject::base_color_map() const {
    const TexturePtr* ptr = nullptr;
    bool ok = property_value(BASE_COLOR_MAP_PROPERTY_HASH, ptr);
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

const TexturePtr& MaterialObject::metallic_roughness_map() const {
    const TexturePtr* ptr = nullptr;
#ifndef NDEBUG
    bool ok = property_value(METALLIC_ROUGHNESS_MAP_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
#else
    property_value(METALLIC_ROUGHNESS_MAP_PROPERTY_HASH, ptr);
#endif
    return *ptr;
}

const Mat4& MaterialObject::base_color_map_matrix() const {
    const Mat4* ptr = nullptr;
#ifndef NDEBUG
    bool ok = property_value(BASE_COLOR_MAP_MATRIX_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
#else
    property_value(BASE_COLOR_MAP_MATRIX_PROPERTY_HASH, ptr);
#endif
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

const Mat4& MaterialObject::metallic_roughness_map_matrix() const {
    const Mat4* ptr = nullptr;
    bool ok = property_value(METALLIC_ROUGHNESS_MAP_MATRIX_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const Color& MaterialObject::specular_color() const {
    // FIXME: Naughty cast from Vec4& -> Color&
    const Color* ptr = nullptr;
    bool ok = property_value(SPECULAR_COLOR_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

const Color& MaterialObject::base_color() const {
    const Color* ptr = nullptr;
    bool ok = property_value(BASE_COLOR_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

float MaterialObject::metallic() const {
    const float* ptr = nullptr;
    bool ok = property_value(METALLIC_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

float MaterialObject::roughness() const {
    const float* ptr = nullptr;
    bool ok = property_value(ROUGHNESS_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

float MaterialObject::specular() const {
    const float* ptr = nullptr;
    bool ok = property_value(SPECULAR_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

void MaterialObject::set_fog_density(float density) {
    set_property_value(FOG_DENSITY_PROPERTY_NAME, density);
}

void MaterialObject::set_fog_start(float start) {
    set_property_value(FOG_START_PROPERTY_NAME, start);
}

void MaterialObject::set_fog_end(float end) {
    set_property_value(FOG_END_PROPERTY_NAME, end);
}

void MaterialObject::set_fog_mode(FogMode mode) {
    set_property_value(FOG_MODE_PROPERTY_NAME, (EnumType) mode);
}

void MaterialObject::set_fog_color(const Color& color) {
    set_property_value(FOG_COLOR_PROPERTY_NAME, color);
}

float MaterialObject::fog_density() const {
    const float* ptr = nullptr;
    bool ok = property_value(FOG_DENSITY_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

float MaterialObject::fog_start() const {
    const float* ptr = nullptr;
    bool ok = property_value(FOG_START_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

float MaterialObject::fog_end() const {
    const float* ptr = nullptr;
    bool ok = property_value(FOG_END_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

FogMode MaterialObject::fog_mode() const {
    const EnumType* ptr = nullptr;
    bool ok = property_value(FOG_MODE_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *(reinterpret_cast<const FogMode*>(ptr));
}

const Color& MaterialObject::fog_color() const {
    const Color* ptr = nullptr;
    bool ok = property_value(FOG_COLOR_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

bool MaterialObject::is_blending_enabled() const {
    return blend_func() != BLEND_NONE;
}

void MaterialObject::set_blend_func(BlendType b) {
    set_property_value(BLEND_FUNC_PROPERTY_NAME, (EnumType) b);
}

BlendType MaterialObject::blend_func() const {
    const EnumType* ptr = nullptr;
    bool ok = property_value(BLEND_FUNC_PROPERTY_HASH, (const EnumType*&) ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *(reinterpret_cast<const BlendType*>(ptr));
}

void MaterialObject::set_alpha_func(AlphaFunc a) {
    set_property_value(ALPHA_FUNC_PROPERTY_NAME, (EnumType) a);
}

AlphaFunc MaterialObject::alpha_func() const {
    const EnumType* ptr = nullptr;
    bool ok = property_value(ALPHA_FUNC_PROPERTY_HASH, (const EnumType*&) ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *(reinterpret_cast<const AlphaFunc*>(ptr));
}

void MaterialObject::set_alpha_threshold(float v) {
    set_property_value(ALPHA_THRESHOLD_PROPERTY_NAME, v);
}

float MaterialObject::alpha_threshold() const {
    const float* ptr = nullptr;
    bool ok = property_value(ALPHA_FUNC_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *ptr;
}

bool MaterialObject::is_alpha_testing_enabled() const {
    return alpha_func() != ALPHA_FUNC_NONE;
}

void MaterialObject::set_depth_func(DepthFunc b) {
    set_property_value(DEPTH_FUNC_PROPERTY_NAME, (EnumType) b);
}

DepthFunc MaterialObject::depth_func() const {
    const EnumType* ptr = nullptr;
    bool ok = property_value(DEPTH_FUNC_PROPERTY_HASH, (const EnumType*&) ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *(reinterpret_cast<const DepthFunc*>(ptr));
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
    set_property_value(CULL_MODE_PROPERTY_NAME, (EnumType) mode);
}

CullMode MaterialObject::cull_mode() const {
    const EnumType* ptr = nullptr;
    bool ok = property_value(CULL_MODE_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *(reinterpret_cast<const CullMode*>(ptr));
}

void MaterialObject::set_depth_test_enabled(bool v) {
    set_property_value(DEPTH_TEST_ENABLED_PROPERTY_NAME, v);
}

bool MaterialObject::is_depth_test_enabled() const {
    const bool* ptr = nullptr;
#ifndef NDEBUG
    bool ok = property_value(DEPTH_TEST_ENABLED_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
#else
    property_value(DEPTH_TEST_ENABLED_PROPERTY_HASH, ptr);
#endif
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

void MaterialObject::set_textures_enabled(EnabledTextureMask v) {
    set_property_value(TEXTURES_ENABLED_PROPERTY_NAME, v);
}

int32_t MaterialObject::textures_enabled() const {
    const int32_t* ptr = nullptr;
    bool ok = property_value(TEXTURES_ENABLED_PROPERTY_HASH, ptr);
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
    set_property_value(POLYGON_MODE_PROPERTY_NAME, (EnumType) mode);
}

PolygonMode MaterialObject::polygon_mode() const {
    const EnumType* ptr = nullptr;
    bool ok = property_value(POLYGON_MODE_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *(reinterpret_cast<const PolygonMode*>(ptr));
}

void MaterialObject::set_shade_model(ShadeModel model) {
    set_property_value(SHADE_MODEL_PROPERTY_NAME, (EnumType) model);
}

ShadeModel MaterialObject::shade_model() const {
    const EnumType* ptr = nullptr;
    bool ok = property_value(SHADE_MODEL_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *(reinterpret_cast<const ShadeModel*>(ptr));
}

ColorMaterial MaterialObject::color_material() const {
    const EnumType* ptr = nullptr;
    bool ok = property_value(COLOR_MATERIAL_PROPERTY_HASH, ptr);
    assert(ok);
    _S_UNUSED(ok);
    return *(reinterpret_cast<const ColorMaterial*>(ptr));
}

void MaterialObject::set_color_material(ColorMaterial cm) {
    set_property_value(COLOR_MATERIAL_PROPERTY_NAME, (EnumType) cm);
}


}
