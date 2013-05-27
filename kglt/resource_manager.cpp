#include "window_base.h"
#include "resource_manager.h"
#include "loader.h"

namespace kglt {

Scene& ResourceManager::scene() {
    return window().scene();
}

MeshRef ResourceManager::mesh(MeshID m) {
    if(!parent_) {
        return MeshManager::manager_get(m);
    } else if(!has_mesh(m)) {
        return parent_->mesh(m);
    }    
    return MeshManager::manager_get(m);
}

const MeshRef ResourceManager::mesh(MeshID m) const {
    if(!parent_) {
        return MeshManager::manager_get(m);
    } else if(!has_mesh(m)) {
        return parent_->mesh(m);
    }
    return MeshManager::manager_get(m);
}

MeshID ResourceManager::new_mesh() {
    MeshID result = MeshManager::manager_new();
    return result;
}

MeshID ResourceManager::new_mesh_from_file(const unicode& path) {
    //Load the material
    MeshPtr m = mesh(new_mesh()).lock();
    window().loader_for(path.encode())->into(*m);
    return m->id();
}

bool ResourceManager::has_mesh(MeshID m) const {
    return MeshManager::manager_contains(m);
}

uint32_t ResourceManager::mesh_count() const {
    return MeshManager::manager_count();
}

MaterialID ResourceManager::new_material() {
    MaterialID result = MaterialManager::manager_new();
    return result;
}

MaterialID ResourceManager::new_material_from_file(const unicode& path) {
    //Load the material
    MaterialPtr mat = material(new_material()).lock();
    window().loader_for(path.encode())->into(*mat);
    return mat->id();
}

MaterialRef ResourceManager::material(MaterialID mid) {
    if(!has_material(mid) && parent_) {
        return parent_->material(mid);
    }
    return MaterialManager::manager_get(mid);
}

bool ResourceManager::has_material(MaterialID m) const {
    return MaterialManager::manager_contains(m);
}

TextureID ResourceManager::new_texture() {
    return TextureManager::manager_new();
}

TextureID ResourceManager::new_texture_from_file(const unicode& path) {
    //Load the texture
    TexturePtr tex = texture(new_texture()).lock();
    window().loader_for(path.encode())->into(*tex);
    window().idle().add_once(std::bind(&Texture::upload, tex, false, true, true, false));
    return tex->id();
}

TextureRef ResourceManager::texture(TextureID t) {
    if(!has_texture(t) && parent_) {
        return parent_->texture(t);
    }
    return TextureManager::manager_get(t);
}

bool ResourceManager::has_texture(TextureID t) const {
    return TextureManager::manager_contains(t);
}

ShaderRef ResourceManager::shader(ShaderID s) {
    if(!has_shader(s) && parent_) {
        return parent_->shader(s);
    }
    return ShaderManager::manager_get(s);
}

ShaderProgram* ResourceManager::__shader(ShaderID s) {
    if(!has_shader(s) && parent_) {
        return parent_->__shader(s);
    }
    return ShaderManager::manager_get_unsafe(s);
}

bool ResourceManager::has_shader(ShaderID s) const {
    return ShaderManager::manager_contains(s);
}

ShaderID ResourceManager::new_shader() {
    return ShaderManager::manager_new();
}

SoundID ResourceManager::new_sound() {
    return SoundManager::manager_new();
}

SoundID ResourceManager::new_sound_from_file(const unicode& path) {
    //Load the sound
    SoundPtr snd = sound(new_sound()).lock();
    window().loader_for(path.encode())->into(*snd);
    return snd->id();
}

SoundRef ResourceManager::sound(SoundID s) {
    return SoundManager::manager_get(s);
}

bool ResourceManager::has_sound(SoundID s) const {
    return SoundManager::manager_contains(s);
}

std::pair<ShaderID, bool> ResourceManager::find_shader(const std::string& name) {
    std::map<std::string, ShaderID>::const_iterator it = shader_lookup_.find(name);
    if(it == shader_lookup_.end()) {
        return std::make_pair(ShaderID(), false);
    }

    return std::make_pair((*it).second, true);
}

void ResourceManager::inherit(Resource* resource) {
    if(ShaderProgram* shader = dynamic_cast<ShaderProgram*>(resource)) {
        ShaderManager* source = dynamic_cast<ShaderManager*>(&shader->resource_manager());
        ShaderManager::__objects()[shader->id()] = source->__objects()[shader->id()];
        source->__objects().erase(shader->id());
    } else {
        //FIXME: Do other nasty casts
        throw NotImplementedError(__FILE__, __LINE__);
    }
}

}
