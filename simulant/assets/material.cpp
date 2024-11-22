//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdexcept>
#include <cassert>
#include <map>

#include "../application.h"
#include "../window.h"
#include "../asset_manager.h"
#include "../renderers/renderer.h"
#include "../renderers/gl2x/gpu_program.h"

#include "material.h"

namespace smlt {

const std::string Material::BuiltIns::DEFAULT = "materials/${RENDERER}/default.smat";
const std::string Material::BuiltIns::TEXTURE_ONLY = "materials/${RENDERER}/texture_only.smat";
const std::string Material::BuiltIns::DIFFUSE_ONLY = "materials/${RENDERER}/diffuse_only.smat";

/* This list is used by the particle script loader to determine if a specified material
 * is a built-in or not. Please keep this up-to-date when changing the above materials!
 */
const std::unordered_map<std::string, std::string> Material::BUILT_IN_NAMES = {
    {"DEFAULT", Material::BuiltIns::DEFAULT},
    {"TEXTURE_ONLY", Material::BuiltIns::TEXTURE_ONLY},
    {"DIFFUSE_ONLY", Material::BuiltIns::DIFFUSE_ONLY},
};

Material::Material(AssetID id, AssetManager* asset_manager):
    Asset(asset_manager),
    generic::Identifiable<AssetID>(id),
    renderer_(get_app()->window->renderer) {

    /* The core material has 4 texture properties by default */

    TexturePropertyInfo diffuse_map;
    diffuse_map.texture_property_name = DIFFUSE_MAP_PROPERTY_NAME;
    diffuse_map.texture_property_name_hash = DIFFUSE_MAP_PROPERTY_HASH;
    diffuse_map.matrix_property_name = DIFFUSE_MAP_MATRIX_PROPERTY_NAME;
    diffuse_map.matrix_property_name_hash = DIFFUSE_MAP_MATRIX_PROPERTY_HASH;
    texture_properties_.insert(std::make_pair(DIFFUSE_MAP_PROPERTY_HASH, diffuse_map));

    TexturePropertyInfo light_map;
    light_map.texture_property_name = LIGHT_MAP_PROPERTY_NAME;
    light_map.texture_property_name_hash = LIGHT_MAP_PROPERTY_HASH;
    light_map.matrix_property_name = LIGHT_MAP_MATRIX_PROPERTY_NAME;
    light_map.matrix_property_name_hash = LIGHT_MAP_MATRIX_PROPERTY_HASH;
    texture_properties_.insert(std::make_pair(LIGHT_MAP_PROPERTY_HASH, light_map));

    TexturePropertyInfo specular_map;
    specular_map.texture_property_name = SPECULAR_MAP_PROPERTY_NAME;
    specular_map.texture_property_name_hash = SPECULAR_MAP_PROPERTY_HASH;
    specular_map.matrix_property_name = SPECULAR_MAP_MATRIX_PROPERTY_NAME;
    specular_map.matrix_property_name_hash = SPECULAR_MAP_MATRIX_PROPERTY_HASH;
    texture_properties_.insert(std::make_pair(SPECULAR_MAP_PROPERTY_HASH, specular_map));


    TexturePropertyInfo normal_map;
    normal_map.texture_property_name = NORMAL_MAP_PROPERTY_NAME;
    normal_map.texture_property_name_hash = NORMAL_MAP_PROPERTY_HASH;
    normal_map.matrix_property_name = NORMAL_MAP_MATRIX_PROPERTY_NAME;
    normal_map.matrix_property_name_hash = NORMAL_MAP_MATRIX_PROPERTY_HASH;
    texture_properties_.insert(std::make_pair(NORMAL_MAP_PROPERTY_HASH, normal_map));

    initialize_core_properties();
    set_pass_count(1);  // Enable a single pass by default otherwise the material is useless

    /* Some renderers will need to register additional properties etc.
     * on the material. So this is called on material construction */
    renderer_->prepare_material(this);
}

Material::~Material() {}

void Material::initialize_core_properties() {
    set_property_value(DIFFUSE_PROPERTY_NAME, Color::white());
    set_property_value(AMBIENT_PROPERTY_NAME, Color::white());
    set_property_value(EMISSION_PROPERTY_NAME, Color(0, 0, 0, 1));
    set_property_value(SPECULAR_PROPERTY_NAME, Color(0, 0, 0, 1));

    set_property_value(SHININESS_PROPERTY_NAME, 0.0f);
    set_property_value(POINT_SIZE_PROPERTY_NAME, 1.0f);

    set_property_value(DEPTH_WRITE_ENABLED_PROPERTY_NAME, true);
    set_property_value(DEPTH_TEST_ENABLED_PROPERTY_NAME, true);
    set_property_value(LIGHTING_ENABLED_PROPERTY_NAME, true);

    set_property_value(TEXTURES_ENABLED_PROPERTY_NAME,
                       DIFFUSE_MAP_ENABLED | LIGHT_MAP_ENABLED |
                           SPECULAR_MAP_ENABLED | NORMAL_MAP_ENABLED);

    set_property_value(DIFFUSE_MAP_PROPERTY_NAME, TexturePtr());
    set_property_value(SPECULAR_MAP_PROPERTY_NAME, TexturePtr());
    set_property_value(LIGHT_MAP_PROPERTY_NAME, TexturePtr());
    set_property_value(NORMAL_MAP_PROPERTY_NAME, TexturePtr());

    set_property_value(DIFFUSE_MAP_MATRIX_PROPERTY_NAME, Mat4());
    set_property_value(SPECULAR_MAP_MATRIX_PROPERTY_NAME, Mat4());
    set_property_value(LIGHT_MAP_MATRIX_PROPERTY_NAME, Mat4());
    set_property_value(NORMAL_MAP_MATRIX_PROPERTY_NAME, Mat4());

    set_property_value(BLEND_FUNC_PROPERTY_NAME, (EnumType)BLEND_NONE);
    set_property_value(POLYGON_MODE_PROPERTY_NAME, (EnumType)POLYGON_MODE_FILL);
    set_property_value(SHADE_MODEL_PROPERTY_NAME, (EnumType)SHADE_MODEL_SMOOTH);
    set_property_value(COLOR_MATERIAL_PROPERTY_NAME,
                       (EnumType)COLOR_MATERIAL_NONE);
    set_property_value(CULL_MODE_PROPERTY_NAME, (EnumType)CULL_MODE_NONE);
    set_property_value(DEPTH_FUNC_PROPERTY_NAME, (EnumType)DEPTH_FUNC_LEQUAL);
    set_property_value(ALPHA_FUNC_PROPERTY_NAME, (EnumType)ALPHA_FUNC_NONE);
    set_property_value(ALPHA_THRESHOLD_PROPERTY_NAME, 1.0f);

    set_property_value(FOG_MODE_PROPERTY_NAME, (EnumType)FOG_MODE_NONE);
    set_property_value(FOG_DENSITY_PROPERTY_NAME, 1.0f);
    set_property_value(FOG_START_PROPERTY_NAME, 100.0f);
    set_property_value(FOG_END_PROPERTY_NAME, 1000.0f);
    set_property_value(FOG_COLOR_PROPERTY_NAME, Color::white());
}

bool Material::set_pass_count(uint8_t pass_count) {
    if(pass_count >= MAX_MATERIAL_PASSES) {
        return false;
    }

    for(int i = passes_.size(); i < pass_count; ++i) {
        passes_.push_back(MaterialPass(this, i));
    }

    while(passes_.size() > pass_count) {
        passes_.pop_back();
    }

    return true;
}

MaterialPass *Material::pass(uint8_t pass) {
    if(pass < passes_.size()) {
        return &passes_[pass];
    }

    /* Shouldn't happen in normal operation */
    assert(pass < passes_.size());
    return nullptr;
}

Material &Material::operator=(const Material &rhs) {
    MaterialObject::operator=(rhs);

    renderer_ = rhs.renderer_;
    texture_properties_ = rhs.texture_properties_;
    custom_properties_ = rhs.custom_properties_;
    values_ = rhs.values_;
    passes_.clear();
    set_pass_count(rhs.passes_.size());

    /* Make sure this material is prepped */
    renderer_->prepare_material(this);

    return *this;
}

MaterialPass::MaterialPass():
    MaterialObject(nullptr) {}

MaterialPass::MaterialPass(Material* material, uint8_t pass_number) :
    MaterialObject(material), pass_number_(pass_number) {

    /* If the renderer supports GPU programs, at least specify *something* */
    auto& renderer = get_app()->window->renderer;
    if(renderer->supports_gpu_programs()) {
        set_gpu_program(renderer->default_gpu_program());
    }
}

GPUProgramID MaterialPass::gpu_program_id() const {
    return program_->id();
}

const Material *MaterialPass::material() const {
    return dynamic_cast<const Material*>(parent_material_object());
}

bool MaterialPass::on_check_existence(MaterialPropertyNameHash hsh) const {
    auto material = (Material*)parent_;

    auto& values = material->values_;
    return values.count(hsh + (pass_number_ + 1));
}

bool MaterialPass::on_clear_override(MaterialPropertyNameHash hsh) {
    auto key = hsh + (pass_number_ + 1);

    auto material = (Material*)parent_;

    auto& values = material->values_;
    if(values.count(key)) {
        values.at(key).reset();
        return true;
    }

    return false;
}

// We always just return the type of the parent
bool MaterialPass::property_type(const char* name,
                                 MaterialPropertyType* type) const {
    auto material = (Material*)parent_;
    return material->property_type(name, type);
}
}
