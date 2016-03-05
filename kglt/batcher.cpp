#include "utils/glcompat.h"

#include "actor.h"
#include "batcher.h"

#include "light.h"
#include "material.h"
#include "stage.h"
#include "gpu_program.h"
#include "camera.h"
#include "partitioner.h"
#include "window_base.h"
#include "utils/gl_error.h"

/*
  This is the structure of the render tree:

          ShaderNode <- GLSL shader is bound
              |
          DepthNode  <- Depth testing stuff
              |
          MaterialNode <- Uniform bindings
              |
          BlendingNode <- Blending settings
              |
          RenderSettingsNode  <- Polygon mode
              |         |
            Meshes    TextureNode <- Texture units
                        |
                      Meshes

 FIXME: It's perhaps an error that texture unit grouping is so low in the tree, as texture switching
 is probably more expensive than everything else aside shader binding and moving it to below
 the shader node will likely result in improved performance.
*/

namespace kglt {

void RootGroup::bind(GPUProgram *program) {

}

ProtectedPtr<CameraProxy> RootGroup::camera() {
    return stage()->camera(camera_id_);
}

StagePtr RootGroup::stage() {
    return window_.stage(stage_id_);
}

LightGroupData generate_light_group_data(GPUProgramInstance* program_instance, ProtectedPtr<Light> light) {
    LightGroupData ret;

    auto& uniforms = program_instance->uniforms;

    ret.light_id = light->id();

    if(uniforms->uses_auto(SP_AUTO_LIGHT_GLOBAL_AMBIENT)) {
        ret.global_ambient_variable_.emplace(program_instance->uniforms->auto_variable_name(SP_AUTO_LIGHT_GLOBAL_AMBIENT));
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_POSITION)) {
        ret.light_position_variable_.emplace(uniforms->auto_variable_name(SP_AUTO_LIGHT_POSITION));
        ret.light_position_value_ = Vec4(light->absolute_position(), (light->type() == LIGHT_TYPE_DIRECTIONAL) ? 0.0 : 1.0);
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_AMBIENT)) {
        ret.light_ambient_variable_.emplace(uniforms->auto_variable_name(SP_AUTO_LIGHT_AMBIENT));
        ret.light_ambient_value_ = light->ambient();
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_DIFFUSE)) {
        ret.light_diffuse_variable_.emplace(uniforms->auto_variable_name(SP_AUTO_LIGHT_DIFFUSE));
        ret.light_diffuse_value_ = light->diffuse();
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_SPECULAR)) {
        ret.light_specular_variable_.emplace(uniforms->auto_variable_name(SP_AUTO_LIGHT_SPECULAR));
        ret.light_specular_value_ = light->specular();
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_CONSTANT_ATTENUATION)) {
        ret.light_constant_attenuation_variable_.emplace(uniforms->auto_variable_name(SP_AUTO_LIGHT_CONSTANT_ATTENUATION));
        ret.light_constant_attenuation_value_ = light->constant_attenuation();
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_LINEAR_ATTENUATION)) {
        ret.light_linear_attenuation_variable_.emplace(uniforms->auto_variable_name(SP_AUTO_LIGHT_LINEAR_ATTENUATION));
        ret.light_linear_attenuation_value_ = light->linear_attenuation();
    }

    if(uniforms->uses_auto(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION)) {
        ret.light_quadratic_attenuation_variable_.emplace(uniforms->auto_variable_name(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION));
        ret.light_quadratic_attenuation_value_ = light->quadratic_attenuation();
    }
    return ret;
}

MaterialGroupData generate_material_group_data(GPUProgramInstance* instance, MaterialPass* pass) {
    MaterialGroupData data;
    auto& uniforms = instance->uniforms;
    if(uniforms->uses_auto(SP_AUTO_MATERIAL_AMBIENT)) {
        data.ambient_variable = uniforms->auto_variable_name(SP_AUTO_MATERIAL_AMBIENT);
        data.ambient = pass->ambient();
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_DIFFUSE)) {
        data.diffuse_variable = uniforms->auto_variable_name(SP_AUTO_MATERIAL_DIFFUSE);
        data.diffuse = pass->diffuse();
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_SPECULAR)) {
        data.specular_variable = uniforms->auto_variable_name(SP_AUTO_MATERIAL_SPECULAR);
        data.specular = pass->specular();
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_SHININESS)) {
        data.shininess_variable = uniforms->auto_variable_name(SP_AUTO_MATERIAL_SHININESS);
        data.shininess = pass->shininess();
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_POINT_SIZE)) {
        data.point_size_variable = uniforms->auto_variable_name(SP_AUTO_MATERIAL_POINT_SIZE);
        data.point_size = pass->point_size();
    }

    if(uniforms->uses_auto(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS)) {
        data.active_texture_count_variable = uniforms->auto_variable_name(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS);
        data.active_texture_count = pass->texture_unit_count();
    }

    return data;
}

void RootGroup::generate_mesh_groups(RenderGroup* parent, Renderable* renderable, MaterialPass* pass, const std::vector<LightID>& lights) {
    /*
     *  Here we add the entities to the leaves of the tree. If the Renderable can return an instanced_mesh_id we create an
     *  InstancedMeshGroup, otherwise a simple basic RenderableGroup. At the moment THERE IS NO DIFFERENCE BETWEEN THESE TWO THINGS,
     *  but it does pave the way for proper geometry instancing.
     */

    uint32_t iteration_count = 1;

    auto mesh_id = renderable->instanced_mesh_id();
    auto submesh_id = renderable->instanced_submesh_id();

    bool supports_instancing = bool(mesh_id);

    if(pass->iteration() == ITERATE_N) {
        iteration_count = pass->max_iterations();
        for(uint8_t i = 0; i < iteration_count; ++i) {
            //FIXME: What exactly is this for? Should we pass an iteration counter to the shader?

            if(supports_instancing) {
                parent->get_or_create<InstancedMeshGroup>(MeshGroupData(mesh_id, submesh_id)).add(renderable, pass);
            } else {
                parent->get_or_create<RenderableGroup>(RenderableGroupData()).add(renderable, pass);
            }
        }
    } else if (pass->iteration() == ITERATE_ONCE_PER_LIGHT) {
        iteration_count = std::min<uint32_t>(lights.size(), pass->max_iterations());
        for(uint8_t i = 0; i < iteration_count; ++i) {

            auto light = stage()->light(lights[i]);
            LightGroupData data = generate_light_group_data(pass->program.get(), light);

            auto& light_node = parent->get_or_create<LightGroup>(data);
            if(supports_instancing) {
                light_node.get_or_create<InstancedMeshGroup>(MeshGroupData(mesh_id, submesh_id)).add(renderable, pass);
            } else {
                light_node.get_or_create<RenderableGroup>(RenderableGroupData()).add(renderable, pass);
            }
        }
    } else {
        if(supports_instancing) {
            parent->get_or_create<InstancedMeshGroup>(MeshGroupData(mesh_id, submesh_id)).add(renderable, pass);
        } else {
            parent->get_or_create<RenderableGroup>(RenderableGroupData()).add(renderable, pass);
        }
    }
}

void RootGroup::insert(Renderable *renderable, MaterialPass *pass, const std::vector<LightID>& lights) {
    if(!renderable->is_visible()) return;

    auto& program_instance = pass->program;

    //First, let's build the texture units
    RenderGroup* current = this;

    //Add a shader node
    current = &current->get_or_create<ShaderGroup>(ShaderGroupData(program_instance->program.get()));

    //Add a node for depth settings
    current = &current->get_or_create<DepthGroup>(
        DepthGroupData(pass->depth_test_enabled(), pass->depth_write_enabled())
    );

    //Add a node for the material properties
    current = &current->get_or_create<MaterialGroup>(generate_material_group_data(program_instance.get(), pass));

    //Add a node for the blending type
    current = &current->get_or_create<BlendGroup>(BlendGroupData(pass->blending()));

    //Add a node for the render settings
    current = &current->get_or_create<RenderSettingsGroup>(
        RenderSettingsData(pass->point_size(), pass->polygon_mode())
    );

    if(!pass->texture_unit_count()) {
        generate_mesh_groups(current, renderable, pass, lights);
    } else {
        //Add the texture-related branches of the tree under the shader(
        std::vector<GLuint> units;
        for(uint8_t tu = 0; tu < pass->texture_unit_count(); ++tu) {
            auto& unit = pass->texture_unit(tu);

            units.push_back(stage()->texture(unit.texture_id())->gl_tex());

            /*
            RenderGroup* iteration_parent = &current->get_or_create<TextureGroup>(
                TextureGroupData(tu, unit.texture_id())
            );

            if(program_instance->uniforms->uses_auto(ShaderAvailableAuto(SP_AUTO_MATERIAL_TEX_MATRIX0 + tu))) {
                auto name = program_instance->uniforms->auto_variable_name(
                    ShaderAvailableAuto(SP_AUTO_MATERIAL_TEX_MATRIX0 + tu)
                );

                iteration_parent = &iteration_parent->get_or_create<TextureMatrixGroup>(
                    TextureMatrixGroupData(tu, name, unit.matrix())
                );
            }
            generate_mesh_groups(iteration_parent, renderable, pass, lights);*/
        }

        RenderGroup* iteration_parent = &current->get_or_create<TextureGroup>(TextureGroupData(units));
        generate_mesh_groups(iteration_parent, renderable, pass, lights);
    }
}

void LightGroup::bind(GPUProgram* program) {
    if(!data_.light_id) {
        return;
    }

    RootGroup& root = static_cast<RootGroup&>(get_root());
    auto light = root.stage()->light(data_.light_id);

    if(data_.global_ambient_variable_) {
        program->set_uniform_colour(data_.global_ambient_variable_.value(), root.stage()->ambient_light());
    }

    if(data_.light_position_variable_) {
        program->set_uniform_vec4(data_.light_position_variable_.value(), data_.light_position_value_);
    }

    if(data_.light_ambient_variable_) {
        program->set_uniform_colour(data_.light_ambient_variable_.value(), data_.light_ambient_value_);
    }

    if(data_.light_diffuse_variable_) {
        program->set_uniform_colour(data_.light_diffuse_variable_.value(), data_.light_diffuse_value_);
    }

    if(data_.light_specular_variable_) {
        program->set_uniform_colour(data_.light_specular_variable_.value(), data_.light_specular_value_);
    }

    if(data_.light_constant_attenuation_variable_) {
        program->set_uniform_float(data_.light_constant_attenuation_variable_.value(), data_.light_constant_attenuation_value_);
    }

    if(data_.light_linear_attenuation_variable_) {
        program->set_uniform_float(data_.light_linear_attenuation_variable_.value(), data_.light_linear_attenuation_value_);
    }

    if(data_.light_quadratic_attenuation_variable_) {
        program->set_uniform_float(data_.light_quadratic_attenuation_variable_.value(), data_.light_quadratic_attenuation_value_);
    }
}

void LightGroup::unbind(GPUProgram* program) {

}

void InstancedMeshGroup::bind(GPUProgram* program) {}
void InstancedMeshGroup::unbind(GPUProgram* program) {}

void ShaderGroup::bind(GPUProgram* program) {
    RootGroup& root = static_cast<RootGroup&>(get_root());

    root.set_current_program(data_.shader_);
    data_.shader_->build();
    data_.shader_->activate();
}

std::size_t ShaderGroupData::do_hash() const {
    size_t seed = 0;
    hash_combine(seed, typeid(ShaderGroupData).name());
    hash_combine(seed, shader_->md5());
    return seed;
}

void ShaderGroup::unbind(GPUProgram *program) {

}

void DepthGroup::bind(GPUProgram *program) {
    if(data_.depth_test) {
        GLCheck(glEnable, GL_DEPTH_TEST);
    } else {
        GLCheck(glDisable, GL_DEPTH_TEST);
    }

    if(data_.depth_write) {
        GLCheck(glDepthMask, GL_TRUE);
    } else {
        GLCheck(glDepthMask, GL_FALSE);
    }
}

void DepthGroup::unbind(GPUProgram *program) {
    if(data_.depth_test) {
        GLCheck(glDisable, GL_DEPTH_TEST);
    }
}

void TextureGroup::bind(GPUProgram* program) {
    for(uint8_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        GLuint unit = data_.textures[i];
        GLCheck(glActiveTexture, GL_TEXTURE0 + i);
        GLCheck(glBindTexture, GL_TEXTURE_2D, unit);
    }
}

void TextureGroup::unbind(GPUProgram *program) {
    for(uint8_t i = 0; i < MAX_TEXTURE_UNITS; ++i) {
        GLCheck(glActiveTexture, GL_TEXTURE0 + i);
        GLCheck(glBindTexture, GL_TEXTURE_2D, 0);
    }
}

void TextureMatrixGroup::bind(GPUProgram *program) {
    program->set_uniform_mat4x4(data_.matrix_variable, data_.matrix);
}

void TextureMatrixGroup::unbind(GPUProgram* program) {

}

void MaterialGroup::bind(GPUProgram* program) {
    if(data_.ambient_variable) {
        program->set_uniform_colour(data_.ambient_variable.value(), data_.ambient);
    }

    if(data_.diffuse_variable) {
        program->set_uniform_colour(data_.diffuse_variable.value(), data_.diffuse);
    }

    if(data_.specular_variable) {
        program->set_uniform_colour(data_.specular_variable.value(), data_.specular);
    }

    if(data_.shininess_variable) {
        program->set_uniform_float(data_.shininess_variable.value(), data_.shininess);
    }

    if(data_.point_size_variable) {
        program->set_uniform_float(data_.point_size_variable.value(), data_.point_size);
    }

    if(data_.active_texture_count_variable) {
        program->set_uniform_int(data_.active_texture_count_variable.value(), data_.active_texture_count);
    }
}

void MaterialGroup::unbind(GPUProgram *program) {

}

void BlendGroup::bind(GPUProgram* program) {
    if(data_.type == BLEND_NONE) {
        GLCheck(glDisable, GL_BLEND);
        return;
    }

    GLCheck(glEnable, GL_BLEND);
    switch(data_.type) {
        case BLEND_ADD: GLCheck(glBlendFunc, GL_ONE, GL_ONE);
        break;
        case BLEND_ALPHA: GLCheck(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
        case BLEND_COLOUR: GLCheck(glBlendFunc, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
        break;
        case BLEND_MODULATE: GLCheck(glBlendFunc, GL_DST_COLOR, GL_ZERO);
        break;
        case BLEND_ONE_ONE_MINUS_ALPHA: GLCheck(glBlendFunc, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    default:
        throw ValueError("Invalid blend type specified");
    }
}

void BlendGroup::unbind(GPUProgram *program) {
    GLCheck(glDisable, GL_BLEND);
}

void RenderSettingsGroup::bind(GPUProgram* program) {
#ifndef __ANDROID__
    GLCheck(glPointSize, data_.point_size);
#else
    L_WARN_ONCE("On GLES glPointSize doesn't exist");
#endif

#ifndef __ANDROID__
    switch(data_.polygon_mode) {
        case POLYGON_MODE_FILL: GLCheck(glPolygonMode, GL_FRONT_AND_BACK, GL_FILL);
        break;
        case POLYGON_MODE_LINE: GLCheck(glPolygonMode, GL_FRONT_AND_BACK, GL_LINE);
        break;
        case POLYGON_MODE_POINT: GLCheck(glPolygonMode, GL_FRONT_AND_BACK, GL_POINT);
        break;
    default:
        throw ValueError("Invalid polygon mode specified");
    }
#else
    if(data_.polygon_mode != POLYGON_MODE_FILL) {
        L_WARN_ONCE("On GLES glPolygonMode doesn't exist");
    }
#endif

}

void RenderSettingsGroup::unbind(GPUProgram *program) {
#ifndef __ANDROID__
    GLCheck(glPointSize, 1);
    GLCheck(glPolygonMode, GL_FRONT_AND_BACK, GL_FILL);
#endif

}

}
