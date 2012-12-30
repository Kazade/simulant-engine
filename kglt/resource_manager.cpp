
#include "resource_manager.h"

namespace kglt {

Mesh& ResourceManager::mesh(MeshID m) {
    if(!has_mesh(m) && parent_) {
        return parent_->mesh(m);
    }
    return const_cast<Mesh&>(static_cast<const ResourceManager*>(this)->mesh(m));
}

const Mesh& ResourceManager::mesh(MeshID m) const {
    if(!has_mesh(m) && parent_) {
        return parent_->mesh(m);
    }

    return MeshManager::manager_get(m);
}

MeshID ResourceManager::new_mesh() {
    MeshID result = MeshManager::manager_new();
    return result;
}

bool ResourceManager::has_mesh(MeshID m) const {
    return MeshManager::manager_contains(m);
}

void ResourceManager::delete_mesh(MeshID mid) {
    return MeshManager::manager_delete(mid);
}

MaterialID ResourceManager::new_material(MaterialID clone_from) {
    MaterialID result = MaterialManager::manager_new();
    if(clone_from) {
        kglt::Material& existing = material(clone_from);
        kglt::Material& new_mat = material(result);

        for(uint32_t i = 0; i < existing.technique_count(); ++i) {
            //FIXME: handle multiple schemes
            MaterialTechnique& old_tech = existing.technique();
            MaterialTechnique& new_tech = new_mat.technique();
            for(uint32_t j = 0; j < old_tech.pass_count(); ++j) {
                MaterialPass& old_pass = old_tech.pass(j);
                uint32_t pass_id = new_tech.new_pass(old_pass.shader());
                MaterialPass& new_pass = new_tech.pass(pass_id);

                new_pass.set_blending(old_pass.blending());
                new_pass.set_iteration(old_pass.iteration(), old_pass.max_iterations());
                for(uint32_t k = 0; k < old_pass.texture_unit_count(); ++k) {
                    new_pass.set_texture_unit(k, old_pass.texture_unit(k).texture());
                }
                //FIXME: Copy animated texture unit and other properties
            }
        }
    }
    return result;
}

Material& ResourceManager::material(MaterialID mid) {
    if(!has_material(mid) && parent_) {
        return parent_->material(mid);
    }
    return MaterialManager::manager_get(mid);
}

bool ResourceManager::has_material(MaterialID m) const {
    return MaterialManager::manager_contains(m);
}

void ResourceManager::delete_material(MaterialID mid) {
    MaterialManager::manager_delete(mid);
}

TextureID ResourceManager::new_texture() {
    return TextureManager::manager_new();
}

void ResourceManager::delete_texture(TextureID tid) {
    TextureManager::manager_delete(tid);
}

Texture& ResourceManager::texture(TextureID t) {
    return TextureManager::manager_get(t);
}

bool ResourceManager::has_texture(TextureID t) const {
    return TextureManager::manager_contains(t);
}

ShaderProgram& ResourceManager::shader(ShaderID s) {
    if(!has_shader(s) && parent_) {
        return parent_->shader(s);
    }
    return ShaderManager::manager_get(s);
}

bool ResourceManager::has_shader(ShaderID s) const {
    return ShaderManager::manager_contains(s);
}

ShaderID ResourceManager::new_shader() {
    return ShaderManager::manager_new();
}

void ResourceManager::delete_shader(ShaderID s) {
    ShaderManager::manager_delete(s);
}

std::pair<ShaderID, bool> ResourceManager::find_shader(const std::string& name) {
    std::map<std::string, ShaderID>::const_iterator it = shader_lookup_.find(name);
    if(it == shader_lookup_.end()) {
        return std::make_pair(ShaderID(), false);
    }

    return std::make_pair((*it).second, true);
}

}
