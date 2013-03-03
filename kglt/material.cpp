#include <stdexcept>
#include <cassert>

#include "material.h"

namespace kglt {

const uint32_t MAX_TEXTURE_UNITS = 8;

TextureUnit::TextureUnit():
    time_elapsed_(0),
    current_texture_(0),
    texture_unit_(0){

    kmMat4Identity(&texture_matrix_);
}

TextureUnit::TextureUnit(TextureID tex_id):
    time_elapsed_(0),
    current_texture_(0),
    texture_unit_(tex_id) {

    kmMat4Identity(&texture_matrix_);
}

TextureUnit::TextureUnit(std::vector<TextureID> textures, double duration):
    animated_texture_units_(textures),
    animated_texture_duration_(duration),
    time_elapsed_(0),
    current_texture_(0),
    texture_unit_(0) {

    kmMat4Identity(&texture_matrix_);
}

Material::Material(ResourceManager *resource_manager, MaterialID mat_id):
    Resource(resource_manager),
    generic::Identifiable<MaterialID>(mat_id) {

    new_technique(DEFAULT_MATERIAL_SCHEME); //Create the default technique
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

MaterialTechnique::MaterialTechnique(Material& mat, const std::string& scheme) {
    scheme_ = scheme;
}

uint32_t MaterialTechnique::new_pass(ShaderID shader) {
    passes_.push_back(MaterialPass::ptr(new MaterialPass(shader)));
    return passes_.size() - 1; //Return the index
}

MaterialPass& MaterialTechnique::pass(uint32_t index) {
    return *passes_.at(index);
}

MaterialPass::MaterialPass(ShaderID shader):
    shader_(shader),
    shininess_(0.0),
    iteration_(ITERATE_ONCE),    
    max_iterations_(1),
    blend_(BLEND_NONE),
    depth_writes_enabled_(true),
    depth_test_enabled_(true),
    point_size_(1),
    line_width_(1) {

}

void MaterialPass::set_texture_unit(uint32_t texture_unit_id, TextureID tex) {
    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        throw std::logic_error("Texture unit ID is too high");
    }

    if(texture_units_.size() <= texture_unit_id) {
        texture_units_.resize(texture_unit_id + 1);
    }
    texture_units_[texture_unit_id] = TextureUnit(tex);
}

void MaterialPass::set_animated_texture_unit(uint32_t texture_unit_id, const std::vector<TextureID> textures, double duration) {
    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        throw std::logic_error("Texture unit ID is too high");
    }

    if(texture_units_.size() <= texture_unit_id) {
        texture_units_.resize(texture_unit_id + 1);
    }
    texture_units_[texture_unit_id] = TextureUnit(textures, duration);
}

}
