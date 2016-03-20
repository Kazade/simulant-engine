#ifndef MATERIAL_H
#define MATERIAL_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <set>

#include <kazbase/signals.h>
#include "generic/managed.h"
#include "kazmath/mat4.h"
#include "resource.h"
#include "loadable.h"
#include "generic/identifiable.h"
#include "generic/cloneable.h"
#include "generic/property.h"
#include "types.h"
#include "interfaces.h"

namespace kglt {

class Material;
class MaterialPass;
class GPUProgramInstance;

class TextureUnit:
    public Updateable {
public:
    TextureUnit(MaterialPass& pass);
    TextureUnit(MaterialPass& pass, TextureID tex_id);
    TextureUnit(MaterialPass& pass, std::vector<TextureID> textures, double duration);

    TextureUnit(const TextureUnit& rhs) = default;
    TextureUnit& operator=(const TextureUnit& rhs) = default;

    bool is_animated() const { return !animated_texture_units_.empty(); }

    TextureID texture_id() const;

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
        Mat4 diff;
        kmMat4Translation(&diff, amount, 0, 0);
        kmMat4Multiply(&texture_matrix_, &texture_matrix_, &diff);
    }

    void scroll_y(float amount) {
        Mat4 diff;
        kmMat4Translation(&diff, 0, amount, 0);
        kmMat4Multiply(&texture_matrix_, &texture_matrix_, &diff);
    }

    Mat4& matrix() {
        return texture_matrix_;
    }

private:
    MaterialPass* pass_;

    std::vector<TexturePtr> animated_texture_units_;
    double animated_texture_duration_;
    double time_elapsed_;
    uint32_t current_texture_;

    TexturePtr texture_unit_;
    Mat4 texture_matrix_;

    friend class MaterialPass;
    TextureUnit new_clone(MaterialPass& owner) const;
};

enum IterationType {
    ITERATE_ONCE,
    ITERATE_N,
    ITERATE_ONCE_PER_LIGHT
};


class MaterialPass:
    public Managed<MaterialPass>,
    public Updateable {
public:
    MaterialPass(Material* material);

    void set_shininess(float s) { shininess_ = s; }
    void set_ambient(const kglt::Colour& colour) { ambient_ = colour; }
    void set_diffuse(const kglt::Colour& colour) { diffuse_ = colour; }
    void set_specular(const kglt::Colour& colour) { specular_ = colour; }

    void set_texture_unit(uint32_t texture_unit_id, TextureID tex);
    void set_animated_texture_unit(uint32_t texture_unit_id, const std::vector<TextureID> textures, double duration);

    Colour diffuse() const { return diffuse_; }
    Colour ambient() const { return ambient_; }
    Colour specular() const { return specular_; }
    float shininess() const { return shininess_; }

    int32_t texture_unit_count() const { return (int) texture_units_.size(); }
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

    float point_size() const { return point_size_; }

    void set_albedo(float reflectiveness);
    float albedo() const { return albedo_; }
    bool is_reflective() const { return albedo_ > 0.0; }
    void set_reflection_texture_unit(uint8_t i) { reflection_texture_unit_ = i; }
    uint8_t reflection_texture_unit() const { return reflection_texture_unit_; }
    void set_polygon_mode(PolygonMode mode) { polygon_mode_ = mode; }
    PolygonMode polygon_mode() const { return polygon_mode_; }

    void stage_uniform(const std::string& name, const int& value) {
        int_uniforms_[name] = value;
    }

    void stage_uniform(const std::string& name, const float& value) {
        float_uniforms_[name] = value;
    }

    void apply_staged_uniforms(GPUProgram* program);
    void set_prevent_textures(bool value) { allow_textures_ = !value; }

    Property<MaterialPass, Material> material = { this, &MaterialPass::material_ };
    Property<MaterialPass, GPUProgramInstance> program = { this, &MaterialPass::program_ };

private:
    Material* material_;

    std::unordered_map<std::string, float> float_uniforms_;
    std::unordered_map<std::string, int> int_uniforms_;

    std::shared_ptr<GPUProgramInstance> program_;

    Colour diffuse_ = Colour::WHITE;
    Colour ambient_ = Colour::WHITE;
    Colour specular_ = Colour::WHITE;
    float shininess_ = 0.0;
    bool allow_textures_ = true;

    std::vector<TextureUnit> texture_units_;

    IterationType iteration_ = ITERATE_ONCE;
    uint32_t max_iterations_ = 0;

    BlendType blend_;

    bool depth_writes_enabled_ = true;
    bool depth_test_enabled_ = true;

    float point_size_;

    float albedo_ = 0.0;
    uint8_t reflection_texture_unit_ = 0;

    PolygonMode polygon_mode_ = POLYGON_MODE_FILL;

    std::map<kglt::ShaderType, unicode> shader_sources_;

    friend class Material;
    MaterialPass::ptr new_clone(Material *owner) const;
};

class Material :
    public Resource,
    public Loadable,
    public generic::Identifiable<MaterialID>,
    public Managed<Material>,
    public Updateable {

public:
    Material(ResourceManager* resource_manager, MaterialID mat_id);
    ~Material();

    void update(double dt) override;
    bool has_reflective_pass() const { return !reflective_passes_.empty(); }

    uint32_t new_pass();
    MaterialPass& pass(uint32_t index);
    uint32_t pass_count() const { return passes_.size(); }

    void set_texture_unit_on_all_passes(uint32_t texture_unit_id, TextureID tex);

    MaterialID new_clone(bool garbage_collect=true) const;

    void each(std::function<void (uint32_t, MaterialPass*)> callback) {
        for(uint32_t i = 0; i < passes_.size(); ++i) {
            callback(i, passes_[i].get());
        }
    }

private:
    std::vector<MaterialPass::ptr> passes_;
    std::set<MaterialPass*> reflective_passes_;

    sig::connection update_connection_;

    friend class MaterialPass;
};

}

#endif // MATERIAL_H
