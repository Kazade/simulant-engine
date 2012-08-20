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

const std::string DEFAULT_SCHEME = "scheme";

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

    void update_texture_animation(double dt);

    struct TextureUnit {
        std::vector<TextureID> animated_texture_units;
        double animated_texture_duration;

        TextureID texture_unit;

        bool is_animated() const { return !animated_texture_units.empty(); }
    };

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

    MaterialTechnique(Material& mat, const std::string& scheme=DEFAULT_SCHEME);
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
    MaterialTechnique& technique(const std::string& scheme=DEFAULT_SCHEME);
    MaterialTechnique& new_technique(const std::string& scheme);
    bool has_technique(const std::string& scheme) const { return techniques_.find(scheme) != techniques_.end(); }

    uint32_t technique_count() const { return techniques_.size(); }

private:
    std::map<std::string, MaterialTechnique::ptr> techniques_;
};

}

#endif // MATERIAL_H
