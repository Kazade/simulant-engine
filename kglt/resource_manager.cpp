#include "window_base.h"
#include "resource_manager.h"
#include "loader.h"

namespace kglt {

ResourceManagerImpl::ResourceManagerImpl(WindowBase* window):
    window_(window) {

    window_->signal_frame_finished().connect(std::bind(&ResourceManagerImpl::update, this));

    ShaderManager::signal_post_create().connect(sigc::mem_fun(this, &ResourceManagerImpl::post_create_shader_callback));
}

void ResourceManagerImpl::update() {
    //Garbage collect all the things
    MeshManager::garbage_collect();
    MaterialManager::garbage_collect();
    TextureManager::garbage_collect();
    ShaderManager::garbage_collect();
    SoundManager::garbage_collect();

    /*
      Update all animated materials
    */
    double dt = window_->delta_time();
    MaterialManager::apply_func_to_objects(std::bind(&Material::update, std::placeholders::_1, dt));
}

Scene& ResourceManagerImpl::scene() {
    return window().scene();
}

const Scene& ResourceManagerImpl::scene() const {
    return window().scene();
}

MeshRef ResourceManagerImpl::mesh(MeshID m) {
    return MeshManager::manager_get(m);
}

const MeshRef ResourceManagerImpl::mesh(MeshID m) const {
    return MeshManager::manager_get(m);
}

MeshID ResourceManagerImpl::new_mesh() {
    MeshID result = MeshManager::manager_new();
    return result;
}

MeshID ResourceManagerImpl::new_mesh_from_file(const unicode& path) {
    //Load the material
    MeshPtr m = mesh(new_mesh()).lock();
    window().loader_for(path.encode())->into(*m);
    return m->id();
}

bool ResourceManagerImpl::has_mesh(MeshID m) const {
    return MeshManager::manager_contains(m);
}

uint32_t ResourceManagerImpl::mesh_count() const {
    return MeshManager::manager_count();
}

MaterialID ResourceManagerImpl::new_material() {
    MaterialID result = MaterialManager::manager_new();
    return result;
}

MaterialID ResourceManagerImpl::new_material_from_file(const unicode& path) {
    //Load the material
    MaterialPtr mat = material(new_material()).lock();
    window().loader_for(path.encode())->into(*mat);
    return mat->id();
}

MaterialRef ResourceManagerImpl::material(MaterialID mid) {
    return MaterialManager::manager_get(mid);
}

const MaterialRef ResourceManagerImpl::material(MaterialID mid) const {
    return MaterialManager::manager_get(mid);
}

bool ResourceManagerImpl::has_material(MaterialID m) const {
    return MaterialManager::manager_contains(m);
}

uint32_t ResourceManagerImpl::material_count() const {
    return MaterialManager::manager_count();
}

TextureID ResourceManagerImpl::new_texture() {
    return TextureManager::manager_new();
}

TextureID ResourceManagerImpl::new_texture_from_file(const unicode& path) {
    //Load the texture
    TexturePtr tex = texture(new_texture()).lock();
    window().loader_for(path.encode())->into(*tex);
    window().idle().add_once(std::bind(&Texture::upload, tex, false, true, true, false));
    return tex->id();
}

TextureRef ResourceManagerImpl::texture(TextureID t) {
    return TextureManager::manager_get(t);
}

const TextureRef ResourceManagerImpl::texture(TextureID t) const {
    return TextureManager::manager_get(t);
}

bool ResourceManagerImpl::has_texture(TextureID t) const {
    return TextureManager::manager_contains(t);
}

uint32_t ResourceManagerImpl::texture_count() const {
    return TextureManager::manager_count();
}

ShaderRef ResourceManagerImpl::shader(ShaderID s) {
    return ShaderManager::manager_get(s);
}

const ShaderRef ResourceManagerImpl::shader(ShaderID s) const {
    return ShaderManager::manager_get(s);
}

ShaderProgram* ResourceManagerImpl::__shader(ShaderID s) {
    return ShaderManager::manager_get_unsafe(s);
}

bool ResourceManagerImpl::has_shader(ShaderID s) const {
    return ShaderManager::manager_contains(s);
}

uint32_t ResourceManagerImpl::shader_count() const {
    return ShaderManager::manager_count();
}

ShaderID ResourceManagerImpl::new_shader() {
    return ShaderManager::manager_new();
}

SoundID ResourceManagerImpl::new_sound() {
    return SoundManager::manager_new();
}

SoundID ResourceManagerImpl::new_sound_from_file(const unicode& path) {
    //Load the sound
    SoundPtr snd = sound(new_sound()).lock();
    window().loader_for(path.encode())->into(*snd);
    return snd->id();
}

SoundRef ResourceManagerImpl::sound(SoundID s) {
    return SoundManager::manager_get(s);
}

const SoundRef ResourceManagerImpl::sound(SoundID s) const {
    return SoundManager::manager_get(s);
}

uint32_t ResourceManagerImpl::sound_count() const {
    return SoundManager::manager_count();
}

bool ResourceManagerImpl::has_sound(SoundID s) const {
    return SoundManager::manager_contains(s);
}

std::pair<ShaderID, bool> ResourceManagerImpl::find_shader(const std::string& name) {
    std::map<std::string, ShaderID>::const_iterator it = shader_lookup_.find(name);
    if(it == shader_lookup_.end()) {
        return std::make_pair(ShaderID(), false);
    }

    return std::make_pair((*it).second, true);
}

}
