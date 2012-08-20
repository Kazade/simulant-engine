#include <stdexcept>
#include <cassert>

#include "material.h"

namespace kglt {

const int MAX_TEXTURE_UNITS = 8;

Material::Material(Scene *scene, MaterialID mat_id):
    generic::Identifiable<MaterialID>(mat_id) {

    MaterialTechnique& def = new_technique(DEFAULT_SCHEME);
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
    shader_(shader) {

}

void MaterialPass::set_texture_unit(uint32_t texture_unit_id, TextureID tex) {
    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        throw std::logic_error("Texture unit ID is too high");
    }

    if(texture_units_.size() <= texture_unit_id) {
        texture_units_.resize(texture_unit_id + 1);
    }
    texture_units_[texture_unit_id].animated_texture_units = std::vector<TextureID>();
    texture_units_[texture_unit_id].animated_texture_duration = 0.0;
    texture_units_[texture_unit_id].texture_unit = tex;
}

void MaterialPass::set_animated_texture_unit(uint32_t texture_unit_id, const std::vector<TextureID> textures, double duration) {
    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        throw std::logic_error("Texture unit ID is too high");
    }

    if(texture_units_.size() <= texture_unit_id) {
        texture_units_.resize(texture_unit_id + 1);
    }
    texture_units_[texture_unit_id].animated_texture_units = textures;
    texture_units_[texture_unit_id].animated_texture_duration = duration;
    texture_units_[texture_unit_id].texture_unit = 0;
}

}
