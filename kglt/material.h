#ifndef MATERIAL_H
#define MATERIAL_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <tr1/memory>

#include "kazmath/mat4.h"
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

    void scroll_x(float amount) {
        kmMat4 diff;
        kmMat4Translation(&diff, amount, 0, 0);
        kmMat4Multiply(&texture_matrix_, &texture_matrix_, &diff);
    }

    void scroll_y(float amount) {
        kmMat4 diff;
        kmMat4Translation(&diff, 0, amount, 0);
        kmMat4Multiply(&texture_matrix_, &texture_matrix_, &diff);
    }

    kmMat4& matrix() {
        return texture_matrix_;
    }

private:
    std::vector<TextureID> animated_texture_units_;
    double animated_texture_duration_;
    double time_elapsed_;
    uint32_t current_texture_;

    TextureID texture_unit_;
    kmMat4 texture_matrix_;
};

enum IterationType {
    ITERATE_ONCE,
    ITERATE_N,
    ITERATE_ONCE_PER_LIGHT
};

enum BlendType {
    BLEND_NONE,
    BLEND_ADD,
    BLEND_MODULATE,
    BLEND_COLOUR,
    BLEND_ALPHA
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

    ShaderID shader() const { return shader_; }

    uint32_t texture_unit_count() const { return texture_units_.size(); }
    TextureUnit& texture_unit(uint32_t index) { return texture_units_.at(index); }

    void update(double dt) {
        for(TextureUnit& t: texture_units_) {
            t.update(dt);
        }
    }

    IterationType iteration() const { return iteration_; }
    uint32_t max_iterations() const { return max_iterations_; }
    void set_iteration(IterationType iter_type, uint32_t max=1) {
        iteration_ = iter_type;
        max_iterations_ = max;
    }

    void set_blending(BlendType blend) { blend_ = blend; }
    BlendType blending() { return blend_; }
private:
    ShaderID shader_;

    Colour diffuse_;
    Colour ambient_;
    Colour specular_;
    float shininess_;

    std::vector<TextureUnit> texture_units_;

    IterationType iteration_;
    uint32_t max_iterations_;

    BlendType blend_;
};

class MaterialTechnique {
public:
    typedef std::tr1::shared_ptr<MaterialTechnique> ptr;

    MaterialTechnique(Material& mat, const std::string& scheme=DEFAULT_MATERIAL_SCHEME);
    uint32_t new_pass(ShaderID shader);
    MaterialPass& pass(uint32_t index);
    uint32_t pass_count() const { return passes_.size(); }

    const std::string& scheme() const { return scheme_; }

    void update(double dt) {
        for(MaterialPass::ptr& p: passes_) {
            p->update(dt);
        }
    }

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

    void update(double dt) {
        for(std::pair<std::string, MaterialTechnique::ptr> p: techniques_) {
            p.second->update(dt);
        }
    }

private:
    std::map<std::string, MaterialTechnique::ptr> techniques_;
};

}

#endif // MATERIAL_H
