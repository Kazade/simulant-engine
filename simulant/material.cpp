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
    Asset(asset_manager),
    generic::Identifiable<MaterialID>(id),
    MaterialPropertyRegistry(),
    MaterialObject(this, MATERIAL_OBJECT_TYPE_ROOT),
    pass_count_(MAX_PASSES),
    passes_{{MaterialPass(this, 0), MaterialPass(this, 1), MaterialPass(this, 2), MaterialPass(this, 3)}} {

    set_pass_count(1);  // Enable a single pass by default otherwise the material is useless
}

Material::~Material() {
    for(auto& pass: passes_) {
        unregister_object(&pass);
    }
}

Material::Material(const Material& rhs):
    std::enable_shared_from_this<Material>(rhs),
    Asset(rhs),
    generic::Identifiable<MaterialID>(rhs),
    RefCounted<Material>(rhs),
    MaterialPropertyRegistry(rhs),
    MaterialObject(rhs),
    pass_count_(rhs.pass_count_),
    passes_(rhs.passes_) {

    *this = rhs;
}

Material& Material::operator=(const Material& rhs) {
    Asset::operator=(rhs);
    MaterialPropertyRegistry::operator=(rhs);
    MaterialObject::operator=(rhs);

    register_object(this, MATERIAL_OBJECT_TYPE_ROOT);

    for(auto i = 0; i < rhs.pass_count_; ++i) {
        passes_[i] = rhs.passes_[i];
        passes_[i].material_ = this;
        register_object(&passes_[i], MATERIAL_OBJECT_TYPE_LEAF);
    }

    set_pass_count(rhs.pass_count_);

    return *this;
}

void Material::set_pass_count(uint8_t pass_count) {
    if(pass_count == pass_count_) {
        return;
    }

    thread::Lock<thread::Mutex> lock(pass_mutex_);

    /* We're adding more passes, so let's make sure they're clean */
    if(pass_count > pass_count_) {
        for(auto i = pass_count_; i < pass_count; ++i) {
            passes_[i] = MaterialPass(this, i);
            register_object(&passes_[i], MATERIAL_OBJECT_TYPE_LEAF);
        }
    }

    /* We need to unregister any passes not in use anymore
     * from the property registry */
    for(auto i = pass_count_ - 1; i >= pass_count; --i) {
        unregister_object(&passes_[i]);
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


MaterialPass::MaterialPass(Material *material, uint8_t index):
    MaterialObject(material, MATERIAL_OBJECT_TYPE_LEAF),
    material_(material) {

    /* If the renderer supports GPU programs, at least specify *something* */
    auto& renderer = material_->asset_manager().window->renderer;
    if(renderer->supports_gpu_programs()) {
        set_gpu_program(renderer->default_gpu_program_id());
    }
}

GPUProgramID MaterialPass::gpu_program_id() const {
    return program_->id();
}

}
