#include <stdexcept>
#include <cassert>

#include "window_base.h"
#include "material.h"
#include "resource_manager.h"
#include "gpu_program.h"

namespace kglt {

static const unicode DEFAULT_VERT_SHADER = R"(
    attribute vec3 vertex_position;
    attribute vec4 vertex_diffuse;

    uniform mat4 modelview_projection;

    varying vec4 diffuse;

    void main() {
        diffuse = vertex_diffuse;
        gl_Position = (modelview_projection * vec4(vertex_position, 1.0));
    }
)";

static const unicode DEFAULT_FRAG_SHADER = R"(
    varying vec4 diffuse;
    void main() {
        gl_FragColor = diffuse;
    }
)";

static const uint32_t MAX_TEXTURE_UNITS = 8;

TextureUnit::TextureUnit(MaterialPass &pass):
    pass_(&pass),
    time_elapsed_(0),
    current_texture_(0) {

    kmMat4Identity(&texture_matrix_);

    //Initialize the texture unit to the default texture
    ResourceManager& rm = pass.material().resource_manager();
    texture_unit_ = rm.texture(rm.default_texture_id()).__object;
}

TextureUnit::TextureUnit(MaterialPass &pass, TextureID tex_id):
    pass_(&pass),
    time_elapsed_(0),
    current_texture_(0) {

    kmMat4Identity(&texture_matrix_);

    //Initialize the texture unit
    ResourceManager& rm = pass.material().resource_manager();
    texture_unit_ = rm.texture(tex_id).__object;
}

TextureUnit::TextureUnit(MaterialPass &pass, std::vector<TextureID> textures, double duration):
    pass_(&pass),
    animated_texture_duration_(duration),
    time_elapsed_(0),
    current_texture_(0),
    texture_unit_(0) {

    kmMat4Identity(&texture_matrix_);

    ResourceManager& rm = pass.material().resource_manager();

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

    update_connection_ = resource_manager->window().signal_step().connect(std::bind(&Material::update, this, std::placeholders::_1));
}

Material::~Material() {
    update_connection_.disconnect();
}

uint32_t Material::new_pass() {
    passes_.push_back(MaterialPass::ptr(new MaterialPass(*this)));
    return passes_.size() - 1; //Return the index
}

MaterialPass& Material::pass(uint32_t index) {
    return *passes_.at(index);
}

MaterialPass::MaterialPass(Material &material):
    material_(material),
    iteration_(ITERATE_ONCE),    
    max_iterations_(1),
    blend_(BLEND_NONE),
    depth_writes_enabled_(true),
    depth_test_enabled_(true),
    point_size_(1) {

    //Create and build the default GPUProgram
    program_ = GPUProgram::create();
    program_->set_shader_source(SHADER_TYPE_VERTEX, DEFAULT_VERT_SHADER);
    program_->set_shader_source(SHADER_TYPE_FRAGMENT, DEFAULT_FRAG_SHADER);
}

void MaterialPass::set_texture_unit(uint32_t texture_unit_id, TextureID tex) {
    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        L_ERROR(_u("Texture unit ID is too high. {0} >= {1}").format(texture_unit_id, MAX_TEXTURE_UNITS));
        throw LogicError(_u("Texture unit ID is too high. {0} >= {1}").format(texture_unit_id, MAX_TEXTURE_UNITS).encode());
    }

    if(texture_unit_id >= texture_units_.size()) {
        texture_units_.resize(texture_unit_id + 1, TextureUnit(*this));
    }
    texture_units_.at(texture_unit_id) = TextureUnit(*this, tex);
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
        material().reflective_passes_.insert(this);
    } else {
        material().reflective_passes_.erase(this);
    }
}

void Material::update(double dt) {
    for(auto& p: passes_) {
        p->update(dt);
    }
}


}
