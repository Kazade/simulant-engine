#ifndef MATERIAL_H
#define MATERIAL_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <tr1/memory>

#include "generic/identifiable.h"
#include "types.h"

namespace kglt {

class Scene;
class Material;

class TextureUnit {
public:
    TextureUnit();

    TextureUnit(TextureID tex_id);
    TextureUnit(std::vector<TextureID> textures, double duration);

    bool is_animated() const { return !animated_texture_units_.empty(); }

    TextureID texture() const {
        if(is_animated()) {
            return animated_texture_units_[current_texture_];
        } else {
            return texture_unit_;
        }
    }

    void update(double dt) {
        time_elapsed_ += dt;
        if(time_elapsed_ >= (animated_texture_duration_ / double(animated_texture_units_.size()))) {
            current_texture_++;
            time_elapsed_ = 0.0;

            if(current_texture_ == animated_texture_units_.size()) {
                current_texture_ = 0;
            }
        }
    }

private:
    std::vector<TextureID> animated_texture_units_;
    double animated_texture_duration_;
    double time_elapsed_;
    uint32_t current_texture_;

    TextureID texture_unit_;
};

class MaterialPass {
public:
    typedef std::tr1::shared_ptr<MaterialPass> ptr;

    MaterialPass(ShaderID shader);
    void set_texture_unit(uint32_t texture_unit_id, TextureID tex);
    void set_animated_texture_unit(uint32_t texture_unit_id, const std::vector<TextureID> textures, double duration);

    Colour diffuse() const { return diffuse_; }
    Colour ambient() const { return ambient_; }
    Colour specular() const { return specular_; }
    float shininess() const { return shininess_; }

private:
    ShaderID shader_;

    Colour diffuse_;
    Colour ambient_;
    Colour specular_;
    float shininess_;

    std::vector<TextureUnit> texture_units_;
};

class MaterialTechnique {
public:
    typedef std::tr1::shared_ptr<MaterialTechnique> ptr;

    MaterialTechnique(Material& mat, const std::string& scheme=DEFAULT_MATERIAL_SCHEME);
    uint32_t new_pass(ShaderID shader);
    MaterialPass& pass(uint32_t index);
    uint32_t pass_count() const { return passes_.size(); }

    const std::string& scheme() const { return scheme_; }

private:
    std::string scheme_;

    std::vector<MaterialPass::ptr> passes_;
};

class Material : public generic::Identifiable<MaterialID> {
public:
    typedef std::tr1::shared_ptr<Material> ptr;

    Material(Scene* scene, MaterialID mat_id);
    MaterialTechnique& technique(const std::string& scheme=DEFAULT_MATERIAL_SCHEME);
    MaterialTechnique& new_technique(const std::string& scheme);
    bool has_technique(const std::string& scheme) const { return techniques_.find(scheme) != techniques_.end(); }

    uint32_t technique_count() const { return techniques_.size(); }

private:
    std::map<std::string, MaterialTechnique::ptr> techniques_;
};

}

#endif // MATERIAL_H
