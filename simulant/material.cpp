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

const std::string Material::BuiltIns::DEFAULT = "simulant/materials/${RENDERER}/default.kglm";
const std::string Material::BuiltIns::TEXTURE_ONLY = "simulant/materials/${RENDERER}/texture_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_ONLY = "simulant/materials/${RENDERER}/diffuse_only.kglm";
const std::string Material::BuiltIns::ALPHA_TEXTURE = "simulant/materials/${RENDERER}/alpha_texture.kglm";
const std::string Material::BuiltIns::DIFFUSE_WITH_LIGHTING = "simulant/materials/${RENDERER}/diffuse_with_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE = "simulant/materials/${RENDERER}/multitexture2_modulate.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_ADD = "simulant/materials/${RENDERER}/multitexture2_add.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP = "simulant/materials/${RENDERER}/texture_with_lightmap.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP_AND_LIGHTING = "simulant/materials/${RENDERER}/texture_with_lightmap_and_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING = "simulant/materials/${RENDERER}/multitexture2_modulate_with_lighting.kglm";
const std::string Material::BuiltIns::SKYBOX = "simulant/materials/${RENDERER}/skybox.kglm";
const std::string Material::BuiltIns::TEXTURED_PARTICLE = "simulant/materials/${RENDERER}/textured_particle.kglm";
const std::string Material::BuiltIns::DIFFUSE_PARTICLE = "simulant/materials/${RENDERER}/diffuse_particle.kglm";

/* This list is used by the particle script loader to determine if a specified material
 * is a built-in or not. Please keep this up-to-date when changing the above materials!
 */
const std::unordered_map<std::string, std::string> Material::BUILT_IN_NAMES = {
    {"DEFAULT", Material::BuiltIns::DEFAULT},
    {"TEXTURE_ONLY", Material::BuiltIns::TEXTURE_ONLY},
    {"DIFFUSE_ONLY", Material::BuiltIns::DIFFUSE_ONLY},
    {"ALPHA_TEXTURE", Material::BuiltIns::ALPHA_TEXTURE},
    {"DIFFUSE_WITH_LIGHTING", Material::BuiltIns::DIFFUSE_WITH_LIGHTING},
    {"MULTITEXTURE2_MODULATE", Material::BuiltIns::MULTITEXTURE2_MODULATE},
    {"MULTITEXTURE2_ADD", Material::BuiltIns::MULTITEXTURE2_ADD},
    {"TEXTURE_WITH_LIGHTMAP", Material::BuiltIns::TEXTURE_WITH_LIGHTMAP},
    {"TEXTURE_WITH_LIGHTMAP_AND_LIGHTING", Material::BuiltIns::TEXTURE_WITH_LIGHTMAP_AND_LIGHTING},
    {"MULTITEXTURE2_MODULATE_WITH_LIGHTING", Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING},
    {"SKYBOX", Material::BuiltIns::SKYBOX},
    {"TEXTURED_PARTICLE", Material::BuiltIns::TEXTURED_PARTICLE},
    {"DIFFUSE_PARTICLE", Material::BuiltIns::DIFFUSE_PARTICLE}
};

Material::Material(MaterialID id, AssetManager* asset_manager):
    Resource(asset_manager),
    generic::Identifiable<MaterialID>(id),
    _material_impl::PropertyValueHolder(this),
    passes_({MaterialPass(this), MaterialPass(this), MaterialPass(this), MaterialPass(this)}) {

    initialize_default_properties();
}

std::vector<std::string> Material::defined_custom_properties() const {
    std::map<uint32_t, std::string> ordered_properties;
    for(auto& p: defined_properties_) {
        if(p.second.is_custom) {
            ordered_properties.insert(std::make_pair(p.second.order, p.first));
        }
    }

    std::vector<std::string> ret;
    for(auto& p: ordered_properties) {
        ret.push_back(p.second);
    }

    return ret;
}

void Material::update(float dt) {

}

std::vector<std::string> Material::defined_properties_by_type(MaterialPropertyType type) const {
    std::map<uint32_t, std::string> ordered_properties;
    for(auto& p: defined_properties_) {
        if(p.second.type == type) {
            ordered_properties.insert(std::make_pair(p.second.order, p.first));
        }
    }

    std::vector<std::string> ret;
    for(auto& p: ordered_properties) {
        ret.push_back(p.second);
    }

    return ret;
}

void Material::initialize_default_properties() {
    define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, EMISSION_PROPERTY, "s_material_emission", Vec4(1, 1, 1, 1));
    define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, AMBIENT_PROPERTY, "s_material_ambient", Vec4(1, 1, 1, 1));
    define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, DIFFUSE_PROPERTY, "s_material_diffuse", Vec4(1, 1, 1, 1));
    define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, SPECULAR_PROPERTY, "s_material_specular", Vec4(1, 1, 1, 1));
    define_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, SHININESS_PROPERTY, "s_material_shininess", 0);

    define_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, DIFFUSE_MAP_PROPERTY, "s_diffuse_map");
    define_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, NORMAL_MAP_PROPERTY, "s_normal_map");
    define_builtin_property(MATERIAL_PROPERTY_TYPE_TEXTURE, SPECULAR_MAP_PROPERTY, "s_specular_map");

    define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, BLENDING_ENABLE_PROPERTY, "s_blending_enabled", false);
    define_builtin_property(MATERIAL_PROPERTY_TYPE_INT, BLEND_FUNC_PROPERTY, "s_blend_mode", BLEND_NONE);

    define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, DEPTH_TEST_ENABLED_PROPERTY, "s_depth_test_enabled", true);
    // define_builtin_property(DEPTH_FUNC_PROPERTY, MATERIAL_PROPERTY_TYPE_INT, "s_depth_func", DEPTH_FUNC_LEQUAL);

    define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, DEPTH_WRITE_ENABLED_PROPERTY, "s_depth_write_enabled", true);

    define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, CULLING_ENABLED_PROPERTY, "s_culling_enabled", true);
    define_builtin_property(MATERIAL_PROPERTY_TYPE_INT, CULL_MODE_PROPERTY, "s_cull_mode", CULL_MODE_BACK_FACE);

    define_builtin_property(MATERIAL_PROPERTY_TYPE_INT, SHADE_MODEL_PROPERTY, "s_shade_model", SHADE_MODEL_SMOOTH);
    define_builtin_property(MATERIAL_PROPERTY_TYPE_INT, POLYGON_MODE_PROPERTY, "s_polygon_mode", POLYGON_MODE_FILL);

    define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, LIGHTING_ENABLED_PROPERTY, "s_lights_enabled", false);
    define_builtin_property(MATERIAL_PROPERTY_TYPE_BOOL, TEXTURING_ENABLED_PROPERTY, "s_textures_enabled", true);
    define_builtin_property(MATERIAL_PROPERTY_TYPE_FLOAT, POINT_SIZE_PROPERTY, "s_point_size", 1.0);

    define_builtin_property(MATERIAL_PROPERTY_TYPE_VEC4, LIGHT_POSITION_PROPERTY, "s_light_position");
}

std::string PropertyValue::shader_variable() const {
    return defined_property_->shader_variable;
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

void _material_impl::PropertyValueHolder::set_property_value(const std::string &name, bool value) {
    PropertyValue v(&top_level_->defined_properties_.at(name));
    v.value_ = smlt::any(value);

    auto ret = property_values_.emplace(std::make_pair(name, v));
    if(!ret.second) {
        property_values_.at(name) = v;
    }
}

void _material_impl::PropertyValueHolder::set_property_value(const std::string &name, smlt::Vec4 value) {
    PropertyValue v(&top_level_->defined_properties_.at(name));
    v.value_ = smlt::any(value);

    auto ret = property_values_.emplace(std::make_pair(name, v));
    if(!ret.second) {
        property_values_.at(name) = v;
    }
}

void _material_impl::PropertyValueHolder::set_property_value(const std::string &name, smlt::TextureUnit value) {
    PropertyValue v(&top_level_->defined_properties_.at(name));
    v.value_ = smlt::any(value);

    auto ret = property_values_.emplace(std::make_pair(name, v));
    if(!ret.second) {
        property_values_.at(name) = v;
    }
}

}
