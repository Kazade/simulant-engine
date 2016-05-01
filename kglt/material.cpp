#include <stdexcept>
#include <cassert>

#include "window_base.h"
#include "material.h"
#include "resource_manager.h"
#include "gpu_program.h"

namespace kglt {

const std::string Material::BuiltIns::TEXTURE_ONLY = "kglt/materials/opengl-2.x/texture_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_ONLY = "kglt/materials/opengl-2.x/diffuse_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_WITH_LIGHTING = "kglt/materials/opengl-2.x/diffuse_with_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE = "kglt/materials/opengl-2.x/multitexture2_modulate.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_ADD = "kglt/materials/opengl-2.x/multitexture2_add.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP = "kglt/materials/opengl-2.x/texture_with_lightmap.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP_AND_LIGHTING = "kglt/materials/opengl-2.x/texture_with_lightmap_and_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING = "kglt/materials/opengl-2.x/multitexture2_modulate_with_lighting.kglm";

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

    return passes_.size() - 1; //Return the index
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

    //Create and build the default GPUProgram
    /* FIXME! Do this only once! */
    auto gpu_program = GPUProgram::create();
    gpu_program->set_shader_source(SHADER_TYPE_VERTEX, DEFAULT_VERT_SHADER);
    gpu_program->set_shader_source(SHADER_TYPE_FRAGMENT, DEFAULT_FRAG_SHADER);

    program_ = GPUProgramInstance::create(gpu_program);
}

void MaterialPass::set_texture_unit(uint32_t texture_unit_id, TextureID tex) {
    if(!allow_textures_) {
        throw LogicError("Attempted to set a texture on a pass which prevents them");
    }

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
        material->reflective_passes_.insert(this);
    } else {
        material->reflective_passes_.erase(this);
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

    ret->float_uniforms_ = float_uniforms_;
    ret->int_uniforms_ = int_uniforms_;

    auto clone_gpu_program = [](GPUProgramInstance::ptr prog) -> GPUProgramInstance::ptr {
        /* Create a new program instance using the same GPU program */
        GPUProgramInstance::ptr ret = GPUProgramInstance::create(
            prog->_program_as_shared_ptr()
        );

        for(auto& p: prog->uniforms->auto_uniforms()) {
            ret->uniforms->register_auto(p.first, p.second);
        }

        for(auto& p: prog->attributes->auto_attributes()) {
            ret->attributes->register_auto(p.first, p.second);
        }
        return ret;
    };

    ret->program_ = clone_gpu_program(program_);

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
    ret->shader_sources_ = shader_sources_;
    return ret;
}

MaterialID Material::new_clone(bool garbage_collect) const {

    // Probably the only legit use of const_cast I've ever done! The const-ness applies
    // to the source material, not the resource manager, and there's no other way to get
    // a non-const resource manager reference unless we pass it in as an argument and that
    // is nasty
    ResourceManager& tmp = const_cast<ResourceManager&>(resource_manager());

    MaterialID ret = tmp.new_material(garbage_collect);
    auto mat = tmp.material(ret);

    for(auto pass: passes_) {
        mat->passes_.push_back(pass->new_clone(mat.get()));
    }

    return ret;
}

}
