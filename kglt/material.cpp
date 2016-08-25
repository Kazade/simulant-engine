#include <stdexcept>
#include <cassert>

#include "window_base.h"
#include "material.h"
#include "resource_manager.h"

#ifndef KGLT_GL_VERSION_1X
#include "renderers/gl2x/gpu_program.h"
#endif

namespace kglt {

#ifdef KGLT_GL_VERSION_1X
const std::string Material::BuiltIns::TEXTURE_ONLY = "kglt/materials/opengl-1.x/texture_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_ONLY = "kglt/materials/opengl-1.x/diffuse_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_WITH_LIGHTING = "kglt/materials/opengl-1.x/diffuse_with_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE = "kglt/materials/opengl-1.x/multitexture2_modulate.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_ADD = "kglt/materials/opengl-1.x/multitexture2_add.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP = "kglt/materials/opengl-1.x/texture_with_lightmap.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP_AND_LIGHTING = "kglt/materials/opengl-1.x/texture_with_lightmap_and_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING = "kglt/materials/opengl-1.x/multitexture2_modulate_with_lighting.kglm";
#else
const std::string Material::BuiltIns::TEXTURE_ONLY = "kglt/materials/opengl-2.x/texture_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_ONLY = "kglt/materials/opengl-2.x/diffuse_only.kglm";
const std::string Material::BuiltIns::DIFFUSE_WITH_LIGHTING = "kglt/materials/opengl-2.x/diffuse_with_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE = "kglt/materials/opengl-2.x/multitexture2_modulate.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_ADD = "kglt/materials/opengl-2.x/multitexture2_add.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP = "kglt/materials/opengl-2.x/texture_with_lightmap.kglm";
const std::string Material::BuiltIns::TEXTURE_WITH_LIGHTMAP_AND_LIGHTING = "kglt/materials/opengl-2.x/texture_with_lightmap_and_lighting.kglm";
const std::string Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING = "kglt/materials/opengl-2.x/multitexture2_modulate_with_lighting.kglm";
#endif


static const std::string DEFAULT_VERT_SHADER = R"(
    attribute vec3 vertex_position;
    attribute vec4 vertex_diffuse;

    uniform mat4 modelview_projection;

    varying vec4 diffuse;

    void main() {
        diffuse = vertex_diffuse;
        gl_Position = (modelview_projection * vec4(vertex_position, 1.0));
    }
)";

static const std::string DEFAULT_FRAG_SHADER = R"(
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

    signal_material_pass_created_(id(), passes_.back().get());
    return passes_.size() - 1; //Return the index
}

MaterialPass::ptr Material::pass(uint32_t index) {
    std::lock_guard<std::mutex> lock(pass_lock_);
    return passes_.at(index);
}

#ifndef KGLT_GL_VERSION_1X
GPUProgram::ptr MaterialPass::default_program;
#endif

MaterialPass::MaterialPass(Material *material):
    material_(material),
    iteration_(ITERATE_ONCE),    
    max_iterations_(1),
    blend_(BLEND_NONE),
    depth_writes_enabled_(true),
    depth_test_enabled_(true),
    point_size_(1) {

#ifndef KGLT_GL_VERSION_1X
    //Create and build the default GPUProgram
    if(!default_program) {
        default_program = GPUProgram::create(DEFAULT_VERT_SHADER, DEFAULT_FRAG_SHADER);
    }

    program_ = GPUProgramInstance::create(default_program);
#endif
}

void MaterialPass::set_texture_unit(uint32_t texture_unit_id, TextureID tex) {
    if(!allow_textures_) {
        throw LogicError("Attempted to set a texture on a pass which prevents them");
    }

    if(texture_unit_id >= MAX_TEXTURE_UNITS) {
        L_ERROR(_F("Texture unit ID is too high. {0} >= {1}").format(texture_unit_id, MAX_TEXTURE_UNITS));
        throw LogicError(_F("Texture unit ID is too high. {0} >= {1}").format(texture_unit_id, MAX_TEXTURE_UNITS));
    }

    TextureID previous_texture;

    if(texture_unit_id >= texture_units_.size()) {
        texture_units_.resize(texture_unit_id + 1, TextureUnit(*this));
    } else {
        previous_texture = texture_units_[texture_unit_id].texture_id();
    }
    texture_units_.at(texture_unit_id) = TextureUnit(*this, tex);

    MaterialPassChangeEvent evt;
    evt.type = MATERIAL_PASS_CHANGE_TYPE_TEXTURE_UNIT_CHANGED;
    evt.texture_unit_changed.pass = this;
    evt.texture_unit_changed.old_texture_id = previous_texture;
    evt.texture_unit_changed.new_texture_id = tex;
    evt.texture_unit_changed.texture_unit = texture_unit_id;

    material->signal_material_pass_changed_(material->id(), evt);
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

#ifndef KGLT_GL_VERSION_1X
void MaterialPass::build_program_and_bind_attributes() {
    auto do_build_and_bind = [&]() {
        program->program->prepare_program();

        for(auto attribute: SHADER_AVAILABLE_ATTRS) {
            if(program->attributes->uses_auto(attribute)) {
                auto varname = program->attributes->variable_name(attribute);
                program->program->set_attribute_location(varname, attribute);
            }
        }

        program->program->build();
    };

    // If we're not in the GL thread, then make this run on the idle task and wait
    if(!GLThreadCheck::is_current()) {
        this->material->resource_manager().window->idle->run_sync(do_build_and_bind);
    } else {
        // Otherwise do this inline
        do_build_and_bind();
    }
}
#endif

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

#ifndef KGLT_GL_VERSION_1X
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
    ret->shader_sources_ = shader_sources_;
#endif

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

    return ret;
}

MaterialID Material::new_clone(ResourceManager* target_resource_manager, GarbageCollectMethod garbage_collect) const {

    MaterialID ret = target_resource_manager->new_material(garbage_collect);
    auto mat = target_resource_manager->material(ret);

    for(auto pass: passes_) {
        mat->passes_.push_back(pass->new_clone(mat.get()));
    }

    mat->pass_count_ = pass_count_;

    return ret;
}

}
