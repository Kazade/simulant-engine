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

#include "../window.h"
#include "../asset_manager.h"
#include "../renderers/renderer.h"
#include "../renderers/gl2x/gpu_program.h"

#include "material.h"

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

std::unordered_map<MaterialPropertyNameHash, Material::PropertyName> Material::hashes_to_names_;

Material::Material(MaterialID id, AssetManager* asset_manager):
    Asset(asset_manager),
    generic::Identifiable<MaterialID>(id),
    renderer_(asset_manager->window->renderer) {

    set_pass_count(1);  // Enable a single pass by default otherwise the material is useless

    /* Some renderers will need to register additional properties etc.
     * on the material. So this is called on material construction */
    renderer_->prepare_material(this);
}

Material::~Material() {

}

bool Material::set_pass_count(uint8_t pass_count) {
    if(pass_count >= MAX_MATERIAL_PASSES) {
        return false;
    }

    passes_.resize(pass_count, MaterialPass(this));

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

void Material::update(float dt) {
    _S_UNUSED(dt);
}

Material &Material::operator=(const Material &rhs) {
    MaterialObject::operator=(rhs);

    renderer_ = rhs.renderer_;
    passes_.clear();

    /* Must set the parent to this material */
    for(std::size_t i = 0; i < rhs.passes_.size(); ++i) {
        MaterialPass pass;
        pass = rhs.passes_[i];
        pass.parent_ = this;
        passes_.push_back(std::move(pass));
    }

    /* Make sure this material is prepped */
    renderer_->prepare_material(this);

    return *this;
}

MaterialPtr Material::new_clone() {
    auto mat = asset_manager().new_material();
    *mat = *this;
    return mat;
}


MaterialPass::MaterialPass():
    MaterialObject(nullptr) {}

MaterialPass::MaterialPass(Material *material):
    MaterialObject(material) {

    /* If the renderer supports GPU programs, at least specify *something* */
    auto& renderer = this->material()->asset_manager().window->renderer;
    if(renderer->supports_gpu_programs()) {
        set_gpu_program(renderer->default_gpu_program_id());
    }
}

GPUProgramID MaterialPass::gpu_program_id() const {
    return program_->id();
}

const Material *MaterialPass::material() const {
    return dynamic_cast<const Material*>(parent_material_object());
}


}
