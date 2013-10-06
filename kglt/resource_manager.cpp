#include "window_base.h"
#include "resource_manager.h"
#include "loader.h"

#include "procedural/mesh.h"

#include "kazbase/datetime.h"

namespace kglt {

ResourceManagerImpl::ResourceManagerImpl(WindowBase* window):
    window_(window) {

    window_->signal_frame_finished().connect(std::bind(&ResourceManagerImpl::update, this));

    ShaderManager::signal_post_create().connect(std::bind(&ResourceManagerImpl::post_create_shader_callback, this, std::placeholders::_1, std::placeholders::_2));
}

void ResourceManagerImpl::update() {
    static datetime::DateTime last_collection = datetime::now();

    if(datetime::timedelta_in_seconds(datetime::now() - last_collection) >= 5) {
        //Garbage collect all the things
        L_DEBUG("Collecting meshes");
        MeshManager::garbage_collect();

        L_DEBUG("Collecting materials");
        MaterialManager::garbage_collect();

        L_DEBUG("Collecting textures");
        TextureManager::garbage_collect();

        L_DEBUG("Collecting shaders");
        ShaderManager::garbage_collect();

        L_DEBUG("Collecting sounds");
        SoundManager::garbage_collect();

        last_collection = datetime::now();
    }

    /*
      Update all animated materials
    */
    double dt = window_->delta_time();
    apply_func_to_materials(std::bind(&Material::update, std::placeholders::_1, dt));
}

Scene& ResourceManagerImpl::scene() {
    return window().scene();
}

const Scene& ResourceManagerImpl::scene() const {
    return window().scene();
}

ProtectedPtr<Mesh> ResourceManagerImpl::mesh(MeshID m) {
    return MeshManager::manager_get(m);
}

const ProtectedPtr<Mesh> ResourceManagerImpl::mesh(MeshID m) const {
    return MeshManager::manager_get(m);
}

MeshID ResourceManagerImpl::new_mesh() {
    MeshID result = MeshManager::manager_new();
    return result;
}

MeshID ResourceManagerImpl::new_mesh_from_file(const unicode& path) {
    //Load the material
    kglt::MeshID mesh_id = new_mesh();
    window().loader_for(path.encode())->into(mesh(mesh_id));
    return mesh_id;
}

MeshID ResourceManagerImpl::new_mesh_as_cube(float width) {
    MeshID m = new_mesh();
    kglt::procedural::mesh::cube(mesh(m), width);
    return m;
}

MeshID ResourceManagerImpl::new_mesh_as_sphere(float diameter) {
    MeshID m = new_mesh();
    kglt::procedural::mesh::sphere(mesh(m), diameter);
    return m;
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
    auto mat = material(new_material());
    window().loader_for(path.encode())->into(mat);
    return mat->id();
}

MaterialID ResourceManagerImpl::clone_material(MaterialID mat) {
    MaterialID result = MaterialManager::manager_clone(mat);
    return result;
}

ProtectedPtr<Material> ResourceManagerImpl::material(MaterialID mid) {
    return ProtectedPtr<Material>(MaterialManager::manager_get(mid));
}

const ProtectedPtr<Material> ResourceManagerImpl::material(MaterialID mid) const {
    return ProtectedPtr<Material>(MaterialManager::manager_get(mid));
}

bool ResourceManagerImpl::has_material(MaterialID m) const {
    return MaterialManager::manager_contains(m);
}

uint32_t ResourceManagerImpl::material_count() const {
    return MaterialManager::manager_count();
}

void ResourceManagerImpl::mark_material_as_uncollected(MaterialID t) {
    MaterialManager::mark_as_uncollected(t);
}

TextureID ResourceManagerImpl::new_texture() {
    return TextureManager::manager_new();
}

TextureID ResourceManagerImpl::new_texture_from_file(const unicode& path) {
    //Load the texture
    auto tex = texture(new_texture());
    window().loader_for(path.encode())->into(tex);
    tex->upload(false, true, true, false);
    return tex->id();
}

ProtectedPtr<Texture> ResourceManagerImpl::texture(TextureID t) {
    return ProtectedPtr<Texture>(TextureManager::manager_get(t).lock());
}

const ProtectedPtr<Texture> ResourceManagerImpl::texture(TextureID t) const {
    return ProtectedPtr<Texture>(TextureManager::manager_get(t).lock());
}

bool ResourceManagerImpl::has_texture(TextureID t) const {
    return TextureManager::manager_contains(t);
}

uint32_t ResourceManagerImpl::texture_count() const {
    return TextureManager::manager_count();
}

void ResourceManagerImpl::mark_texture_as_uncollected(TextureID t) {
    TextureManager::mark_as_uncollected(t);
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
    window().loader_for(path.encode())->into(snd);
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
