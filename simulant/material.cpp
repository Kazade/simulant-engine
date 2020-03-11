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
    pass_count_(MAX_MATERIAL_PASSES),
    passes_{{MaterialPass(this), MaterialPass(this), MaterialPass(this), MaterialPass(this)}} {

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
    passes_{{MaterialPass(this), MaterialPass(this), MaterialPass(this), MaterialPass(this)}}{

    *this = rhs;
}

Material& Material::operator=(const Material& rhs) {
    Asset::operator=(rhs);

    /* Copy the material registry, then all the passes and register those
       while maintaining their property value entries */
    initialize_free_object_ids();
    clear_registered_objects();

    // First object is the registry
    registered_objects_[0] = this;

    // Copy all properties
    properties_ = rhs.properties_;
    pass_count_ = rhs.pass_count_;

    /* Copy the passes */
    for(auto i = 0; i < rhs.pass_count_; ++i) {
        // MaterialObject
        passes_[i].registry_ = this;
        passes_[i].object_id_ = rhs.passes_[i].object_id_;

        // MaterialPass
        passes_[i].copy_from(rhs.passes_[i], this);

        /* Rebuild the registered objects manually */
        assert(passes_[i].object_id_ > 0);
        registered_objects_[passes_[i].object_id_] = &passes_[i];
    }

    /* Update property pointers on entries */
    for(auto& prop: properties_) {
        prop.entries[0].object = this;
        prop.entries[0].is_set = true;
        prop.entries[0].value.property_ = &prop;

        for(uint8_t i = 1u; i < _S_ARRAY_LENGTH(prop.entries); ++i) {
            prop.entries[i].object = &passes_[i - 1];
            prop.entries[i].value.property_ = &prop;
        }
    }

    rebuild_texture_properties();
    rebuild_custom_properties();

    material_ambient_id_ = rhs.material_ambient_id_;
    material_diffuse_id_ = rhs.material_diffuse_id_;
    material_specular_id_ = rhs.material_specular_id_;
    material_shininess_id_ = rhs.material_shininess_id_;
    diffuse_map_id_ = rhs.diffuse_map_id_;
    specular_map_id_ = rhs.specular_map_id_;
    light_map_id_ = rhs.light_map_id_;
    normal_map_id_ = rhs.normal_map_id_;

    blending_enabled_id_ = rhs.blending_enabled_id_;
    texturing_enabled_id_ = rhs.texturing_enabled_id_;
    lighting_enabled_id_ = rhs.lighting_enabled_id_;
    depth_test_enabled_id_ = rhs.depth_test_enabled_id_;
    depth_write_enabled_id_ = rhs.depth_write_enabled_id_;

    shade_model_id_ = rhs.shade_model_id_;
    cull_mode_id_ = rhs.cull_mode_id_;
    polygon_mode_id_ = rhs.polygon_mode_id_;
    point_size_id_ = rhs.point_size_id_;
    colour_material_id_ = rhs.colour_material_id_;
    blend_func_id_ = rhs.blend_func_id_;

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
            new (&passes_[i]) MaterialPass(this);
            register_object(&passes_[i]);
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
    _S_UNUSED(dt);
}


MaterialPass::MaterialPass(Material *material):
    MaterialObject(material),
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
