//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "window_base.h"
#include "resource_manager.h"
#include "loader.h"
#include "procedural/mesh.h"
#include "utils/gl_thread_check.h"

/** FIXME
 *
 * - Write tests to show that all new_X_from_file methods mark resources as uncollected before returning
 * - Think of better GC logic, perhaps collect on every frame?
 */

namespace smlt {

ResourceManager::ResourceManager(WindowBase* window, ResourceManager *parent):
    WindowHolder(window),
    parent_(parent) {

    font_manager_.reset(new FontManager());

    if(parent_) {
        base_manager()->register_child(this);
    }
}

ResourceManager::~ResourceManager() {
    if(parent_) {
        base_manager()->unregister_child(this);
    }
}

bool ResourceManager::init() {
    if(base_manager() != this) {
        // Only the base manager needs to load default materials and textures
        return true;
    }

    default_heading_font_ = new_font_from_ttf(default_font_filename(), 64).fetch();
    default_subheading_font_ = new_font_from_ttf(default_font_filename(), 24).fetch();
    default_body_font_ = new_font_from_ttf(default_font_filename(), 18).fetch();

    //FIXME: Should lock the default texture and material during construction!
    //Create the default blank texture
    default_texture_id_ = new_texture(GARBAGE_COLLECT_NEVER);

    auto tex = texture(default_texture_id_);

    tex->resize(1, 1);
    tex->set_bpp(32);

    tex->data()[0] = 255;
    tex->data()[1] = 255;
    tex->data()[2] = 255;
    tex->data()[3] = 255;
    tex->upload();

    //Maintain ref-count
    default_material_id_ = new_material_from_file(default_material_filename(), GARBAGE_COLLECT_NEVER);

    //Set the default material's first texture to the default (white) texture
    material(default_material_id_)->pass(0)->set_texture_unit(0, default_texture_id_);

    return true;
}

void ResourceManager::update(double dt) {
    MaterialManager::each([dt](Material* mat) {
        mat->update_controllers(dt);
        mat->update(dt);
    });
}

void ResourceManager::run_garbage_collection() {
    for(auto child: children_) {
        child->run_garbage_collection();
    }

    auto now = std::chrono::system_clock::now();
    auto diff = now - last_collection_;

    if(std::chrono::duration_cast<std::chrono::seconds>(diff).count() >= 5) {
        //Garbage collect all the things
        MeshManager::garbage_collect();
        MaterialManager::garbage_collect();
        TextureManager::garbage_collect();
        SoundManager::garbage_collect();

        font_manager_->garbage_collect();

        last_collection_ = std::chrono::system_clock::now();
    }
}

MeshPtr ResourceManager::mesh(MeshID m) {
    if(parent_ && !has_mesh(m)) {
        return parent_->mesh(m);
    }

    return MeshManager::get(m).lock();
}

const MeshPtr ResourceManager::mesh(MeshID m) const {
    if(parent_ && !has_mesh(m)) {
        return parent_->mesh(m);
    }

    return MeshManager::get(m).lock();
}

MeshID ResourceManager::new_mesh(VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect) {
    MeshID result = MeshManager::make(garbage_collect, this, vertex_specification);
    return result;
}

MeshID ResourceManager::new_animated_mesh(
    VertexSpecification vertex_specification,
    MeshAnimationType animation_type,
    uint32_t animation_frames,
    GarbageCollectMethod garbage_collect
) {
    MeshID result = MeshManager::make(
        garbage_collect,
        this,
        vertex_specification
    );

    mesh(result)->enable_animation(animation_type, animation_frames);

    // Mark as uncollected as the user didn't actually grab it
    MeshManager::mark_as_uncollected(result);

    return result;
}

MeshID ResourceManager::new_mesh_from_submesh(SubMesh* submesh, GarbageCollectMethod garbage_collect) {
    VertexSpecification spec = submesh->vertex_data->specification();
    MeshID result = new_mesh(spec, garbage_collect);

    SubMesh* target = mesh(result)->new_submesh_with_material(
        submesh->name(),
        submesh->material_id(),
        submesh->arrangement(),
        VERTEX_SHARING_MODE_SHARED
    );

    std::unordered_map<Index, Index> old_to_new;

    for(uint32_t i = 0; i < submesh->index_data->count(); ++i) {
        auto old_index = submesh->index_data->at(i);

        if(old_to_new.count(old_index)) {
            target->index_data->index(old_to_new[old_index]);
        } else {
            old_to_new[old_index] = submesh->vertex_data->copy_vertex_to_another(
                *target->vertex_data.get(), submesh->index_data->at(i)
            );
            target->index_data->index(i);
        }
    }

    target->vertex_data->done();
    target->index_data->done();

    return result;
}

MeshID ResourceManager::new_mesh_from_file(const unicode& path, GarbageCollectMethod garbage_collect) {
    //Load the material
    smlt::MeshID mesh_id = new_mesh(VertexSpecification::POSITION_ONLY, garbage_collect);
    auto loader = window->loader_for(path.encode());
    assert(loader && "Unable to locate a loader for the specified mesh file");

    loader->into(mesh(mesh_id));

    MeshManager::mark_as_uncollected(mesh_id);
    return mesh_id;
}

MeshID ResourceManager::new_mesh_from_tmx_file(const unicode& tmx_file, const unicode& layer_name, float tile_render_size, GarbageCollectMethod garbage_collect) {
    smlt::MeshID mesh_id = new_mesh(VertexSpecification::DEFAULT, garbage_collect);
    window->loader_for(tmx_file.encode())->into(mesh(mesh_id), {
        {"layer", layer_name},
        {"render_size", tile_render_size}
    });
    MeshManager::mark_as_uncollected(mesh_id);
    return mesh_id;
}

MeshID ResourceManager::new_mesh_from_heightmap(const unicode& image_file, const HeightmapSpecification& spec, GarbageCollectMethod garbage_collect) {
    smlt::MeshID mesh_id = new_mesh(VertexSpecification::DEFAULT, garbage_collect);
    window->loader_for("heightmap_loader", image_file)->into(mesh(mesh_id), {
        { "spec", spec},
    });
    MeshManager::mark_as_uncollected(mesh_id);
    return mesh_id;
}

MeshID ResourceManager::new_mesh_as_cube(float width, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh(
        VertexSpecification::DEFAULT,
        garbage_collect
    );
    smlt::procedural::mesh::cube(mesh(m), width);
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManager::new_mesh_as_box(float width, float height, float depth, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh(VertexSpecification::DEFAULT, garbage_collect);
    smlt::procedural::mesh::box(mesh(m), width, height, depth);
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManager::new_mesh_as_sphere(float diameter, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh(VertexSpecification::DEFAULT, garbage_collect);
    smlt::procedural::mesh::sphere(mesh(m), diameter);
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManager::new_mesh_as_rectangle(float width, float height, const Vec2& offset, MaterialID material, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh(VertexSpecification::DEFAULT, garbage_collect);
    smlt::procedural::mesh::rectangle(mesh(m), width, height, offset.x, offset.y, 0, false, material);
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManager::new_mesh_as_cylinder(float diameter, float length, int segments, int stacks, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh(VertexSpecification::DEFAULT, garbage_collect);
    smlt::procedural::mesh::cylinder(mesh(m), diameter, length, segments, stacks);
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManager::new_mesh_as_capsule(float diameter, float length, int segments, int stacks, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh(VertexSpecification::DEFAULT, garbage_collect);
    smlt::procedural::mesh::capsule(mesh(m), diameter, length, segments, 1, stacks);
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManager::new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<Vec2> &vertices, MeshArrangement arrangement, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh(vertex_specification, garbage_collect);

    auto new_mesh = mesh(m);
    auto submesh = new_mesh->new_submesh(submesh_name, arrangement);
    int i = 0;
    for(auto v: vertices) {
        new_mesh->shared_data->position(v);
        new_mesh->shared_data->move_next();
        submesh->index_data->index(i++);
    }

    new_mesh->shared_data->done();
    submesh->index_data->done();

    MeshManager::mark_as_uncollected(m);

    return m;
}

MeshID ResourceManager::new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<Vec3> &vertices, MeshArrangement arrangement, GarbageCollectMethod garbage_collect) {
    //FIXME: THis is literally a copy/paste of the function above, we can templatize this
    MeshID m = new_mesh(vertex_specification, garbage_collect);

    auto new_mesh = mesh(m);
    auto submesh = new_mesh->new_submesh(submesh_name, arrangement);
    int i = 0;
    for(auto v: vertices) {
        new_mesh->shared_data->position(v);
        new_mesh->shared_data->move_next();
        submesh->index_data->index(i++);
    }

    new_mesh->shared_data->done();
    submesh->index_data->done();
    MeshManager::mark_as_uncollected(m);
    return m;
}

MeshID ResourceManager::new_mesh_with_alias(const std::string& alias, VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh(vertex_specification, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManager::new_mesh_with_alias_from_file(const std::string &alias, const unicode& path, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh_from_file(path, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManager::new_mesh_with_alias_as_cube(const std::string& alias, float width, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh_as_cube(width, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManager::new_mesh_with_alias_as_sphere(const std::string &alias, float diameter, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh_as_sphere(diameter, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManager::new_mesh_with_alias_as_rectangle(const std::string& alias, float width, float height, const Vec2& offset, smlt::MaterialID material, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh_as_rectangle(width, height, offset, material, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }
    return m;
}

MeshID ResourceManager::new_mesh_with_alias_as_cylinder(const std::string &alias, float diameter, float length, int segments, int stacks, GarbageCollectMethod garbage_collect) {
    MeshID m = new_mesh_as_cylinder(diameter, length, segments, stacks, garbage_collect);
    try {
        MeshManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_mesh(m);
        throw;
    }

    return m;
}

MeshID ResourceManager::get_mesh_with_alias(const std::string& alias) {
    return MeshManager::get_by_alias(alias);
}

void ResourceManager::delete_mesh(MeshID m) {
    mesh(m)->enable_gc();
}

bool ResourceManager::has_mesh(MeshID m) const {
    return MeshManager::contains(m);
}

uint32_t ResourceManager::mesh_count() const {
    return MeshManager::count();
}

MaterialID ResourceManager::new_material(GarbageCollectMethod garbage_collect) {
    MaterialID result = MaterialManager::make(garbage_collect, this);
    return result;
}

void ResourceManager::delete_material(MaterialID m) {
    material(m)->enable_gc();
}

MaterialID ResourceManager::get_template_material(const unicode& path) {
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
        std::lock_guard<std::mutex> lock(template_material_lock_);
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
            L_INFO(_F("Loading material {0} into {1}").format(path, template_id));
            auto mat = material(template_id);
            window->loader_for(path.encode())->into(mat);
            materials_loading_.erase(template_id);
        }
    }

    // Templates should never have garbage collection enabled
    assert(!template_id.fetch()->uses_gc());

    return template_id;
}

MaterialID ResourceManager::new_material_from_file(const unicode& path, GarbageCollectMethod garbage_collect) {

    MaterialID template_id = get_template_material(path);

    assert(template_id);

    /* Take the template, clone it, and set garbage_collection appropriately */
    auto new_mat = material(template_id)->new_clone(this, garbage_collect).fetch();
    new_mat->enable_gc((garbage_collect == GARBAGE_COLLECT_NEVER) ? false: true);
    mark_material_as_uncollected(new_mat->id());

    //L_DEBUG(_F("Cloned material {0} into {1}").format(template_id, new_mat->id()));

    return new_mat->id();
}

MaterialID ResourceManager::new_material_with_alias(const std::string& alias, GarbageCollectMethod garbage_collect) {
    MaterialID m = new_material(garbage_collect);
    assert(m);

    try {
        MaterialManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_material(m);
        throw;
    }
    return m;
}

MaterialID ResourceManager::new_material_with_alias_from_file(const std::string &alias, const unicode& path, GarbageCollectMethod garbage_collect) {
    MaterialID m = new_material_from_file(path, garbage_collect);
    assert(m);

    try {
        MaterialManager::manager_store_alias(alias, m);
    } catch(...) {
        delete_material(m);
        throw;
    }
    return m;
}

MaterialID ResourceManager::new_material_from_texture(TextureID texture_id, GarbageCollectMethod garbage_collect) {
    MaterialID m = new_material_from_file(Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING, garbage_collect);
    assert(m);

    material(m)->set_texture_unit_on_all_passes(0, texture_id);
    mark_material_as_uncollected(m); //FIXME: Race-y
    return m;
}

MaterialID ResourceManager::get_material_with_alias(const std::string& alias) {
    return MaterialManager::get_by_alias(alias);
}

MaterialPtr ResourceManager::material(MaterialID mid) {
    if(parent_ && !has_material(mid)) {
        return parent_->material(mid);
    }

    return MaterialManager::get(mid).lock();
}

const MaterialPtr ResourceManager::material(MaterialID mid) const {
    if(parent_ && !has_material(mid)) {
        return parent_->material(mid);
    }

    return MaterialManager::get(mid).lock();
}

bool ResourceManager::has_material(MaterialID m) const {
    return MaterialManager::contains(m);
}

uint32_t ResourceManager::material_count() const {
    return MaterialManager::count();
}

void ResourceManager::mark_material_as_uncollected(MaterialID t) {
    MaterialManager::mark_as_uncollected(t);
}

TextureID ResourceManager::new_texture(GarbageCollectMethod garbage_collect) {
    return TextureManager::make(garbage_collect, this);
}

TextureID ResourceManager::new_texture_from_file(const unicode& path, TextureFlags flags, GarbageCollectMethod garbage_collect) {
    //Load the texture
    auto tex = texture(new_texture(garbage_collect));
    window->loader_for(path, LOADER_HINT_TEXTURE)->into(tex);

    if(flags.flip_vertically) {
        tex->flip_vertically();
    }

    tex->upload(
        flags.mipmap,
        flags.wrap,
        flags.filter,
        false
    );

    mark_texture_as_uncollected(tex->id());
    return tex->id();
}

void ResourceManager::delete_texture(TextureID t) {
    texture(t)->enable_gc();
}

TextureID ResourceManager::new_texture_with_alias(const std::string& alias, GarbageCollectMethod garbage_collect) {
    TextureID t = new_texture(garbage_collect);
    try {
        TextureManager::manager_store_alias(alias, t);
    } catch(...) {
        delete_texture(t);
        throw;
    }
    return t;
}

TextureID ResourceManager::new_texture_with_alias_from_file(const std::string &alias, const unicode& path, TextureFlags flags, GarbageCollectMethod garbage_collect) {
    TextureID t = new_texture_from_file(path, flags, garbage_collect);
    try {
        TextureManager::manager_store_alias(alias, t);
    } catch(...) {
        delete_texture(t);
        throw;
    }
    return t;
}

TextureID ResourceManager::get_texture_with_alias(const std::string& alias) {
    return TextureManager::get_by_alias(alias);
}

TexturePtr ResourceManager::texture(TextureID t) {
    if(parent_ && !has_texture(t)) {
        return parent_->texture(t);
    }

    return TexturePtr(TextureManager::get(t).lock());
}

const TexturePtr ResourceManager::texture(TextureID t) const {
    if(parent_ && !has_texture(t)) {
        return parent_->texture(t);
    }

    return TexturePtr(TextureManager::get(t).lock());
}

bool ResourceManager::has_texture(TextureID t) const {
    return TextureManager::contains(t);
}

uint32_t ResourceManager::texture_count() const {
    return TextureManager::count();
}

void ResourceManager::mark_texture_as_uncollected(TextureID t) {
    TextureManager::mark_as_uncollected(t);
}

SoundID ResourceManager::new_sound(GarbageCollectMethod garbage_collect) {
    return SoundManager::make(garbage_collect, this);
}

SoundID ResourceManager::new_sound_from_file(const unicode& path, GarbageCollectMethod garbage_collect) {
    //Load the sound
    auto snd = sound(new_sound(garbage_collect));
    window->loader_for(path.encode())->into(snd);
    return snd->id();
}

SoundID ResourceManager::new_sound_with_alias(const std::string &alias, GarbageCollectMethod garbage_collect) {
    SoundID s = new_sound(garbage_collect);
    try {
        SoundManager::manager_store_alias(alias, s);
    } catch(...) {
        delete_sound(s);
        throw;
    }
    return s;
}

SoundID ResourceManager::new_sound_with_alias_from_file(const std::string& alias, const unicode& path, GarbageCollectMethod garbage_collect) {
    SoundID s = new_sound_from_file(path, garbage_collect);
    try {
        SoundManager::manager_store_alias(alias, s);
    } catch(...) {
        delete_sound(s);
        throw;
    }
    return s;
}

SoundID ResourceManager::get_sound_with_alias(const std::string &alias) {
    return SoundManager::get_by_alias(alias);
}

SoundPtr ResourceManager::sound(SoundID s) {
    if(parent_ && !has_sound(s)) {
        return parent_->sound(s);
    }

    return SoundManager::get(s).lock();
}

const SoundPtr ResourceManager::sound(SoundID s) const {
    if(parent_ && !has_sound(s)) {
        return parent_->sound(s);
    }

    return SoundManager::get(s).lock();
}

uint32_t ResourceManager::sound_count() const {
    return SoundManager::count();
}

bool ResourceManager::has_sound(SoundID s) const {
    return SoundManager::contains(s);
}

void ResourceManager::delete_sound(SoundID t) {
    sound(t)->enable_gc();
}

TextureID ResourceManager::default_texture_id() const {
    if(base_manager() != this) {
        return base_manager()->default_texture_id();
    } else {
        return default_texture_id_;
    }
}

FontID ResourceManager::default_font_id(DefaultFontStyle style) const {
    if(base_manager() != this) {
        return base_manager()->default_font_id(style);
    } else {
        switch(style) {
        case DEFAULT_FONT_STYLE_HEADING:
            return default_heading_font_->id();
        case DEFAULT_FONT_STYLE_SUBHEADING:
            return default_subheading_font_->id();
        default:
            return default_body_font_->id();
        }
    }
}

MaterialID ResourceManager::default_material_id() const {
    if(base_manager() != this) {
        return base_manager()->default_material_id();
    } else {
        return default_material_id_;
    }
}

unicode ResourceManager::default_font_filename() const {
    return window->resource_locator->locate_file("simulant/fonts/orbitron/Orbitron-Regular.ttf");
}

unicode ResourceManager::default_material_filename() const {
    return window->resource_locator->locate_file(Material::BuiltIns::MULTITEXTURE2_MODULATE_WITH_LIGHTING);
}

// ========== FONTS ======================

FontID ResourceManager::new_font_from_ttf(const unicode& filename, uint32_t font_size, CharacterSet charset, GarbageCollectMethod garbage_collect) {
    auto font_id = font_manager_->make(garbage_collect, this);
    auto font = font_id.fetch();

    try {
        LoaderOptions options;
        options["size"] = font_size;
        options["charset"] = charset;
        window->loader_for(filename)->into(font.get(), options);

        mark_font_as_uncollected(font_id);
    } catch (...) {
        // Make sure we don't leave the font hanging around
        delete_font(font_id);
        throw;
    }

    return font_id;
}

FontID ResourceManager::new_font_with_alias_from_ttf(const std::string& alias, const unicode& filename, uint32_t font_size, CharacterSet charset, GarbageCollectMethod garbage_collect) {
    auto fid = new_font_from_ttf(filename, font_size, charset, garbage_collect);
    try {
        font_manager_->manager_store_alias(alias, fid);
    } catch(...) {
        delete_font(fid);
        throw;
    }
    return fid;
}

FontID ResourceManager::get_font_with_alias(const std::string& alias) {
    return font_manager_->get_by_alias(alias);
}

void ResourceManager::delete_font(FontID f) {
    f.fetch()->enable_gc();
}

FontPtr ResourceManager::font(FontID f) {
    if(parent_ && !font_manager_->contains(f)) {
        return parent_->font_manager_->get(f).lock();
    }

    return font_manager_->get(f).lock();
}

const FontPtr ResourceManager::font(FontID f) const {
    if(parent_ && !font_manager_->contains(f)) {
        return parent_->font_manager_->get(f).lock();
    }
    return font_manager_->get(f).lock();
}

uint32_t ResourceManager::font_count() const {
    return font_manager_->count();
}

bool ResourceManager::has_font(FontID f) const {
    return font_manager_->contains(f);
}

void ResourceManager::mark_font_as_uncollected(FontID f) {
    font_manager_->mark_as_uncollected(f);
}

}
