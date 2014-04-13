#include "window_base.h"
#include "resource_manager.h"
#include "loader.h"
#include "scene.h"
#include "procedural/mesh.h"

#include "kazbase/datetime.h"


/** FIXME
 *
 * - Write tests to show that all new_X_from_file methods mark resources as uncollected before returning
 * - Think of better GC logic, perhaps collect on every frame?
 */

namespace kglt {

ResourceManagerImpl::ResourceManagerImpl(WindowBase* window):
    window_(window) {

    ShaderManager::signal_post_create().connect(std::bind(&ResourceManagerImpl::post_create_shader_callback, this, std::placeholders::_1, std::placeholders::_2));


}

bool ResourceManagerImpl::init() {
    //FIXME: Should lock the default texture and material during construction!

    //Create the default blank texture
    default_texture_id_ = new_texture(false);

    auto tex = texture(default_texture_id_);

    tex->resize(1, 1);
    tex->set_bpp(32);

    tex->data()[0] = 255;
    tex->data()[1] = 255;
    tex->data()[2] = 255;
    tex->data()[3] = 255;
    tex->upload();

    //Maintain ref-count
    default_material_id_ = new_material_from_file(default_material_filename(), false);

    //Set the default material's first texture to the default (white) texture
    material(default_material_id_)->technique().pass(0).set_texture_unit(0, default_texture_id_);

    return true;
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

MeshID ResourceManagerImpl::new_mesh(bool garbage_collect) {
    MeshID result = MeshManager::manager_new(garbage_collect);
    return result;
}

MeshID ResourceManagerImpl::new_mesh_from_file(const unicode& path, bool garbage_collect) {
    //Load the material
    kglt::MeshID mesh_id = new_mesh(garbage_collect);
    window().loader_for(path.encode())->into(mesh(mesh_id));
    MeshManager::mark_as_uncollected(mesh_id);
    return mesh_id;
}

MeshID ResourceManagerImpl::new_mesh_as_cube(float width, bool garbage_collect) {
    MeshID m = new_mesh(garbage_collect);
    kglt::procedural::mesh::cube(mesh(m), width);
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManagerImpl::new_mesh_as_sphere(float diameter, bool garbage_collect) {
    MeshID m = new_mesh(garbage_collect);
    kglt::procedural::mesh::sphere(mesh(m), diameter);
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManagerImpl::new_mesh_as_rectangle(float width, float height, const Vec2& offset, bool garbage_collect) {
    MeshID m = new_mesh(garbage_collect);
    kglt::procedural::mesh::rectangle(mesh(m), width, height, offset.x, offset.y);
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManagerImpl::new_mesh_with_alias(const unicode& alias, bool garbage_collect) {
    MeshID m = new_mesh(garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManagerImpl::new_mesh_with_alias_from_file(const unicode& alias, const unicode& path, bool garbage_collect) {
    MeshID m = new_mesh_from_file(path, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManagerImpl::new_mesh_with_alias_as_cube(const unicode& alias, float width, bool garbage_collect) {
    MeshID m = new_mesh_as_cube(width, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManagerImpl::new_mesh_with_alias_as_sphere(const unicode& alias, float diameter, bool garbage_collect) {
    MeshID m = new_mesh_as_sphere(diameter, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManagerImpl::new_mesh_with_alias_as_rectangle(const unicode& alias, float width, float height, const Vec2& offset, bool garbage_collect) {
    MeshID m = new_mesh_as_rectangle(width, height, offset, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManagerImpl::get_mesh_with_alias(const unicode& alias) {
    return MeshManager::manager_get_by_alias(alias);
}

void ResourceManagerImpl::delete_mesh(MeshID m) {
    mesh(m)->enable_gc();
}

bool ResourceManagerImpl::has_mesh(MeshID m) const {
    return MeshManager::manager_contains(m);
}

uint32_t ResourceManagerImpl::mesh_count() const {
    return MeshManager::manager_count();
}

MaterialID ResourceManagerImpl::new_material(bool garbage_collect) {
    MaterialID result = MaterialManager::manager_new(garbage_collect);
    return result;
}

void ResourceManagerImpl::delete_material(MaterialID m) {
    material(m)->enable_gc();
}

MaterialID ResourceManagerImpl::new_material_from_file(const unicode& path, bool garbage_collect) {
    //Load the material
    auto mat = material(new_material(garbage_collect));
    window().loader_for(path.encode())->into(mat);
    mark_material_as_uncollected(mat->id());
    return mat->id();
}

MaterialID ResourceManagerImpl::new_material_with_alias(const unicode& alias, bool garbage_collect) {
    MaterialID m = new_material(garbage_collect);

    try {
        MaterialManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_material(m);
        throw;
    }
    return m;
}

MaterialID ResourceManagerImpl::new_material_with_alias_from_file(const unicode& alias, const unicode& path, bool garbage_collect) {
    MaterialID m = new_material_from_file(path, garbage_collect);
    try {
        MaterialManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_material(m);
        throw;
    }
    return m;
}

MaterialID ResourceManagerImpl::new_material_from_texture(TextureID texture_id, bool garbage_collect) {
    MaterialID m = new_material_from_file(default_material_filename(), garbage_collect);
    material(m)->technique().pass(0).set_texture_unit(0, texture_id);
    mark_material_as_uncollected(m); //FIXME: Race-y
    return m;
}

MaterialID ResourceManagerImpl::get_material_with_alias(const unicode& alias) {
    return MaterialManager::manager_get_by_alias(alias);
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

TextureID ResourceManagerImpl::new_texture(bool garbage_collect) {
    return TextureManager::manager_new(garbage_collect);
}

TextureID ResourceManagerImpl::new_texture_from_file(const unicode& path, TextureFlags flags, bool garbage_collect) {
    //Load the texture
    auto tex = texture(new_texture(garbage_collect));
    window().loader_for(path.encode())->into(tex);

    if((flags & TEXTURE_OPTION_FLIP_VERTICALLY) == TEXTURE_OPTION_FLIP_VERTICALLY) {
        tex->flip_vertically();
    }

    tex->upload(
        false,
        (flags & TEXTURE_OPTION_DISABLE_MIPMAPS) != TEXTURE_OPTION_DISABLE_MIPMAPS,
        (flags & TEXTURE_OPTION_CLAMP_TO_EDGE) != TEXTURE_OPTION_CLAMP_TO_EDGE,
        (flags & TEXTURE_OPTION_NEAREST_FILTER) != TEXTURE_OPTION_NEAREST_FILTER
    );

    mark_texture_as_uncollected(tex->id());
    return tex->id();
}

void ResourceManagerImpl::delete_texture(TextureID t) {
    texture(t)->enable_gc();
}

TextureID ResourceManagerImpl::new_texture_with_alias(const unicode& alias, bool garbage_collect) {
    TextureID t = new_texture(garbage_collect);
    try {
        TextureManager::manager_store_alias(alias, t);
    } catch(...) {
        delete_texture(t);
        throw;
    }
    return t;
}

TextureID ResourceManagerImpl::new_texture_with_alias_from_file(const unicode& alias, const unicode& path, TextureFlags flags, bool garbage_collect) {
    TextureID t = new_texture_from_file(path, flags, garbage_collect);
    try {
        TextureManager::manager_store_alias(alias, t);
    } catch(...) {
        delete_texture(t);
        throw;
    }
    return t;
}

TextureID ResourceManagerImpl::get_texture_with_alias(const unicode& alias) {
    return TextureManager::manager_get_by_alias(alias);
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

ShaderID ResourceManagerImpl::new_shader(bool garbage_collect) {
    return ShaderManager::manager_new(garbage_collect);
}

ShaderID ResourceManagerImpl::new_shader_from_files(const unicode& vert_shader, const unicode& frag_shader, bool garbage_collect) {
    ShaderPtr shd = shader(new_shader(garbage_collect)).lock();
    shd->add_and_compile(SHADER_TYPE_VERTEX, vert_shader);
    shd->add_and_compile(SHADER_TYPE_FRAGMENT, frag_shader);
    return shd->id();
}


SoundID ResourceManagerImpl::new_sound(bool garbage_collect) {
    return SoundManager::manager_new(garbage_collect);
}

SoundID ResourceManagerImpl::new_sound_from_file(const unicode& path, bool garbage_collect) {
    //Load the sound
    auto snd = sound(new_sound(garbage_collect));
    window().loader_for(path.encode())->into(snd);
    return snd->id();
}

SoundID ResourceManagerImpl::new_sound_with_alias(const unicode& alias, bool garbage_collect) {
    SoundID s = new_sound(garbage_collect);
    try {
        SoundManager::manager_store_alias(alias, s);
    } catch(...) {
        delete_sound(s);
        throw;
    }
    return s;
}

SoundID ResourceManagerImpl::new_sound_with_alias_from_file(const unicode& alias, const unicode& path, bool garbage_collect) {
    SoundID s = new_sound_from_file(path, garbage_collect);
    try {
        SoundManager::manager_store_alias(alias, s);
    } catch(...) {
        delete_sound(s);
        throw;
    }
    return s;
}

SoundID ResourceManagerImpl::get_sound_with_alias(const unicode& alias) {
    return SoundManager::manager_get_by_alias(alias);
}

ProtectedPtr<Sound> ResourceManagerImpl::sound(SoundID s) {
    return SoundManager::manager_get(s);
}

const ProtectedPtr<Sound> ResourceManagerImpl::sound(SoundID s) const {
    return SoundManager::manager_get(s);
}

uint32_t ResourceManagerImpl::sound_count() const {
    return SoundManager::manager_count();
}

bool ResourceManagerImpl::has_sound(SoundID s) const {
    return SoundManager::manager_contains(s);
}

void ResourceManagerImpl::delete_sound(SoundID t) {
    sound(t)->enable_gc();
}

std::pair<ShaderID, bool> ResourceManagerImpl::find_shader(const std::string& name) {
    std::map<std::string, ShaderID>::const_iterator it = shader_lookup_.find(name);
    if(it == shader_lookup_.end()) {
        return std::make_pair(ShaderID(), false);
    }

    return std::make_pair((*it).second, true);
}

TextureID ResourceManagerImpl::default_texture_id() const {
    return default_texture_id_;
}

MaterialID ResourceManagerImpl::default_material_id() const {
    return default_material_id_;
}

unicode ResourceManagerImpl::default_material_filename() const {
    return window().resource_locator().locate_file("kglt/materials/multitexture_and_lighting.kglm");
}

}
