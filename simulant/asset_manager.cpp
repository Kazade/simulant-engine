//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "window.h"
#include "asset_manager.h"
#include "loader.h"
#include "procedural/mesh.h"
#include "utils/gl_thread_check.h"
#include "loaders/heightmap_loader.h"

/** FIXME
 *
 * - Write tests to show that all new_X_from_file methods mark resources as uncollected before returning
 * - Think of better GC logic, perhaps collect on every frame?
 */

namespace smlt {

#define HEADING_FONT "simulant/fonts/orbitron/orbitron-regular-48.fnt"
#define BODY_FONT "simulant/fonts/orbitron/orbitron-regular-18.fnt"

AssetManager::AssetManager(Window* window, AssetManager* parent):
    WindowHolder(window),
    parent_(parent) {

    if(parent_) {
        S_DEBUG("Registering new resource manager: {0}", this);
        base_manager()->register_child(this);
    } else {
        S_DEBUG("Created base manager: {0}", this);
    }
}

AssetManager::~AssetManager() {
    if(parent_) {
        S_DEBUG("Unregistering resource manager: {0}", this);
        base_manager()->unregister_child(this);
        parent_ = nullptr;
    } else {
        if(!children_.empty()) {
            S_WARN("Destroyed base manager while children remain");
        }

        S_DEBUG("Destroyed base manager: {0}", this);
    }
}

void AssetManager::update(float dt) {
    material_manager_.each([dt](uint32_t, MaterialPtr mat) {
        mat->update(dt);
    });

    texture_manager_.each([dt](uint32_t, TexturePtr tex) {
        tex->update(dt);
    });
}

void SharedAssetManager::set_default_material_filename(const unicode& filename) {
    default_material_filename_ = filename;
    default_material_.reset();
}

unicode SharedAssetManager::default_material_filename() const {
    return default_material_filename_;
}

MaterialPtr AssetManager::default_material() const {
    if(!is_base_manager()) {
        return base_manager()->default_material();
    }

    /* Shouldn't happen */
    assert(0);
    return nullptr;
}


static TexturePtr create_texture_with_colour(AssetManager* manager, const Colour& c) {
    auto tex = manager->new_texture(8, 8, TEXTURE_FORMAT_RGB888, GARBAGE_COLLECT_NEVER);

    const uint8_t r = (uint8_t) (c.r * 255.0f);
    const uint8_t g = (uint8_t) (c.g * 255.0f);
    const uint8_t b = (uint8_t) (c.b * 255.0f);

    std::vector<uint8_t> bytes;
    bytes.reserve(64 * 3);
    for(std::size_t i = 0; i < 64; ++i) {
        bytes.push_back(r);
        bytes.push_back(g);
        bytes.push_back(b);
    }

    tex->set_data(bytes);
    return tex;
}


bool SharedAssetManager::init() {
    S_DEBUG("Initalizing default materials, textures, and fonts (AssetManager: {0})", this);
    set_default_material_filename(Material::BuiltIns::DEFAULT);
    set_default_font_filename(DEFAULT_FONT_STYLE_BODY, BODY_FONT);
    set_default_font_filename(DEFAULT_FONT_STYLE_HEADING, HEADING_FONT);

    white_tex_ = create_texture_with_colour(this, smlt::Colour::WHITE);
    white_tex_->set_name("s_white_texture");

    black_tex_ = create_texture_with_colour(this, smlt::Colour::BLACK);
    black_tex_->set_name("s_black_texture");

    z_tex_ = create_texture_with_colour(this, smlt::Colour::from_hex_string("#8080FF"));
    z_tex_->set_name("s_znormal_texture");

    /* Update the core material */
    CoreMaterial mat;

    mat.diffuse_map = white_tex_;
    mat.light_map = white_tex_;
    mat.specular_map = black_tex_;
    mat.normal_map = z_tex_;

    init_core_material(mat);

    S_DEBUG("Finished initializing defaults");
    return true;
}

MaterialPtr SharedAssetManager::default_material() const {
    assert(is_base_manager());

    if(!default_material_) {
        default_material_ = const_cast<SharedAssetManager*>(this)->new_material_from_file(
            default_material_filename_
        );
    }

    return default_material_;
}

void SharedAssetManager::set_default_font_filename(DefaultFontStyle style, const unicode& filename) {
    if(style == DEFAULT_FONT_STYLE_BODY) {
        default_body_font_filename_ = filename;
        default_body_font_.reset();
    } else {
        default_heading_font_filename_ = filename;
        default_heading_font_.reset();
    }
}

unicode SharedAssetManager::default_font_filename(DefaultFontStyle style) const {
    if(style == DEFAULT_FONT_STYLE_BODY) {
        return default_body_font_filename_;
    } else {
        return default_heading_font_filename_;
    }
}

FontPtr SharedAssetManager::default_font(DefaultFontStyle style) const {
    assert(is_base_manager());

    if(style == DEFAULT_FONT_STYLE_BODY) {
        if(!default_body_font_) {
            default_body_font_ = const_cast<SharedAssetManager*>(this)->new_font_from_file(
                default_body_font_filename_
            );
        }

        return default_body_font_;
    } else {
        if(!default_heading_font_) {
            default_heading_font_ = const_cast<SharedAssetManager*>(this)->new_font_from_file(
                default_heading_font_filename_
            );
        }

        return default_heading_font_;
    }
}

FontPtr AssetManager::default_font(DefaultFontStyle style) const {
    assert(!is_base_manager());
    return base_manager()->default_font(style);
}

void AssetManager::run_garbage_collection() {
    for(auto child: children_) {
        child->run_garbage_collection();
    }

    // Update the managers which triggers GC if necessary
    mesh_manager_.update();
    material_manager_.update();
    texture_manager_.update();
    sound_manager_.update();
    font_manager_.update();
    particle_script_manager_.update();
}

bool AssetManager::is_base_manager() const {
    return !parent_;
}


#define DOCONCAT(x, y) x##y
#define CONCAT(x, y) DOCONCAT(x, y)

#define NEW_X(klass, name, manager_name, ...) \
    auto result = manager_name.make(this, ##__VA_ARGS__); \
    manager_name.set_garbage_collection_method(result->id(), garbage_collect); \
    return result


#define GET_X(klass, name, manager_name) \
    if(parent_ && !CONCAT(has_, name)(id)) { \
        return parent_-> name (id); \
    } \
    return manager_name.get(id)


MeshPtr AssetManager::new_mesh(VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect) {
    NEW_X(Mesh, mesh, mesh_manager_, vertex_specification);
}

MeshPtr AssetManager::mesh(MeshID id) {
    GET_X(Mesh, mesh, mesh_manager_);
}

const MeshPtr AssetManager::mesh(MeshID id) const {
    GET_X(Mesh, mesh, mesh_manager_);
}

MeshPtr AssetManager::new_mesh_from_submesh(SubMesh* submesh, GarbageCollectMethod garbage_collect) {
    VertexSpecification spec = submesh->vertex_data->vertex_specification();
    auto result = new_mesh(spec, garbage_collect);

    SubMesh* target = result->new_submesh_with_material(
        submesh->name(),
        submesh->material(),
        submesh->arrangement()
    );

    std::unordered_map<Index, Index> old_to_new;

    for(uint32_t i = 0; i < submesh->index_data->count(); ++i) {
        auto old_index = submesh->index_data->at(i);

        if(old_to_new.count(old_index)) {
            target->index_data->index(old_to_new[old_index]);
        } else {
            auto j = submesh->vertex_data->copy_vertex_to_another(
                *target->vertex_data.get(), submesh->index_data->at(i)
            );

            old_to_new[old_index] = j;
            target->index_data->index(j);
        }
    }

    target->vertex_data->done();
    target->index_data->done();

    return result;
}

MeshPtr AssetManager::new_mesh_from_file(
    const unicode& path,
    const VertexSpecification& desired_specification,
    const MeshLoadOptions& options,
    GarbageCollectMethod garbage_collect) {

    //Load the material
    auto mesh = new_mesh(desired_specification, GARBAGE_COLLECT_NEVER);
    auto loader = window->loader_for(path.encode());
    assert(loader && "Unable to locate a loader for the specified mesh file");

    LoaderOptions loader_options;
    loader_options[MESH_LOAD_OPTIONS_KEY] = options;

    loader->into(mesh, loader_options);

    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect);
    return mesh;
}

MeshPtr AssetManager::new_mesh_from_heightmap(const unicode& image_file, const HeightmapSpecification& spec, GarbageCollectMethod garbage_collect) {
    auto mesh = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);

    window->loader_for("heightmap_loader", image_file)->into(mesh, {
        { "spec", spec},
    });
    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect);

    return mesh;
}

MeshPtr AssetManager::new_mesh_from_heightmap(const TextureID& texture_id, const HeightmapSpecification& spec, GarbageCollectMethod garbage_collect) {
    auto mesh = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);

    loaders::HeightmapLoader loader(texture(texture_id));

    loader.into(*mesh, {
        {"spec", spec},
    });
    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect);

    return mesh;
}


MeshPtr AssetManager::new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<Vec2> &vertices, MeshArrangement arrangement, GarbageCollectMethod garbage_collect) {
    auto mesh = new_mesh(vertex_specification, GARBAGE_COLLECT_NEVER);
    auto submesh = mesh->new_submesh(submesh_name, arrangement);
    int i = 0;
    for(auto v: vertices) {
        mesh->vertex_data->position(v);
        mesh->vertex_data->move_next();
        submesh->index_data->index(i++);
    }

    mesh->vertex_data->done();
    submesh->index_data->done();

    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect);

    return mesh;
}

MeshPtr AssetManager::new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<Vec3> &vertices, MeshArrangement arrangement, GarbageCollectMethod garbage_collect) {
    //FIXME: THis is literally a copy/paste of the function above, we can templatize this
    auto mesh = new_mesh(vertex_specification, GARBAGE_COLLECT_NEVER);

    auto submesh = mesh->new_submesh(submesh_name, arrangement);
    int i = 0;
    for(auto v: vertices) {
        mesh->vertex_data->position(v);
        mesh->vertex_data->move_next();
        submesh->index_data->index(i++);
    }

    mesh->vertex_data->done();
    submesh->index_data->done();

    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect);
    return mesh;
}

MeshPtr AssetManager::new_mesh_as_cube_with_submesh_per_face(float width, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(
                VertexSpecification::DEFAULT,
                GARBAGE_COLLECT_NEVER
                );
    smlt::procedural::mesh::box(m, width, width, width, smlt::procedural::MESH_STYLE_SUBMESH_PER_FACE);
    mesh_manager_.set_garbage_collection_method(m->id(), garbage_collect);
    return m;
}

MeshPtr AssetManager::find_mesh(const std::string& name) {
    return mesh_manager_.find_object(name);
}

void AssetManager::destroy_mesh(MeshID m) {
    mesh_manager_.set_garbage_collection_method(m, GARBAGE_COLLECT_PERIODIC);
}

bool AssetManager::has_mesh(MeshID m) const {
    return mesh_manager_.contains(m);
}

std::size_t AssetManager::mesh_count() const {
    return mesh_manager_.count();
}

MaterialPtr AssetManager::new_material(GarbageCollectMethod garbage_collect) {
    NEW_X(Material, material, material_manager_);
}

void AssetManager::destroy_material(const MaterialID& m) {
    material_manager_.set_garbage_collection_method(m, GARBAGE_COLLECT_PERIODIC);
}

MaterialPtr AssetManager::get_template_material(const unicode& path) {
    /*
     * We keep a cache of the materials we've loaded from file, this massively improves performance
     * and allows sharing of the GPU program during rendering
     * We have to lock access to the templates otherwise bad things happen in multithreaded land.
     */


    // Only load template materials into the base resource manager, to avoid duplication
    if(base_manager() != this) {
        return base_manager()->get_template_material(path);
    }


    MaterialID template_id;

    /* We must load the material outside the lock, because loading the material
     * in thread B might cause a IdleManager::run_sync which will block and deadlock
     * as it will be holding the template_material_lock_ and the thread A
     * will hanging waiting on it.
     */

    bool load_material = false;
    {
        thread::Lock<thread::Mutex> lock(template_material_lock_);
        if(!template_materials_.count(path)) {
            template_materials_[path] = template_id = new_material(GARBAGE_COLLECT_NEVER);
            materials_loading_.insert(template_id);
            load_material = true; //We need to load the material from file
        } else {
            template_id = template_materials_.at(path);
        }
    }

    // Blocking loop while we wait for either this thread or another thread to load the material
    while(materials_loading_.count(template_id)) { // Not really threadsafe...
        if(!load_material && GLThreadCheck::is_current()) {
            /* If we aren't loading the material in this thread, but this is the main thread and the material is loading
             * in another thread we *must* run the idle tasks while we wait for it to finish. Otherwise it will deadlock
             * on a run_sync call */
            window->idle->execute();
        } else if(load_material) {
            /* Otherwise, if we're loading the material, we load it, then remove it from the list */
            S_INFO("Loading material {0} into {1}", path, template_id);
            auto mat = material(template_id);
            if(!mat) {
                S_ERROR("Tried to fetch material with template_id ({0}). But it didn't exist", template_id);
                materials_loading_.erase(template_id);
                throw std::runtime_error(_F("Error loading file: {0}").format(path));
            }

            S_DEBUG("Locating loader for {0}", path);
            auto loader = window->loader_for(path.encode());
            if(!loader) {
                S_ERROR("Unable to find loader for {0}", path);
                materials_loading_.erase(template_id);
                throw std::runtime_error(_F("Unable to find loader for file: {0}").format(path));
            }

            S_DEBUG("Loading...");
            loader->into(mat);
            materials_loading_.erase(template_id);
            S_DEBUG("Material {0} loaded", mat->id());
        }
    }

    return template_id.fetch();
}

MaterialPtr AssetManager::new_material_from_file(const unicode& path, GarbageCollectMethod garbage_collect) {

    MaterialID template_id = get_template_material(path);

    assert(template_id);

    /* Templates are always created in the base manager, we clone from the base manager into this
     * manager (which might be the same manager) */
    auto new_mat = base_manager()->material_manager_.clone(template_id, &this->material_manager_);
    new_mat->manager_ = this;

    auto new_mat_id = new_mat->id();

    S_DEBUG("Cloned material {0} into {1}", template_id, new_mat_id);

    material_manager_.set_garbage_collection_method(new_mat_id, garbage_collect);
    return new_mat;
}

MaterialPtr AssetManager::new_material_from_texture(TextureID texture_id, GarbageCollectMethod garbage_collect) {
    auto m = new_material_from_file(Material::BuiltIns::TEXTURE_ONLY, GARBAGE_COLLECT_NEVER);
    assert(m);

    m->set_diffuse_map(texture(texture_id));

    material_manager_.set_garbage_collection_method(m->id(), garbage_collect);
    return m;
}

MaterialPtr AssetManager::find_material(const std::string& name) {
    return material_manager_.find_object(name);
}

MaterialPtr AssetManager::material(const MaterialID& id) {
    GET_X(Material, material, material_manager_);
}

const MaterialPtr AssetManager::material(const MaterialID& id) const {
    GET_X(Material, material, material_manager_);
}

bool AssetManager::has_material(const MaterialID& m) const {
    return material_manager_.contains(m);
}

std::size_t AssetManager::material_count() const {
    return material_manager_.count();
}

TexturePtr AssetManager::new_texture(uint16_t width, uint16_t height, TextureFormat format, GarbageCollectMethod garbage_collect) {
    NEW_X(Texture, texture, texture_manager_, width, height, format);
}

TexturePtr AssetManager::new_texture_from_file(const unicode& path, GarbageCollectMethod garbage_collect) {
    return new_texture_from_file(path, TextureFlags(), garbage_collect);
}

TexturePtr AssetManager::new_texture_from_file(const unicode& path, TextureFlags flags, GarbageCollectMethod garbage_collect) {
    //Load the texture
    S_DEBUG("Loading texture from file: {0}", path);
    smlt::TexturePtr tex = new_texture(8, 8, TEXTURE_FORMAT_RGBA8888, garbage_collect);

    {
        S_DEBUG("Finding loader for: {0}", path);
        auto loader = window->loader_for(path, LOADER_HINT_TEXTURE);
        if(!loader) {
            S_WARN("Couldn't find loader for texture");
            return smlt::TexturePtr();
        }

        S_DEBUG("Loader found, loading...");
        loader->into(tex);

        if(flags.flip_vertically) {
            S_DEBUG("Flipping texture vertically");
            tex->flip_vertically();
        }

        tex->set_mipmap_generation(flags.mipmap);
        tex->set_texture_wrap(flags.wrap, flags.wrap, flags.wrap);
        tex->set_texture_filter(flags.filter);
        tex->set_auto_upload(flags.auto_upload);
    }

    S_DEBUG("Texture loaded");
    return tex;
}

void AssetManager::destroy_texture(TextureID t) {
    texture_manager_.set_garbage_collection_method(t, GARBAGE_COLLECT_PERIODIC);
}

TexturePtr AssetManager::find_texture(const std::string& name) {
    return texture_manager_.find_object(name);
}

TexturePtr AssetManager::texture(TextureID id) {
    GET_X(Texture, texture, texture_manager_);
}

const TexturePtr AssetManager::texture(TextureID id) const {
    GET_X(Texture, texture, texture_manager_);
}

bool AssetManager::has_texture(TextureID t) const {
    return texture_manager_.contains(t);
}

std::size_t AssetManager::texture_count() const {
    return texture_manager_.count();
}

SoundPtr AssetManager::new_sound_from_file(const unicode& path, GarbageCollectMethod garbage_collect) {
    //Load the sound
    auto snd = sound_manager_.make(this, window->_sound_driver());
    sound_manager_.set_garbage_collection_method(snd->id(), garbage_collect);

    auto loader = window->loader_for(path.encode());

    if(loader) {
        loader->into(snd);
    } else {
        S_ERROR("Unsupported file type: ", path);
    }

    sound_manager_.set_garbage_collection_method(snd->id(), garbage_collect);

    return snd;
}

SoundPtr AssetManager::find_sound(const std::string &name) {
    return sound_manager_.find_object(name);
}

SoundPtr AssetManager::sound(SoundID id) {
    GET_X(Sound, sound, sound_manager_);
}

const SoundPtr AssetManager::sound(SoundID id) const {
    GET_X(Sound, sound, sound_manager_);
}

std::size_t AssetManager::sound_count() const {
    return sound_manager_.count();
}

bool AssetManager::has_sound(SoundID s) const {
    return sound_manager_.contains(s);
}

void AssetManager::destroy_sound(SoundID t) {
    sound_manager_.set_garbage_collection_method(t, GARBAGE_COLLECT_PERIODIC);
}

MaterialPtr AssetManager::clone_material(const MaterialID& mat_id, GarbageCollectMethod garbage_collect) {
    assert(mat_id && "No default material, called to early?");

    auto& manager = base_manager()->material_manager_;
    auto new_mat_id = manager.clone(mat_id);
    manager.set_garbage_collection_method(new_mat_id, garbage_collect);

    assert(new_mat_id);
    return material(new_mat_id);
}

MaterialPtr AssetManager::clone_default_material(GarbageCollectMethod garbage_collect) {
    return clone_material(base_manager()->default_material(), garbage_collect);
}

AssetManager* AssetManager::base_manager() const {
    AssetManager* ret = const_cast<AssetManager*>(this);
    assert(ret && "Unexpectedly failed to cast");

    if(!parent_) {
        return ret;
    }

    // Constness applies to the resource manager itself, not the returned base manager
    while(ret->parent_) {
        ret = ret->parent_;
    }
    return ret;
}

// ========== FONTS ======================

FontPtr AssetManager::new_font_from_file(const unicode& filename, GarbageCollectMethod garbage_collect) {
    auto font = font_manager_.make(this);
    auto font_id = font->id();
    font_manager_.set_garbage_collection_method(font_id, GARBAGE_COLLECT_NEVER);

    try {
        LoaderOptions options;
        window->loader_for(filename)->into(font.get(), options);
        font_manager_.set_garbage_collection_method(font_id, garbage_collect);
    } catch (...) {
        // Make sure we don't leave the font hanging around
        destroy_font(font_id);
        throw;
    }

    return font;
}

FontPtr AssetManager::new_font_from_ttf(const unicode& filename, uint32_t font_size, CharacterSet charset, GarbageCollectMethod garbage_collect) {
    auto font = font_manager_.make(this);
    auto font_id = font->id();

    font_manager_.set_garbage_collection_method(font_id, GARBAGE_COLLECT_NEVER);

    try {
        LoaderOptions options;
        options["size"] = font_size;
        options["charset"] = charset;
        window->loader_for(filename)->into(font.get(), options);

        font_manager_.set_garbage_collection_method(font_id, garbage_collect);
    } catch (...) {
        // Make sure we don't leave the font hanging around
        destroy_font(font_id);
        throw;
    }

    return font;
}

FontPtr AssetManager::find_font(const std::string& name) {
    return font_manager_.find_object(name);
}

void AssetManager::destroy_font(FontID f) {
    font_manager_.set_garbage_collection_method(f, GARBAGE_COLLECT_PERIODIC);
}

FontPtr AssetManager::font(FontID id) {
    GET_X(Font, font, font_manager_);
}

const FontPtr AssetManager::font(FontID id) const {
    GET_X(Font, font, font_manager_);
}

std::size_t AssetManager::font_count() const {
    return font_manager_.count();
}

bool AssetManager::has_font(FontID f) const {
    return font_manager_.contains(f);
}

ParticleScriptPtr AssetManager::new_particle_script_from_file(const unicode& filename, GarbageCollectMethod garbage_collect) {
    auto ps = particle_script_manager_.make(this);
    auto ps_id = ps->id();
    particle_script_manager_.set_garbage_collection_method(ps_id, garbage_collect);

    try {
        LoaderOptions options;
        window->loader_for(filename)->into(ps.get(), options);
    } catch (...) {
        // Make sure we don't leave the font hanging around
        destroy_particle_script(ps_id);
        throw;
    }

    return ps;
}

ParticleScriptPtr AssetManager::find_particle_script(const std::string& name) {
    return particle_script_manager_.find_object(name);
}

void AssetManager::destroy_particle_script(ParticleScriptID id) {
    particle_script_manager_.set_garbage_collection_method(id, GARBAGE_COLLECT_PERIODIC);
}

ParticleScriptPtr AssetManager::particle_script(ParticleScriptID id) {
    GET_X(ParticleScript, particle_script, particle_script_manager_);
}

std::size_t AssetManager::particle_script_count() const {
    return particle_script_manager_.count();
}

bool AssetManager::has_particle_script(ParticleScriptID id) const {
    return particle_script_manager_.contains(id);
}

}
