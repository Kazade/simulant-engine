#include <stdexcept>
#include <cassert>

#include "window_base.h"
#include "material.h"
#include "resource_manager.h"
#include "scene.h"

namespace kglt {

const uint32_t MAX_TEXTURE_UNITS = 8;

TextureUnit::TextureUnit(MaterialPass &pass):
    pass_(&pass),
    time_elapsed_(0),
    current_texture_(0) {

    kmMat4Identity(&texture_matrix_);

    //Initialize the texture unit to the default texture
    ResourceManager& rm = pass.technique().material().resource_manager();
    texture_unit_ = rm.texture(rm.scene().default_texture_id()).__object;
}

TextureUnit::TextureUnit(MaterialPass &pass, TextureID tex_id):
    pass_(&pass),
    time_elapsed_(0),
    current_texture_(0) {

    kmMat4Identity(&texture_matrix_);

    //Initialize the texture unit
    ResourceManager& rm = pass.technique().material().resource_manager();
    texture_unit_ = rm.texture(tex_id).__object;
}

TextureUnit::TextureUnit(MaterialPass &pass, std::vector<TextureID> textures, double duration):
    pass_(&pass),
    animated_texture_duration_(duration),
    time_elapsed_(0),
    current_texture_(0),
    texture_unit_(0) {

    kmMat4Identity(&texture_matrix_);

    ResourceManager& rm = pass.technique().material().resource_manager();

    for(TextureID tid: textures) {
        animated_texture_units_.push_back(rm.texture(tid).__object);
    }
}

TextureID TextureUnit::texture_id() const {
    if(is_animated()) {
        return animated_texture_units_[current_texture_]->id();
    } else {
        return texture_unit_->id();
    }
}

Material::Material(ResourceManager *resource_manager, MaterialID mat_id):
    Resource(resource_manager),
    generic::Identifiable<MaterialID>(mat_id) {

    new_technique(DEFAULT_MATERIAL_SCHEME); //Create the default technique

    update_connection_ = resource_manager->window().signal_step().connect(std::bind(&Material::update, this, std::placeholders::_1));
}

Material::~Material() {
    update_connection_.disconnect();
}

MaterialTechnique& Material::technique(const std::string& scheme) {
    if(!has_technique(scheme)) {
        throw std::logic_error("No such technique with scheme: " + scheme);
    }

    return *techniques_[scheme];
}

MaterialTechnique& Material::new_technique(const std::string& scheme) {
    if(has_technique(scheme)) {
        throw std::logic_error("Technique with scheme already exists: " + scheme);
    }

    techniques_[scheme].reset(new MaterialTechnique(*this, scheme));
    return technique(scheme);
}

uint32_t MaterialTechnique::new_pass(ShaderID shader) {
    passes_.push_back(MaterialPass::ptr(new MaterialPass(*this, shader)));
    return passes_.size() - 1; //Return the index
}

MaterialPass& MaterialTechnique::pass(uint32_t index) {
    return *passes_.at(index);
}

MaterialPass::MaterialPass(MaterialTechnique& technique, ShaderID shader):
    technique_(technique),
    iteration_(ITERATE_ONCE),    
    max_iterations_(1),
    blend_(BLEND_NONE),
    depth_writes_enabled_(true),
    depth_test_enabled_(true),
    point_size_(1) {

    if(!shader) {
        throw LogicError("You must specify a shader for a material pass");
    } else {
        ResourceManager& rm = technique_.material().resource_manager();
        shader_ = rm.shader(shader).lock();
    }
}

ShaderID MaterialPass::shader_id() const {
    return shader_->id();
}

void MaterialPass::set_texture_unit(uint32_t texture_unit_id, TextureID tex) {
    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        throw std::logic_error("Texture unit ID is too high");
    }

    if(texture_units_.size() <= texture_unit_id) {
        texture_units_.resize(texture_unit_id + 1, TextureUnit(*this));
    }
    texture_units_[texture_unit_id] = TextureUnit(*this, tex);
}

void MaterialPass::set_animated_texture_unit(uint32_t texture_unit_id, const std::vector<TextureID> textures, double duration) {
    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        throw std::logic_error("Texture unit ID is too high");
    }

    if(texture_units_.size() <= texture_unit_id) {
        texture_units_.resize(texture_unit_id + 1, TextureUnit(*this));
    }
    texture_units_[texture_unit_id] = TextureUnit(*this, textures, duration);
}

void MaterialPass::set_iteration(IterationType iter_type, uint32_t max) {
    iteration_ = iter_type;
    max_iterations_ = max;
}

void MaterialPass::set_albedo(float reflectiveness) {
    albedo_ = reflectiveness;
    if(is_reflective()) {
        technique_.reflective_passes_.insert(this);
    } else {
        technique_.reflective_passes_.erase(this);
    }
}

//Assignment stuff
MaterialTechnique::MaterialTechnique(Material& mat, const std::string& scheme):
    material_(mat) {
    scheme_ = scheme;
}

MaterialTechnique::MaterialTechnique(const MaterialTechnique& rhs):
    material_(rhs.material_) {

    //FIXME: Make this reentrant (call operator=?)

    scheme_ = rhs.scheme_;
    passes_.clear();
    reflective_passes_.clear();

    for(MaterialPass::ptr pass: rhs.passes_) {
        passes_.push_back(MaterialPass::ptr(new MaterialPass(*pass)));

        if(rhs.reflective_passes_.find(pass.get()) != rhs.reflective_passes_.end()) {
            reflective_passes_.insert(passes_[passes_.size()-1].get());
        }
    }
}

MaterialTechnique& MaterialTechnique::operator=(const MaterialTechnique& rhs){
    //FIXME: Make this rentrant
    material_ = rhs.material_;
    scheme_ = rhs.scheme_;
    passes_.clear();
    reflective_passes_.clear();

    for(MaterialPass::ptr pass: rhs.passes_) {
        passes_.push_back(MaterialPass::ptr(new MaterialPass(*pass)));

        if(rhs.reflective_passes_.find(pass.get()) != rhs.reflective_passes_.end()) {
            reflective_passes_.insert(passes_[passes_.size()-1].get());
        }
    }

    return *this;
}

Material& Material::operator=(const Material& rhs) {        
    if(this == &rhs) {
        return *this;
    }

    std::unordered_map<std::string, MaterialTechnique::ptr> new_techniques;

    for(auto p: rhs.techniques_) {
        assert(p.second.get());
        new_techniques[p.first] = MaterialTechnique::ptr(new MaterialTechnique(*p.second));
    }

    //Use std::swap to make reentrant
    std::swap(techniques_, new_techniques);

    return *this;
}

void Material::update(double dt) {
    for(auto it = techniques_.begin(); it != techniques_.end(); ++it) {
        assert((*it).second);

        (*it).second->update(dt);
    }
}


}
