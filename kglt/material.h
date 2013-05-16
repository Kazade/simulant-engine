#ifndef MATERIAL_H
#define MATERIAL_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <tr1/memory>
#include <tr1/unordered_map>
#include <set>

#include "kazmath/mat4.h"
#include "resource.h"
#include "loadable.h"
#include "generic/identifiable.h"
#include "types.h"

namespace kglt {

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
        if(!is_animated()) return;

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



class MaterialTechnique;

class MaterialPass {
public:
    typedef std::shared_ptr<MaterialPass> ptr;

    MaterialPass(MaterialTechnique& technique, ShaderID shader);
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
    void set_iteration(IterationType iter_type, uint32_t max=8);
    void set_blending(BlendType blend) { blend_ = blend; }
    BlendType blending() { return blend_; }

    void set_depth_write_enabled(bool value=true) {
        depth_writes_enabled_ = value;
    }

    bool depth_write_enabled() const { return depth_writes_enabled_; }

    void set_depth_test_enabled(bool value=true) {
        depth_test_enabled_ = value;
    }
    bool depth_test_enabled() const { return depth_test_enabled_; }

    void set_point_size(float ps) { point_size_ = ps; }
    void set_line_width(float lw) { line_width_ = lw; }

    float point_size() const { return point_size_; }
    float line_width() const { return line_width_; }

    void set_albedo(float reflectiveness);
    float albedo() const { return albedo_; }
    bool is_reflective() const { return albedo_ > 0.0; }
    void set_reflection_texture_unit(uint8_t i) { reflection_texture_unit_ = i; }
    uint8_t reflection_texture_unit() const { return reflection_texture_unit_; }

private:
    MaterialTechnique& technique_;

    ShaderID shader_;

    Colour diffuse_;
    Colour ambient_;
    Colour specular_;
    float shininess_;

    std::vector<TextureUnit> texture_units_;

    IterationType iteration_;
    uint32_t max_iterations_;

    BlendType blend_;

    bool depth_writes_enabled_;
    bool depth_test_enabled_;    

    float point_size_;
    float line_width_;

    float albedo_;
    uint8_t reflection_texture_unit_;
};

class MaterialTechnique {
public:
    typedef std::shared_ptr<MaterialTechnique> ptr;

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

    bool has_reflective_pass() const { return !reflective_passes_.empty(); }

private:
    friend class MaterialPass;

    std::string scheme_;
    std::vector<MaterialPass::ptr> passes_;
    std::set<MaterialPass*> reflective_passes_;
};

class Material :
    public Resource,
    public Loadable,
    public generic::Identifiable<MaterialID> {

public:
    typedef std::shared_ptr<Material> ptr;

    Material(ResourceManager* resource_manager, MaterialID mat_id);
    MaterialTechnique& technique(const std::string& scheme=DEFAULT_MATERIAL_SCHEME);
    MaterialTechnique& new_technique(const std::string& scheme);
    bool has_technique(const std::string& scheme) const { return techniques_.find(scheme) != techniques_.end(); }

    uint32_t technique_count() const { return techniques_.size(); }

    void update(double dt) {
        std::tr1::unordered_map<std::string, MaterialTechnique::ptr>::iterator it =  techniques_.begin();
        for(; it != techniques_.end(); ++it) {
            (*it).second->update(dt);
        }
    }

private:
    std::tr1::unordered_map<std::string, MaterialTechnique::ptr> techniques_;
};

}

#endif // MATERIAL_H
