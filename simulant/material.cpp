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

#include "window_base.h"
#include "material.h"
#include "resource_manager.h"
#include "renderers/renderer.h"


namespace smlt {

#ifdef SIMULANT_GL_VERSION_1X
const std::string Material::BuiltIns::TEXTURE_ONLY = "simulant/materials/opengl-1.x/texture_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_ONLY = "simulant/materials/opengl-1.x/diffuse_only.kglm";
const std::string Material::BuiltIns::ALPHA_TEXTURE = "simulant/materials/opengl-1.x/alpha_texture.kglm";
const std::string Material::BuiltIns::DIFFUSE_WITH_LIGHTING = "simulant/materials/opengl-1.x/diffuse_with_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE = "simulant/materials/opengl-1.x/multitexture2_modulate.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_ADD = "simulant/materials/opengl-1.x/multitexture2_add.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP = "simulant/materials/opengl-1.x/texture_with_lightmap.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP_AND_LIGHTING = "simulant/materials/opengl-1.x/texture_with_lightmap_and_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING = "simulant/materials/opengl-1.x/multitexture2_modulate_with_lighting.kglm";
#else
const std::string Material::BuiltIns::TEXTURE_ONLY = "simulant/materials/opengl-2.x/texture_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_ONLY = "simulant/materials/opengl-2.x/diffuse_only.kglm";
const std::string Material::BuiltIns::ALPHA_TEXTURE = "simulant/materials/opengl-2.x/alpha_texture.kglm";
const std::string Material::BuiltIns::DIFFUSE_WITH_LIGHTING = "simulant/materials/opengl-2.x/diffuse_with_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE = "simulant/materials/opengl-2.x/multitexture2_modulate.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_ADD = "simulant/materials/opengl-2.x/multitexture2_add.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP = "simulant/materials/opengl-2.x/texture_with_lightmap.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP_AND_LIGHTING = "simulant/materials/opengl-2.x/texture_with_lightmap_and_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING = "simulant/materials/opengl-2.x/multitexture2_modulate_with_lighting.kglm";
#endif


static const std::string DEFAULT_VERT_SHADER = R"(
    attribute vec3 vertex_position;
    attribute vec4 vertex_diffuse;

    uniform mat4 modelview_projection;

    varying vec4 diffuse;

    void main() {
        diffuse = vertex_diffuse;
        gl_Position = (modelview_projection * vec4(vertex_position, 1.0));
    }
)";

static const std::string DEFAULT_FRAG_SHADER = R"(
    varying vec4 diffuse;
    void main() {
        gl_FragColor = diffuse;
    }
)";

TextureUnit::TextureUnit(MaterialPass &pass):
    pass_(&pass),
    time_elapsed_(0),
    current_texture_(0) {

    kmMat4Identity(&texture_matrix_);

    //Initialize the texture unit to the default texture
    ResourceManager& rm = pass.material->resource_manager();
    texture_unit_ = rm.texture(rm.default_texture_id());
}

TextureUnit::TextureUnit(MaterialPass &pass, TextureID tex_id):
    pass_(&pass),
    time_elapsed_(0),
    current_texture_(0) {

    kmMat4Identity(&texture_matrix_);

    //Initialize the texture unit
    ResourceManager& rm = pass.material->resource_manager();
    texture_unit_ = rm.texture(tex_id);
}

TextureUnit::TextureUnit(MaterialPass &pass, std::vector<TextureID> textures, double duration):
    pass_(&pass),
    animated_texture_duration_(duration),
    time_elapsed_(0),
    current_texture_(0),
    texture_unit_(0) {

    kmMat4Identity(&texture_matrix_);

    ResourceManager& rm = pass.material->resource_manager();

    for(TextureID tid: textures) {
        animated_texture_units_.push_back(rm.texture(tid));
    }
}

TextureID TextureUnit::texture_id() const {
    if(is_animated()) {
        return animated_texture_units_[current_texture_]->id();
    } else {
        return texture_unit_->id();
    }
}

Material::Material(MaterialID mat_id, ResourceManager *resource_manager):
    Resource(resource_manager),
    generic::Identifiable<MaterialID>(mat_id) {

}

Material::~Material() {

}

uint32_t Material::new_pass() {
    std::lock_guard<std::mutex> lock(pass_lock_);

    passes_.push_back(MaterialPass::ptr(new MaterialPass(this)));
    pass_count_ = passes_.size();

    on_pass_created(passes_.back().get());
    return passes_.size() - 1; //Return the index
}

void Material::delete_pass(uint32_t index) {
    std::lock_guard<std::mutex> lock(pass_lock_);

    auto pass = passes_.at(index);
    passes_.erase(passes_.begin() + index);
    pass_count_ = passes_.size();

    on_pass_destroyed(pass.get());
}

MaterialPass::ptr Material::pass(uint32_t index) {
    std::lock_guard<std::mutex> lock(pass_lock_);
    return passes_.at(index);
}

MaterialPass::MaterialPass(Material *material):
    material_(material),
    iteration_(ITERATE_ONCE),    
    max_iterations_(1),
    blend_(BLEND_NONE),
    depth_writes_enabled_(true),
    depth_test_enabled_(true),
    point_size_(1) {

}

void MaterialPass::set_texture_unit(uint32_t texture_unit_id, TextureID tex) {
    if(!allow_textures_) {
        throw std::logic_error("Attempted to set a texture on a pass which prevents them");
    }

    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        L_ERROR(_F("Texture unit ID is too high. {0} >= {1}").format(texture_unit_id, MAX_TEXTURE_UNITS));
        throw std::logic_error(_F("Texture unit ID is too high. {0} >= {1}").format(texture_unit_id, MAX_TEXTURE_UNITS));
    }

    TextureID previous_texture;

    if(texture_unit_id >= texture_units_.size()) {
        texture_units_.resize(texture_unit_id + 1, TextureUnit(*this));
    } else {
        previous_texture = texture_units_[texture_unit_id].texture_id();
    }
    texture_units_.at(texture_unit_id) = TextureUnit(*this, tex);

    material->on_pass_changed(this);
}

void MaterialPass::set_animated_texture_unit(uint32_t texture_unit_id, const std::vector<TextureID> textures, double duration) {
    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        throw std::logic_error("Texture unit ID is too high");
    }

    if(texture_units_.size() <= texture_unit_id) {
        texture_units_.resize(texture_unit_id + 1, TextureUnit(*this));
    }
    texture_units_[texture_unit_id] = TextureUnit(*this, textures, duration);

    material->on_pass_changed(this);
}

void MaterialPass::set_iteration(IterationType iter_type, uint32_t max) {
    iteration_ = iter_type;
    max_iterations_ = max;

    material->on_pass_changed(this);
}

void MaterialPass::set_albedo(float reflectiveness) {
    albedo_ = reflectiveness;
    if(is_reflective()) {
        material->reflective_passes_.insert(this);
    } else {
        material->reflective_passes_.erase(this);
    }

    material->on_pass_changed(this);
}

void MaterialPass::set_gpu_program_id(GPUProgramID program_id) {
    gpu_program_ = program_id;

    if(program_id) {
        // We hold a shared_ptr to the program so it doesn't get destroyed while we're using it
        gpu_program_ref_ = material->resource_manager().window->renderer->gpu_program(program_id);
    } else {
        gpu_program_ref_.reset();
    }
}


void Material::set_texture_unit_on_all_passes(uint32_t texture_unit_id, TextureID tex) {
    for(auto& p: passes_) {
        p->set_texture_unit(texture_unit_id, tex);
    }
}

void Material::update(double dt) {
    // The updating_disabled_ flag wasn't set so we
    // can safely update
    if(!updating_disabled_.test_and_set()) {
        for(auto& p: passes_) {
            p->update(dt);
        }

        // Clear when we are done
        updating_disabled_.clear();
    }
}

TextureUnit TextureUnit::new_clone(MaterialPass& owner) const {
    TextureUnit ret(owner);

    ret.animated_texture_units_ = animated_texture_units_;
    ret.animated_texture_duration_ = animated_texture_duration_;
    ret.time_elapsed_ = time_elapsed_;
    ret.current_texture_ = current_texture_;
    ret.texture_unit_ = texture_unit_;
    ret.texture_matrix_ = texture_matrix_;

    return ret;
}

MaterialPass::ptr MaterialPass::new_clone(Material* owner) const {
    MaterialPass::ptr ret = MaterialPass::create(owner);

    for(auto& p: uniforms_.auto_uniforms()) {
        ret->uniforms_.register_auto(p.first, p.second);
    }

    for(auto& p: attributes_.auto_attributes()) {
        ret->attributes_.register_auto(p.first, p.second);
    }

    ret->gpu_program_ = gpu_program_;
    ret->gpu_program_ref_ = gpu_program_ref_;

    ret->diffuse_ = diffuse_;
    ret->ambient_ = ambient_;
    ret->specular_ = specular_;
    ret->shininess_ = shininess_;

    ret->allow_textures_ = allow_textures_;

    for(auto& unit: texture_units_) {
        ret->texture_units_.push_back(unit.new_clone(*ret));
    }

    ret->iteration_ = iteration_;
    ret->max_iterations_ = max_iterations_;
    ret->blend_ = blend_;
    ret->depth_writes_enabled_ = depth_writes_enabled_;
    ret->depth_test_enabled_ = depth_test_enabled_;
    ret->point_size_ = point_size_;
    ret->albedo_ = albedo_;
    ret->reflection_texture_unit_ = reflection_texture_unit_;
    ret->polygon_mode_ = polygon_mode_;

    return ret;
}

MaterialID Material::new_clone(ResourceManager* target_resource_manager, GarbageCollectMethod garbage_collect) const {

    MaterialID ret = target_resource_manager->new_material(garbage_collect);
    assert(ret);

    auto mat = target_resource_manager->material(ret);

    for(auto pass: passes_) {
        mat->passes_.push_back(pass->new_clone(mat.get()));
    }

    mat->pass_count_ = pass_count_;

    return ret;
}

void Material::set_int_property(const std::__cxx11::string &name, int value) {
    auto& property = properties_.at(name);
    property.type = MATERIAL_PROPERTY_TYPE_INT;
    property.int_value = value;
    property.is_set = true;
}

void Material::set_float_property(const std::__cxx11::string &name, float value) {
    auto& property = properties_.at(name);
    property.type = MATERIAL_PROPERTY_TYPE_FLOAT;
    property.float_value = value;
    property.is_set = true;
}

void Material::create_int_property(const std::string &name) {
    if(!properties_.count(name)) {
        MaterialProperty new_prop;
        new_prop.type = MATERIAL_PROPERTY_TYPE_INT;
        properties_[name] = new_prop;
    }
}

void Material::create_float_property(const std::string &name) {
    if(!properties_.count(name)) {
        MaterialProperty new_prop;
        new_prop.type = MATERIAL_PROPERTY_TYPE_FLOAT;
        properties_[name] = new_prop;
    }
}

void Material::on_pass_created(MaterialPass *pass) {
    signal_material_pass_created_(id(), pass);
    signal_material_changed_(id());
}

void Material::on_pass_changed(MaterialPass *pass) {
    signal_material_changed_(id());
}

void Material::on_pass_destroyed(MaterialPass* pass) {
    signal_material_pass_destroyed_(id(), pass);
    signal_material_changed_(id());
}

}
