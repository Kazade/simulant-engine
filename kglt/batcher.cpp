#include <GLee.h>

#include "entity.h"
#include "batcher.h"

#include "material.h"
#include "resource_manager.h"
#include "shader.h"

namespace kglt {

void RootGroup::insert(SubEntity &ent, uint8_t pass_number) {
    //Get the material for the entity, this is used to build the tree
    Material& mat = resource_manager_.material(ent.material_id());

    MaterialPass& pass = mat.technique().pass(pass_number);

    //First, let's build the texture units
    RenderGroup* current = this;

    //Add a shader node
    current = &current->get_or_create<ShaderGroup>(ShaderGroupData(pass.shader()));

    //Add a node for depth settings
    current = &current->get_or_create<DepthGroup>(DepthGroupData(pass.depth_test_enabled(), pass.depth_write_enabled()));

    //Add the texture-related branches of the tree under the shader(
    for(uint8_t tu = 0; tu < pass.texture_unit_count(); ++tu) {
        current->get_or_create<TextureGroup>(TextureGroupData(tu, pass.texture_unit(tu).texture())).
                 get_or_create<TextureMatrixGroup>(TextureMatrixGroupData(tu, pass.texture_unit(tu).matrix())).
                 get_or_create<MeshGroup>(MeshGroupData(ent._parent().mesh(), ent.submesh_id())).add(&ent);
    }
}


void MeshGroup::bind() {

}

void MeshGroup::unbind() {

}

void ShaderGroup::bind() {
    RootGroup& root = static_cast<RootGroup&>(get_root());

    ShaderProgram& s = root.resource_manager().shader(data_.shader_id);
    s.activate(); //Activate the shader
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
    glBindTexture(GL_TEXTURE_2D, root.resource_manager().texture(data_.texture_id).gl_tex());
}

void TextureGroup::unbind() {
    glActiveTexture(GL_TEXTURE0 + data_.unit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureMatrixGroup::bind() {

}

void TextureMatrixGroup::unbind() {

}

}
