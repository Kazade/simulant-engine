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

namespace kglt {

void RootGroup::bind(GPUProgram *program) {
}

ProtectedPtr<CameraProxy> RootGroup::camera() {
    return stage()->camera(camera_id_);
}

StagePtr RootGroup::stage() {
    return window_.stage(stage_id_);
}

void RootGroup::generate_mesh_groups(RenderGroup* parent, Renderable &ent, MaterialPass& pass) {
    /*
     *  Here we add the entities to the leaves of the tree. If the Renderable can return an instanced_mesh_id we create an
     *  InstancedMeshGroup, otherwise a simple basic RenderableGroup. At the moment THERE IS NO DIFFERENCE BETWEEN THESE TWO THINGS,
     *  but it does pave the way for proper geometry instancing.
     */

    Vec3 pos;
    std::vector<LightID> lights = stage()->partitioner().lights_within_range(pos);
    uint32_t iteration_count = 1;

    auto mesh_id = ent.instanced_mesh_id();
    auto submesh_id = ent.instanced_submesh_id();

    bool supports_instancing = bool(mesh_id);

    if(pass.iteration() == ITERATE_N) {
        iteration_count = pass.max_iterations();
        for(uint8_t i = 0; i < iteration_count; ++i) {
            //FIXME: What exactly is this for? Should we pass an iteration counter to the shader?

            if(supports_instancing) {
                parent->get_or_create<InstancedMeshGroup>(MeshGroupData(mesh_id, submesh_id)).add(&ent, &pass);
            } else {
                parent->get_or_create<RenderableGroup>(RenderableGroupData()).add(&ent, &pass);
            }
        }
    } else if (pass.iteration() == ITERATE_ONCE_PER_LIGHT) {
        iteration_count = std::min<uint32_t>(lights.size(), pass.max_iterations());
        for(uint8_t i = 0; i < iteration_count; ++i) {

            auto& light_node = parent->get_or_create<LightGroup>(LightGroupData(lights[i]));
            if(supports_instancing) {
                light_node.get_or_create<InstancedMeshGroup>(MeshGroupData(mesh_id, submesh_id)).add(&ent, &pass);
            } else {
                light_node.get_or_create<RenderableGroup>(RenderableGroupData()).add(&ent, &pass);
            }
        }
    } else {
        if(supports_instancing) {
            parent->get_or_create<InstancedMeshGroup>(MeshGroupData(mesh_id, submesh_id)).add(&ent, &pass);
        } else {
            parent->get_or_create<RenderableGroup>(RenderableGroupData()).add(&ent, &pass);
        }
    }
}

void RootGroup::insert(Renderable &ent, uint8_t pass_number) {
    if(!ent.is_visible()) return;

    //Get the material for the actor, this is used to build the tree
    auto mat = stage()->material(ent.material_id());

    MaterialPass& pass = mat->pass(pass_number);

    //First, let's build the texture units
    RenderGroup* current = this;

    //Add a shader node
    current = &current->get_or_create<ShaderGroup>(ShaderGroupData(pass.program().get()));

    //Add a node for depth settings
    current = &current->get_or_create<DepthGroup>(
        DepthGroupData(pass.depth_test_enabled(), pass.depth_write_enabled())
    );

    //Add a node for the material properties
    current = &current->get_or_create<MaterialGroup>(
        MaterialGroupData(
            pass.ambient(), pass.diffuse(), pass.specular(),
            pass.shininess(), pass.texture_unit_count())
    );

    //Add a node for the blending type
    current = &current->get_or_create<BlendGroup>(BlendGroupData(pass.blending()));

    //Add a node for the render settings
    current = &current->get_or_create<RenderSettingsGroup>(
        RenderSettingsData(pass.point_size(), pass.polygon_mode())
    );

    //FIXME: This code is duplicated below
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

void LightGroup::bind(GPUProgram* program) {
    if(!data_.light_id) {
        return;
    }

    RootGroup& root = static_cast<RootGroup&>(get_root());
    auto light = root.stage()->light(data_.light_id);

    if(program->uniforms().uses_auto(SP_AUTO_LIGHT_POSITION)) {
        Vec4 light_pos = Vec4(light->absolute_position(), (light->type() == LIGHT_TYPE_DIRECTIONAL) ? 0.0 : 1.0);

        program->uniforms().set_vec4(
            program->uniforms().auto_variable_name(SP_AUTO_LIGHT_POSITION),
            light_pos
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_LIGHT_AMBIENT)) {
        program->uniforms().set_colour(
            program->uniforms().auto_variable_name(SP_AUTO_LIGHT_AMBIENT),
            light->ambient()
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_LIGHT_DIFFUSE)) {
        program->uniforms().set_colour(
            program->uniforms().auto_variable_name(SP_AUTO_LIGHT_DIFFUSE),
            light->diffuse()
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_LIGHT_SPECULAR)) {
        program->uniforms().set_colour(
            program->uniforms().auto_variable_name(SP_AUTO_LIGHT_SPECULAR),
            light->specular()
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_LIGHT_CONSTANT_ATTENUATION)) {
        program->uniforms().set_float(
            program->uniforms().auto_variable_name(SP_AUTO_LIGHT_CONSTANT_ATTENUATION),
            light->constant_attenuation()
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_LIGHT_LINEAR_ATTENUATION)) {
        program->uniforms().set_float(
            program->uniforms().auto_variable_name(SP_AUTO_LIGHT_LINEAR_ATTENUATION),
            light->linear_attenuation()
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION)) {
        program->uniforms().set_float(
            program->uniforms().auto_variable_name(SP_AUTO_LIGHT_QUADRATIC_ATTENUATION),
            light->quadratic_attenuation()
        );
    }
}

void LightGroup::unbind(GPUProgram* program) {

}

void InstancedMeshGroup::bind(GPUProgram* program) {}
void InstancedMeshGroup::unbind(GPUProgram* program) {}

void ShaderGroup::bind(GPUProgram* program) {
    RootGroup& root = static_cast<RootGroup&>(get_root());

    program = data_.shader_;
    root.set_current_program(program);
    program->build();
    program->activate();

    //Pass in the global ambient here, as it's the earliest place
    //in the tree we can, and it's a global value
    auto& params = program->uniforms();

    if(params.uses_auto(SP_AUTO_LIGHT_GLOBAL_AMBIENT)) {
        params.set_colour(
            params.auto_variable_name(SP_AUTO_LIGHT_GLOBAL_AMBIENT),
            root.stage()->ambient_light()
        );
    }
}

std::size_t ShaderGroupData::hash() const {
    size_t seed = 0;
    hash_combine(seed, typeid(ShaderGroupData).name());
    hash_combine(seed, shader_->md5().encode());
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
    GLCheck(glActiveTexture, GL_TEXTURE0 + data_.unit);
    RootGroup& root = static_cast<RootGroup&>(get_root());
    GLCheck(glBindTexture, GL_TEXTURE_2D, root.stage()->texture(data_.texture_id)->gl_tex());
}

void TextureGroup::unbind(GPUProgram *program) {
    GLCheck(glActiveTexture, GL_TEXTURE0 + data_.unit);
    GLCheck(glBindTexture, GL_TEXTURE_2D, 0);
}

void TextureMatrixGroup::bind(GPUProgram *program) {
    if(program->uniforms().uses_auto(ShaderAvailableAuto(SP_AUTO_MATERIAL_TEX_MATRIX0 + data_.unit))) {
        program->uniforms().set_mat4x4(
            program->uniforms().auto_variable_name(ShaderAvailableAuto(SP_AUTO_MATERIAL_TEX_MATRIX0 + data_.unit)),
            data_.matrix
        );
    }
}

void TextureMatrixGroup::unbind(GPUProgram* program) {

}

void MaterialGroup::bind(GPUProgram* program) {
    if(program->uniforms().uses_auto(SP_AUTO_MATERIAL_AMBIENT)) {
        program->uniforms().set_colour(
            program->uniforms().auto_variable_name(SP_AUTO_MATERIAL_AMBIENT),
            data_.ambient
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_MATERIAL_DIFFUSE)) {
        program->uniforms().set_colour(
            program->uniforms().auto_variable_name(SP_AUTO_MATERIAL_DIFFUSE),
            data_.diffuse
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_MATERIAL_SPECULAR)) {
        program->uniforms().set_colour(
            program->uniforms().auto_variable_name(SP_AUTO_MATERIAL_SPECULAR),
            data_.specular
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_MATERIAL_SHININESS)) {
        program->uniforms().set_float(
            program->uniforms().auto_variable_name(SP_AUTO_MATERIAL_SHININESS),
            data_.shininess
        );
    }

    if(program->uniforms().uses_auto(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS)) {
        program->uniforms().set_int(
            program->uniforms().auto_variable_name(SP_AUTO_MATERIAL_ACTIVE_TEXTURE_UNITS),
            data_.active_texture_count
        );
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
    L_WARN("On GLES glPointSize doesn't exist");
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
        L_WARN("On GLES glPolygonMode doesn't exist");
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
