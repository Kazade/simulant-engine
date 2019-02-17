//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdexcept>
#include <cassert>
#include <map>

#include "window.h"
#include "material.h"
#include "asset_manager.h"
#include "renderers/renderer.h"
#include "renderers/gl2x/gpu_program.h"


namespace smlt {

const std::string Material::BuiltIns::DEFAULT = "simulant/materials/${RENDERER}/default.smat";
const std::string Material::BuiltIns::TEXTURE_ONLY = "simulant/materials/${RENDERER}/texture_only.smat";
const std::string Material::BuiltIns::DIFFUSE_ONLY = "simulant/materials/${RENDERER}/diffuse_only.smat";

/* This list is used by the particle script loader to determine if a specified material
 * is a built-in or not. Please keep this up-to-date when changing the above materials!
 */
const std::unordered_map<std::string, std::string> Material::BUILT_IN_NAMES = {
    {"DEFAULT", Material::BuiltIns::DEFAULT},
    {"TEXTURE_ONLY", Material::BuiltIns::TEXTURE_ONLY},
    {"DIFFUSE_ONLY", Material::BuiltIns::DIFFUSE_ONLY},
};

Material::Material(MaterialID id, AssetManager* asset_manager):
    Resource(asset_manager),
    generic::Identifiable<MaterialID>(id),
    _material_impl::PropertyValueHolder(this, 0),
    passes_{MaterialPass(this, 0), MaterialPass(this, 1), MaterialPass(this, 2), MaterialPass(this, 3)} {

    initialize_default_properties();
    set_pass_count(1);  // Enable a single pass by default otherwise the material is useless
}

Material::Material(const Material& rhs):
    Resource(rhs),
    generic::Identifiable<MaterialID>(rhs),
    Managed<Material>(rhs),
    _material_impl::PropertyValueHolder(rhs),
    defined_property_count_(rhs.defined_property_count_),
    defined_properties_(rhs.defined_properties_),
    defined_property_lookup_(rhs.defined_property_lookup_),
    pass_count_(rhs.pass_count_),
    passes_(rhs.passes_) {

    top_level_ = this;

    material_ambient_index_ = rhs.material_ambient_index_;
    material_diffuse_index_ = rhs.material_diffuse_index_;
    material_shininess_index_ = rhs.material_shininess_index_;
    material_specular_index_ = rhs.material_specular_index_;
    diffuse_map_index_ = rhs.diffuse_map_index_;
    specular_map_index_ = rhs.specular_map_index_;
    normal_map_index_ = rhs.normal_map_index_;
    light_map_index_ = rhs.light_map_index_;

    lighting_enabled_index_ = rhs.lighting_enabled_index_;
    texturing_enabled_index_ = rhs.texturing_enabled_index_;
    depth_test_enabled_index_ = rhs.depth_test_enabled_index_;
    depth_write_enabled_index_ = rhs.depth_write_enabled_index_;

    shade_model_index_ = rhs.shade_model_index_;
    cull_mode_index_ = rhs.cull_mode_index_;
    polygon_mode_index_ = rhs.polygon_mode_index_;
    point_size_index_ = rhs.point_size_index_;
    colour_material_index_ = rhs.colour_material_index_;
    blend_func_index_ = rhs.blend_func_index_;

    for(auto& pass: passes_) {
        pass.material_ = this;
        pass.top_level_ = this;
    }

    texture_properties_ = rhs.texture_properties_;
}

Material& Material::operator=(const Material& rhs) {
    Resource::operator=(rhs);
    _material_impl::PropertyValueHolder::operator=(rhs);

    top_level_ = this;
    defined_property_count_ = rhs.defined_property_count_;

    defined_property_lookup_ = rhs.defined_property_lookup_;
    pass_count_ = rhs.pass_count_;

    for(auto i = 0u; i < rhs.defined_property_count_; ++i) {
        defined_properties_[i] = rhs.defined_properties_[i];
    }

    for(auto i = 0; i < pass_count_; ++i) {
        passes_[i] = rhs.passes_[i];
        passes_[i].material_ = this;
        passes_[i].top_level_ = this;
    }

    texture_properties_ = rhs.texture_properties_;

    material_ambient_index_ = rhs.material_ambient_index_;
    material_diffuse_index_ = rhs.material_diffuse_index_;
    material_shininess_index_ = rhs.material_shininess_index_;
    material_specular_index_ = rhs.material_specular_index_;
    diffuse_map_index_ = rhs.diffuse_map_index_;
    specular_map_index_ = rhs.specular_map_index_;
    normal_map_index_ = rhs.normal_map_index_;
    light_map_index_ = rhs.light_map_index_;

    lighting_enabled_index_ = rhs.lighting_enabled_index_;
    texturing_enabled_index_ = rhs.texturing_enabled_index_;

    depth_test_enabled_index_ = rhs.depth_test_enabled_index_;
    depth_write_enabled_index_ = rhs.depth_write_enabled_index_;

    shade_model_index_ = rhs.shade_model_index_;
    cull_mode_index_ = rhs.cull_mode_index_;
    polygon_mode_index_ = rhs.polygon_mode_index_;
    point_size_index_ = rhs.point_size_index_;
    colour_material_index_ = rhs.colour_material_index_;
    blend_func_index_ = rhs.blend_func_index_;

    return *this;
}

std::vector<std::string> Material::defined_custom_properties() const {
    std::vector<std::string> ret;
    for(auto i = 0u; i < defined_property_count_; ++i) {
        auto& p = defined_properties_[i];
        if(p.is_custom) {
            ret.push_back(p.name);
        }

    }

    return ret;
}

void Material::set_pass_count(uint8_t pass_count) {
    if(pass_count == pass_count_) {
        return;
    }

    std::lock_guard<std::mutex> lock(pass_mutex_);

    /* We're adding more passes, so let's make sure they're clean */
    if(pass_count > pass_count_) {
        for(auto i = pass_count_; i < pass_count; ++i) {
            passes_[i] = MaterialPass(this, i);
        }
    }

    pass_count_ = pass_count;
}

MaterialPass *Material::pass(uint8_t pass) {
    if(pass < pass_count_) {
        return &passes_[pass];
    }

    /* Shouldn't happen in normal operation */
    assert(pass < pass_count_);
    return nullptr;
}

void Material::update(float dt) {

}

const std::vector<PropertyIndex>& Material::defined_properties_by_type(MaterialPropertyType type) const {
    if(type == MATERIAL_PROPERTY_TYPE_TEXTURE) {
        return texture_properties_;
    } else {
        assert(0 && "Not implemented");
        L_ERROR("Not implemented");

        static std::vector<PropertyIndex> disabled;
        return disabled;
    }
}

void Material::initialize_default_properties() {
    material_ambient_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, AMBIENT_PROPERTY, Vec4(1, 1, 1, 1));
    material_diffuse_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, DIFFUSE_PROPERTY, Vec4(1, 1, 1, 1));
    material_specular_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, SPECULAR_PROPERTY, Vec4(1, 1, 1, 1));
    material_shininess_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, SHININESS_PROPERTY, 0.0f);

    diffuse_map_index_ =define_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, DIFFUSE_MAP_PROPERTY, TextureUnit());
    light_map_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, LIGHT_MAP_PROPERTY, TextureUnit());
    normal_map_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, NORMAL_MAP_PROPERTY, TextureUnit());
    specular_map_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, SPECULAR_MAP_PROPERTY, TextureUnit());

    blend_func_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_INT, BLEND_FUNC_PROPERTY, (int) BLEND_NONE);
    cull_mode_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_INT, CULL_MODE_PROPERTY, (int) CULL_MODE_BACK_FACE);
    colour_material_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_INT, COLOUR_MATERIAL_PROPERTY, (int) COLOUR_MATERIAL_NONE);

    depth_test_enabled_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, DEPTH_TEST_ENABLED_PROPERTY, true);
    // define_builtin_property(DEPTH_FUNC_PROPERTY, MATERIAL_PROPERTY_TYPE_INT, DEPTH_FUNC_LEQUAL);

    depth_write_enabled_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, DEPTH_WRITE_ENABLED_PROPERTY, true);
    shade_model_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_INT, SHADE_MODEL_PROPERTY, (int) SHADE_MODEL_SMOOTH);
    polygon_mode_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_INT, POLYGON_MODE_PROPERTY, (int) POLYGON_MODE_FILL);

    lighting_enabled_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, LIGHTING_ENABLED_PROPERTY, false);
    texturing_enabled_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, TEXTURING_ENABLED_PROPERTY, true);
    point_size_index_ = define_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, POINT_SIZE_PROPERTY, 1.0f);

    define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, LIGHT_POSITION_PROPERTY, Vec4());
    define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, LIGHT_AMBIENT_PROPERTY, Vec4(1, 1, 1, 1));
    define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, LIGHT_DIFFUSE_PROPERTY, Vec4(1, 1, 1, 1));
    define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, LIGHT_SPECULAR_PROPERTY, Vec4(1, 1, 1, 1));

    define_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, LIGHT_CONSTANT_ATTENUATION_PROPERTY, 0.0f);
    define_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, LIGHT_LINEAR_ATTENUATION_PROPERTY, 0.0f);
    define_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, LIGHT_QUADRATIC_ATTENUATION_PROPERTY, 0.0f);

    define_builtin_property(MATERIAL_PROPERTY_TYPE_MAT4, MODELVIEW_MATRIX_PROPERTY, Mat4());
    define_builtin_property(MATERIAL_PROPERTY_TYPE_MAT4, MODELVIEW_PROJECTION_MATRIX_PROPERTY, Mat4());
    define_builtin_property(MATERIAL_PROPERTY_TYPE_MAT4, PROJECTION_MATRIX_PROPERTY, Mat4());
    define_builtin_property(MATERIAL_PROPERTY_TYPE_MAT4, VIEW_MATRIX_PROPERTY, Mat4());
    define_builtin_property(MATERIAL_PROPERTY_TYPE_MAT3, INVERSE_TRANSPOSE_MODELVIEW_MATRIX_PROPERTY, Mat3());
}

/* This is the same as name for now, potentially we could generate this
 * automatically (e.g. replace spaces with underscores etc. if we want prettier names) */
std::string PropertyValue::shader_variable() const {
    return defined_property_->name;
}

std::string PropertyValue::name() const {
    return defined_property_->name;
}

bool PropertyValue::is_custom() const {
    return defined_property_->is_custom;
}

MaterialPropertyType PropertyValue::type() const {
    return defined_property_->type;
}

MaterialPass::MaterialPass(Material *material, uint8_t index):
    _material_impl::PropertyValueHolder(material, index + 1), // slot 0 is the Material
    material_(material) {

    /* If the renderer supports GPU programs, at least specify *something* */
    auto& renderer = material_->resource_manager().window->renderer;
    if(renderer->supports_gpu_programs()) {
        set_gpu_program(renderer->default_gpu_program_id());
    }
}

GPUProgramID MaterialPass::gpu_program_id() const {
    return program_->id();
}

void TextureUnit::scroll_x(float amount) {
    Mat4 diff = Mat4::as_translation(Vec3(amount, 0, 0));
    *texture_matrix_ = *texture_matrix_ * diff;
}

void TextureUnit::scroll_y(float amount) {
    Mat4 diff = Mat4::as_translation(Vec3(0, amount, 0));
    *texture_matrix_ = *texture_matrix_ * diff;
}

void _material_impl::PropertyValueHolder::set_property_value(const std::string &name, TextureID tex_id) {
    set_property_value(top_level()->defined_property_index(name), tex_id);
}

PropertyValue _material_impl::PropertyValueHolder::property(uint32_t defined_property_index) const {
    auto& prop = top_level_->defined_properties_[defined_property_index];
    if(prop.slot_used[slot_]) {
        return PropertyValue(&prop, slot_);
    } else {
        assert(prop.slot_used[0]);
        return PropertyValue(&prop, 0);
    }
}

PropertyValue _material_impl::PropertyValueHolder::property(const std::string &name) const {
    return property(top_level_->defined_property_index(name));
}

void _material_impl::PropertyValueHolder::set_specular(const Colour &colour) {
    set_property_value(SPECULAR_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void _material_impl::PropertyValueHolder::set_ambient(const Colour &colour) {
    set_property_value(AMBIENT_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void _material_impl::PropertyValueHolder::set_diffuse(const Colour &colour) {
    set_property_value(DIFFUSE_PROPERTY, Vec4(colour.r, colour.g, colour.b, colour.a));
}

void _material_impl::PropertyValueHolder::set_shininess(float shininess) {
    set_property_value(SHININESS_PROPERTY, shininess);
}

void _material_impl::PropertyValueHolder::set_diffuse_map(TextureID texture_id) {
    set_property_value(DIFFUSE_MAP_PROPERTY, texture_id);
}

void _material_impl::PropertyValueHolder::set_light_map(TextureID texture_id) {
    set_property_value(LIGHT_MAP_PROPERTY, texture_id);
}

TextureUnit _material_impl::PropertyValueHolder::diffuse_map() const {
    return property(top_level_->diffuse_map_index_).value<TextureUnit>();
}

TextureUnit _material_impl::PropertyValueHolder::light_map() const {
    return property(top_level_->light_map_index_).value<TextureUnit>();
}

TextureUnit _material_impl::PropertyValueHolder::normal_map() const {
    return property(top_level_->normal_map_index_).value<TextureUnit>();
}

TextureUnit _material_impl::PropertyValueHolder::specular_map() const {
    return property(top_level_->specular_map_index_).value<TextureUnit>();
}

Colour _material_impl::PropertyValueHolder::specular() const {
    auto v = property(top_level_->material_specular_index_).value<Vec4>();
    return Colour(v.x, v.y, v.z, v.w);
}

Colour _material_impl::PropertyValueHolder::ambient() const {
    auto v = property(top_level_->material_ambient_index_).value<Vec4>();
    return Colour(v.x, v.y, v.z, v.w);
}

Colour _material_impl::PropertyValueHolder::diffuse() const {
    auto v = property(top_level_->material_diffuse_index_).value<Vec4>();
    return Colour(v.x, v.y, v.z, v.w);
}

float _material_impl::PropertyValueHolder::shininess() const {
    return property(top_level_->material_shininess_index_).value<float>();
}

bool _material_impl::PropertyValueHolder::is_blending_enabled() const {
    return (BlendType) property(top_level_->blend_func_index_).value<int>() == BLEND_NONE;
}

void _material_impl::PropertyValueHolder::set_blend_func(BlendType b) {
    set_property_value(BLEND_FUNC_PROPERTY, (int) b);
}

BlendType _material_impl::PropertyValueHolder::blend_func() const {
    return (BlendType) property(top_level_->blend_func_index_).value<int>();
}

void _material_impl::PropertyValueHolder::set_depth_write_enabled(bool v) {
    set_property_value(DEPTH_WRITE_ENABLED_PROPERTY, v);
}

bool _material_impl::PropertyValueHolder::is_depth_write_enabled() const {
    return property(top_level_->depth_write_enabled_index_).value<bool>();
}

void _material_impl::PropertyValueHolder::set_cull_mode(CullMode mode) {
    set_property_value(CULL_MODE_PROPERTY, (int) mode);
}

CullMode _material_impl::PropertyValueHolder::cull_mode() const {
    return (CullMode) property(top_level_->cull_mode_index_).value<int>();
}

void _material_impl::PropertyValueHolder::set_depth_test_enabled(bool v) {
    set_property_value(DEPTH_TEST_ENABLED_PROPERTY, v);
}

bool _material_impl::PropertyValueHolder::is_depth_test_enabled() const {
    return property(top_level_->depth_test_enabled_index_).value<bool>();
}

void _material_impl::PropertyValueHolder::set_lighting_enabled(bool v) {
    set_property_value(LIGHTING_ENABLED_PROPERTY, v);
}

bool _material_impl::PropertyValueHolder::is_lighting_enabled() const {
    return property(top_level_->lighting_enabled_index_).value<bool>();
}

void _material_impl::PropertyValueHolder::set_texturing_enabled(bool v) {
    set_property_value(TEXTURING_ENABLED_PROPERTY, v);
}

bool _material_impl::PropertyValueHolder::is_texturing_enabled() const {
    return property(top_level_->texturing_enabled_index_).value<bool>();
}

float _material_impl::PropertyValueHolder::point_size() const {
    return property(top_level_->point_size_index_).value<float>();
}

PolygonMode _material_impl::PropertyValueHolder::polygon_mode() const {
    return (PolygonMode) property(top_level_->polygon_mode_index_).value<int>();
}

void _material_impl::PropertyValueHolder::set_shade_model(ShadeModel model) {
    set_property_value(SHADE_MODEL_PROPERTY, (int) model);
}

ShadeModel _material_impl::PropertyValueHolder::shade_model() const {
    return (ShadeModel) property(top_level_->shade_model_index_).value<int>();
}

ColourMaterial _material_impl::PropertyValueHolder::colour_material() const {
    return (ColourMaterial) property(top_level_->colour_material_index_).value<int>();
}

}
