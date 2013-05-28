#include <GLee.h>

#include "entity.h"
#include "batcher.h"

#include "light.h"
#include "material.h"
#include "subscene.h"
#include "shader.h"
#include "camera.h"
#include "partitioner.h"

namespace kglt {

void RootGroup::bind() {
}

void RootGroup::generate_mesh_groups(RenderGroup* parent, SubEntity& ent, MaterialPass& pass) {
    Vec3 pos;
    std::vector<LightID> lights = subscene().partitioner().lights_within_range(pos);
    uint32_t iteration_count = 1;
    if(pass.iteration() == ITERATE_N) {
        iteration_count = pass.max_iterations();
        for(uint8_t i = 0; i < iteration_count; ++i) {
            //FIXME: What exactly is this for? Should we pass an iteration counter to the shader?
            parent->get_or_create<MeshGroup>(MeshGroupData(ent._parent().mesh_id(), ent.submesh_id())).add(&ent);
        }
    } else if (pass.iteration() == ITERATE_ONCE_PER_LIGHT) {
        iteration_count = std::min<uint32_t>(lights.size(), pass.max_iterations());
        for(uint8_t i = 0; i < iteration_count; ++i) {
            parent->get_or_create<LightGroup>(LightGroupData(&subscene().light(lights[i]))).
                    get_or_create<MeshGroup>(MeshGroupData(ent._parent().mesh_id(), ent.submesh_id())).add(&ent);
        }
    } else {
        parent->get_or_create<MeshGroup>(MeshGroupData(ent._parent().mesh_id(), ent.submesh_id())).add(&ent);
    }
}

void RootGroup::insert(SubEntity &ent, uint8_t pass_number) {
    if(!ent._parent().is_visible()) return;

    //Get the material for the entity, this is used to build the tree
    MaterialPtr mat = subscene().material(ent.material_id()).lock();

    MaterialPass& pass = mat->technique().pass(pass_number);

    //First, let's build the texture units
    RenderGroup* current = this;

    //Add a shader node
    current = &current->get_or_create<ShaderGroup>(ShaderGroupData(pass.__shader()));

    //Add a node for depth settings
    current = &current->get_or_create<DepthGroup>(DepthGroupData(pass.depth_test_enabled(), pass.depth_write_enabled()));

    //Add a node for the material properties
    current = &current->get_or_create<MaterialGroup>(MaterialGroupData(pass.ambient(), pass.diffuse(), pass.specular(), pass.shininess(), pass.texture_unit_count()));

    //Add a node for the blending type
    current = &current->get_or_create<BlendGroup>(BlendGroupData(pass.blending()));

    //Add a node for the render settings
    current = &current->get_or_create<RenderSettingsGroup>(RenderSettingsData(pass.line_width(), pass.point_size()));

    //FIXME: This code is duplicated below and that's bollocks
    if(!pass.texture_unit_count()) {
        generate_mesh_groups(current, ent, pass);
    } else {
        //Add the texture-related branches of the tree under the shader(
        for(uint8_t tu = 0; tu < pass.texture_unit_count(); ++tu) {
            RenderGroup* iteration_parent = &current->get_or_create<TextureGroup>(TextureGroupData(tu, pass.texture_unit(tu).texture_id())).
                     get_or_create<TextureMatrixGroup>(TextureMatrixGroupData(tu, pass.texture_unit(tu).matrix()));

            generate_mesh_groups(iteration_parent, ent, pass);
        }
    }
}

void LightGroup::bind() {
    Light* light = data_.light;
    if(!light) {
        return;
    }

    ShaderProgram* active_shader = ShaderProgram::active_shader();
    assert(active_shader);

    ShaderParams& params = active_shader->params();

    if(params.uses_auto(SP_AUTO_LIGHT_POSITION)) {
        Vec4 light_pos = Vec4(light->absolute_position(), (light->type() == LIGHT_TYPE_DIRECTIONAL) ? 0.0 : 1.0);

        params.set_vec4(
            params.auto_uniform_variable_name(SP_AUTO_LIGHT_POSITION),
            light_pos
        );
    }

    if(params.uses_auto(SP_AUTO_LIGHT_AMBIENT)) {
        params.set_colour(
            params.auto_uniform_variable_name(SP_AUTO_LIGHT_AMBIENT),
            light->ambient()
        );
    }

    if(params.uses_auto(SP_AUTO_LIGHT_DIFFUSE)) {
        params.set_colour(
            params.auto_uniform_variable_name(SP_AUTO_LIGHT_DIFFUSE),
            light->diffuse()
        );
    }

    if(params.uses_auto(SP_AUTO_LIGHT_SPECULAR)) {
        params.set_colour(
            params.auto_uniform_variable_name(SP_AUTO_LIGHT_SPECULAR),
            light->specular()
        );
    }

    if(params.uses_auto(SP_AUTO_LIGHT_CONSTANT_ATTENUATION)) {
        params.set_float(
            params.auto_uniform_variable_name(SP_AUTO_LIGHT_CONSTANT_ATTENUATION),
            light->constant_attenuation()
        );
    }

    if(params.uses_auto(SP_AUTO_LIGHT_LINEAR_ATTENUATION)) {
        params.set_float(
            params.auto_uniform_variable_name(SP_AUTO_LIGHT_LINEAR_ATTENUATION),
            light->linear_attenuation()
        );
    }

    if(params.uses_auto(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION)) {
        params.set_float(
            params.auto_uniform_variable_name(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION),
            light->quadratic_attenuation()
        );
    }
}

void LightGroup::unbind() {

}

void MeshGroup::bind() {

}

void MeshGroup::unbind() {

}

void ShaderGroup::bind() {
    RootGroup& root = static_cast<RootGroup&>(get_root());

    ShaderProgram* s = data_.shader_;
    s->activate(); //Activate the shader

    //Pass in the global ambient here, as it's the earliest place
    //in the tree we can, and it's a global value
    ShaderParams& params = s->params();

    if(params.uses_auto(SP_AUTO_LIGHT_GLOBAL_AMBIENT)) {
        params.set_colour(
            params.auto_uniform_variable_name(SP_AUTO_LIGHT_GLOBAL_AMBIENT),
            root.subscene().ambient_light()
        );
    }
}

std::size_t ShaderGroupData::hash() const {
    size_t seed = 0;
    hash_combine(seed, typeid(ShaderGroupData).name());
    hash_combine(seed, shader_->id().value());
    return seed;
}

void ShaderGroup::unbind() {

}

void DepthGroup::bind() {
    if(data_.depth_test) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if(data_.depth_write) {
        glDepthMask(GL_TRUE);
    } else {
        glDepthMask(GL_FALSE);
    }
}

void DepthGroup::unbind() {
    if(data_.depth_test) {
        glDisable(GL_DEPTH_TEST);
    }
}

void TextureGroup::bind() {
    glActiveTexture(GL_TEXTURE0 + data_.unit);
    RootGroup& root = static_cast<RootGroup&>(get_root());
    glBindTexture(GL_TEXTURE_2D, root.subscene().texture(data_.texture_id).lock()->gl_tex());
}

void TextureGroup::unbind() {
    glActiveTexture(GL_TEXTURE0 + data_.unit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureMatrixGroup::bind() {
    ShaderProgram* active_shader = ShaderProgram::active_shader();
    assert(active_shader);

    ShaderParams& params = active_shader->params();

    if(params.uses_auto(ShaderAvailableAuto(SP_AUTO_MATERIAL_TEX_MATRIX0 + data_.unit))) {
        params.set_mat4x4(
            params.auto_uniform_variable_name(ShaderAvailableAuto(SP_AUTO_MATERIAL_TEX_MATRIX0 + data_.unit)),
            data_.matrix
        );
    }
}

void TextureMatrixGroup::unbind() {

}

void MaterialGroup::bind() {
    ShaderProgram* active_shader = ShaderProgram::active_shader();
    assert(active_shader);

    ShaderParams& params = active_shader->params();


    if(params.uses_auto(SP_AUTO_MATERIAL_AMBIENT)) {
        params.set_colour(
            params.auto_uniform_variable_name(SP_AUTO_MATERIAL_AMBIENT),
            data_.ambient
        );
    }

    if(params.uses_auto(SP_AUTO_MATERIAL_DIFFUSE)) {
        params.set_colour(
            params.auto_uniform_variable_name(SP_AUTO_MATERIAL_DIFFUSE),
            data_.diffuse
        );
    }

    if(params.uses_auto(SP_AUTO_MATERIAL_SPECULAR)) {
        params.set_colour(
            params.auto_uniform_variable_name(SP_AUTO_MATERIAL_SPECULAR),
            data_.specular
        );
    }

    if(params.uses_auto(SP_AUTO_MATERIAL_SHININESS)) {
        params.set_float(
            params.auto_uniform_variable_name(SP_AUTO_MATERIAL_SHININESS),
            data_.shininess
        );
    }

    if(params.uses_auto(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS)) {
        params.set_int(
            params.auto_uniform_variable_name(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS),
            data_.active_texture_count
        );
    }
}

void MaterialGroup::unbind() {

}

void BlendGroup::bind() {
    if(data_.type == BLEND_NONE) {
        glDisable(GL_BLEND);
        return;
    }

    glEnable(GL_BLEND);
    switch(data_.type) {
        case BLEND_ADD: glBlendFunc(GL_ONE, GL_ONE);
        break;
        case BLEND_ALPHA: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
        case BLEND_COLOUR: glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
        break;
        case BLEND_MODULATE: glBlendFunc(GL_DST_COLOR, GL_ZERO);
        break;
        case BLEND_ONE_ONE_MINUS_ALPHA: glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    default:
        throw ValueError("Invalid blend type specified");
    }
}

void BlendGroup::unbind() {
    glDisable(GL_BLEND);
}

void RenderSettingsGroup::bind() {
    glPointSize(data_.point_size);
    glLineWidth(data_.line_width);
}

void RenderSettingsGroup::unbind() {
    glPointSize(1);
    glLineWidth(1);
}

}
