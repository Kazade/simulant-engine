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

/** FIXME
 *
 * - Write tests to show that all new_X_from_file methods mark resources as uncollected before returning
 * - Think of better GC logic, perhaps collect on every frame?
 */

namespace smlt {

#define HEADING_FONT "simulant/fonts/orbitron/orbitron-regular-48.fnt"
#define BODY_FONT "simulant/fonts/orbitron/orbitron-regular-18.fnt"

AssetManager::AssetManager(Window* window, AssetManager *parent):
    WindowHolder(window),
    parent_(parent) {

    if(parent_) {
        L_DEBUG(_F("Registering new resource manager: {0}").format(this));
        base_manager()->register_child(this);
    } else {
        L_DEBUG(_F("Created base manager: {0}").format(this));
    }
}

AssetManager::~AssetManager() {
    if(parent_) {
        L_DEBUG(_F("Unregistering resource manager: {0}").format(this));
        base_manager()->unregister_child(this);
        parent_ = nullptr;
    } else {
        if(!children_.empty()) {
            L_WARN("Destroyed base manager while children remain");
        }

        L_DEBUG(_F("Destroyed base manager: {0}").format(this));
    }
}

bool AssetManager::init() {
    if(base_manager() != this) {
        // Only the base manager needs to load default materials and textures
        L_DEBUG("Not the base manager, so not initializing");
        return true;
    }

    L_DEBUG("Initalizing default materials, textures, and fonts");

    //FIXME: Should lock the default texture and material during construction!
    //Create the default blank texture
    default_texture_id_ = new_texture(GARBAGE_COLLECT_NEVER);

    L_DEBUG("- Created texture");

    assert(default_texture_id_);

    {
        // FIXME: Race condition between these two lines?
        auto tex = texture(default_texture_id_);
        auto texlock = tex->lock();

        /* 8x8 is the smallest size we can do without the Dreamcast pvr library
         * complaining that it's not a valid size :/
         */
        tex->resize(8, 8);
        tex->set_format(TEXTURE_FORMAT_RGBA8888);
        for(uint32_t i = 0; i < 64 * 4; ++i) {
            tex->data()[i] = 255;
        }
        tex->mark_data_changed();

        L_DEBUG("- Generated texture data");
    }

    //Maintain ref-count
    default_material_id_ = new_material_from_file(default_material_filename(), GARBAGE_COLLECT_NEVER);
    L_DEBUG("- Created material");

    assert(default_material_id_);

    auto mat = material(default_material_id_);

    assert(mat && "Material was missing?!");

    L_DEBUG("- Applying texture to material");

    //Set the default material's first texture to the default (white) texture
    mat->set_diffuse_map(default_texture_id_);

    L_DEBUG("- Loading fonts");
    default_heading_font_ = new_font_from_file(HEADING_FONT);
    default_body_font_ = new_font_from_file(BODY_FONT);

    L_DEBUG("Finished initializing defaults");

    return true;
}

void AssetManager::update(float dt) {
    material_manager_.each([dt](uint32_t, MaterialPtr mat) {
        mat->update(dt);
    });

    texture_manager_.each([dt](uint32_t, TexturePtr tex) {
        tex->update(dt);
    });
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
}

void AssetManager::set_garbage_collection_grace_period(uint32_t period) {
    material_manager_.set_garbage_collection_grace_period(period);
    texture_manager_.set_garbage_collection_grace_period(period);
    font_manager_.set_garbage_collection_grace_period(period);
    sound_manager_.set_garbage_collection_grace_period(period);
    mesh_manager_.set_garbage_collection_grace_period(period);
}

MeshPtr AssetManager::mesh(MeshID m) {
    if(parent_ && !has_mesh(m)) {
        return parent_->mesh(m);
    }

    return mesh_manager_.get(m);
}

const MeshPtr AssetManager::mesh(MeshID m) const {
    if(parent_ && !has_mesh(m)) {
        return parent_->mesh(m);
    }

    return mesh_manager_.get(m);
}

MeshPtr AssetManager::new_mesh(VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect) {
    auto result = mesh_manager_.make(this, vertex_specification).fetch();
    mesh_manager_.set_garbage_collection_method(result->id(), garbage_collect);
    return result;
}

MeshPtr AssetManager::new_mesh_from_submesh(SubMesh* submesh, GarbageCollectMethod garbage_collect) {
    VertexSpecification spec = submesh->vertex_data->vertex_specification();
    auto result = new_mesh(spec, garbage_collect);

    SubMesh* target = result->new_submesh_with_material(
        submesh->name(),
        submesh->material_id(),
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

MeshPtr AssetManager::new_mesh_from_file(const unicode& path, const MeshLoadOptions& options, GarbageCollectMethod garbage_collect) {
    //Load the material
    auto mesh = new_mesh(VertexSpecification::POSITION_ONLY, GARBAGE_COLLECT_NEVER);
    auto loader = window->loader_for(path.encode());
    assert(loader && "Unable to locate a loader for the specified mesh file");

    LoaderOptions loader_options;
    loader_options[MESH_LOAD_OPTIONS_KEY] = options;

    loader->into(mesh, loader_options);

    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect, true);
    return mesh;
}

MeshPtr AssetManager::new_mesh_from_tmx_file(const unicode& tmx_file, const unicode& layer_name, float tile_render_size, GarbageCollectMethod garbage_collect) {
    auto mesh = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);

    window->loader_for(tmx_file.encode())->into(mesh, {
        {"layer", layer_name},
        {"render_size", tile_render_size}
    });

    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect, true);
    return mesh;
}

MeshPtr AssetManager::new_mesh_from_heightmap(const unicode& image_file, const HeightmapSpecification& spec, GarbageCollectMethod garbage_collect) {
    auto mesh = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);

    window->loader_for("heightmap_loader", image_file)->into(mesh, {
        { "spec", spec},
    });
    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect, true);

    return mesh;
}

MeshPtr AssetManager::new_mesh_as_cube(float width, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(
        VertexSpecification::DEFAULT,
        GARBAGE_COLLECT_NEVER
    );
    smlt::procedural::mesh::cube(m, width);
    mesh_manager_.set_garbage_collection_method(m->id(), garbage_collect, true);
    return m;
}

MeshPtr AssetManager::new_mesh_as_cube_with_submesh_per_face(float width, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(
        VertexSpecification::DEFAULT,
        GARBAGE_COLLECT_NEVER
    );
    smlt::procedural::mesh::box(m, width, width, width, smlt::procedural::MESH_STYLE_SUBMESH_PER_FACE);
    mesh_manager_.set_garbage_collection_method(m->id(), garbage_collect, true);
    return m;

}

MeshPtr AssetManager::new_mesh_as_box(float width, float height, float depth, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);
    smlt::procedural::mesh::box(m, width, height, depth);
    mesh_manager_.set_garbage_collection_method(m->id(), garbage_collect, true);
    return m;
}

MeshPtr AssetManager::new_mesh_as_sphere(float diameter, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);
    smlt::procedural::mesh::sphere(m, diameter);
    mesh_manager_.set_garbage_collection_method(m->id(), garbage_collect, true);
    return m;
}

MeshPtr AssetManager::new_mesh_as_rectangle(float width, float height, const Vec2& offset, MaterialID material, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);
    smlt::procedural::mesh::rectangle(m, width, height, offset.x, offset.y, 0, false, material);
    mesh_manager_.set_garbage_collection_method(m->id(), garbage_collect, true);
    return m;
}

MeshPtr AssetManager::new_mesh_as_cylinder(float diameter, float length, int segments, int stacks, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);
    smlt::procedural::mesh::cylinder(m, diameter, length, segments, stacks);
    mesh_manager_.set_garbage_collection_method(m->id(), garbage_collect, true);
    return m;
}

MeshPtr AssetManager::new_mesh_as_capsule(float diameter, float length, int segments, int stacks, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);
    smlt::procedural::mesh::capsule(m, diameter, length, segments, 1, stacks);
    mesh_manager_.set_garbage_collection_method(m->id(), garbage_collect, true);
    return m;
}

MeshPtr AssetManager::new_mesh_as_icosphere(float diameter, int subdivisions, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);
    m->new_submesh_as_icosphere("icosphere", MaterialID(), diameter, subdivisions);
    mesh_manager_.set_garbage_collection_method(m->id(), garbage_collect, true);
    return m;
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

    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect, true);

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

    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect, true);
    return mesh;
}

MeshPtr AssetManager::new_mesh_with_alias(const std::string& alias, VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh(vertex_specification, garbage_collect);
    try {
        mesh_manager_.store_alias(alias, m->id());
    } catch(...) {
        delete_mesh(m->id());
        throw;
    }
    return m;
}

MeshPtr AssetManager::new_mesh_with_alias_from_file(const std::string &alias, const unicode& path, const MeshLoadOptions& options, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh_from_file(path, options, garbage_collect);
    try {
        mesh_manager_.store_alias(alias, m->id());
    } catch(...) {
        delete_mesh(m->id());
        throw;
    }
    return m;
}

MeshPtr AssetManager::new_mesh_with_alias_as_cube(const std::string& alias, float width, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh_as_cube(width, garbage_collect);
    try {
        mesh_manager_.store_alias(alias, m->id());
    } catch(...) {
        delete_mesh(m->id());
        throw;
    }
    return m;
}

MeshPtr AssetManager::new_mesh_with_alias_as_sphere(const std::string &alias, float diameter, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh_as_sphere(diameter, garbage_collect);
    try {
        mesh_manager_.store_alias(alias, m->id());
    } catch(...) {
        delete_mesh(m->id());
        throw;
    }
    return m;
}

MeshPtr AssetManager::new_mesh_with_alias_as_rectangle(const std::string& alias, float width, float height, const Vec2& offset, smlt::MaterialID material, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh_as_rectangle(width, height, offset, material, garbage_collect);
    try {
        mesh_manager_.store_alias(alias, m->id());
    } catch(...) {
        delete_mesh(m->id());
        throw;
    }
    return m;
}

MeshPtr AssetManager::new_mesh_with_alias_as_cylinder(const std::string &alias, float diameter, float length, int segments, int stacks, GarbageCollectMethod garbage_collect) {
    auto m = new_mesh_as_cylinder(diameter, length, segments, stacks, garbage_collect);
    try {
        mesh_manager_.store_alias(alias, m->id());
    } catch(...) {
        delete_mesh(m->id());
        throw;
    }

    return m;
}

MeshPtr AssetManager::get_mesh_with_alias(const std::string& alias) {
    return mesh(mesh_manager_.get_id_from_alias(alias));
}

void AssetManager::delete_mesh(MeshID m) {
    mesh_manager_.set_garbage_collection_method(m, GARBAGE_COLLECT_PERIODIC);
}

bool AssetManager::has_mesh(MeshID m) const {
    return mesh_manager_.contains(m);
}

uint32_t AssetManager::mesh_count() const {
    return mesh_manager_.count();
}

MaterialPtr AssetManager::new_material(GarbageCollectMethod garbage_collect) {
    MaterialID result = material_manager_.make(this);
    material_manager_.set_garbage_collection_method(result, garbage_collect);
    return result.fetch();
}

void AssetManager::delete_material(MaterialID m) {
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
            auto loader = window->loader_for(path.encode());
            if(!loader) {
                L_ERROR(_F("Unable to find loader for {0}").format(path));
                materials_loading_.erase(template_id);
                throw std::runtime_error(_F("Unable to find loader for file: {0}").format(path));
            }

            loader->into(mat);
            materials_loading_.erase(template_id);
        }
    }

    return template_id.fetch();
}

MaterialPtr AssetManager::new_material_from_file(const unicode& path, GarbageCollectMethod garbage_collect) {

    MaterialID template_id = get_template_material(path);

    assert(template_id);

    /* Templates are always created in the base manager, we clone from the base manager into this
     * manager (which might be the same manager) */
    auto new_mat = base_manager()->material_manager_.clone(template_id, &this->material_manager_).fetch();
    new_mat->manager_ = this;

    auto new_mat_id = new_mat->id();

    L_DEBUG(_F("Cloned material {0} into {1}").format(template_id, new_mat_id));

    material_manager_.set_garbage_collection_method(new_mat_id, garbage_collect, true);
    return new_mat;
}

MaterialPtr AssetManager::new_material_with_alias(const std::string& alias, GarbageCollectMethod garbage_collect) {
    auto m = new_material(garbage_collect);
    assert(m);

    try {
        material_manager_.store_alias(alias, m->id());
    } catch(...) {
        delete_material(m->id());
        throw;
    }
    return m;
}

MaterialPtr AssetManager::new_material_with_alias_from_file(const std::string &alias, const unicode& path, GarbageCollectMethod garbage_collect) {
    auto m = new_material_from_file(path, garbage_collect);
    assert(m);

    try {
        material_manager_.store_alias(alias, m->id());
    } catch(...) {
        delete_material(m->id());
        throw;
    }
    return m;
}

MaterialPtr AssetManager::new_material_from_texture(TextureID texture_id, GarbageCollectMethod garbage_collect) {
    auto m = new_material_from_file(Material::BuiltIns::TEXTURE_ONLY, GARBAGE_COLLECT_NEVER);
    assert(m);

    m->set_diffuse_map(texture_id);

    material_manager_.set_garbage_collection_method(m->id(), garbage_collect, true);
    return m;
}

MaterialPtr AssetManager::get_material_with_alias(const std::string& alias) {
    return material_manager_.get_id_from_alias(alias).fetch();
}

MaterialPtr AssetManager::material(MaterialID mid) {
    if(parent_ && !has_material(mid)) {
        return parent_->material(mid);
    }

    return material_manager_.get(mid);
}

const MaterialPtr AssetManager::material(MaterialID mid) const {
    if(parent_ && !has_material(mid)) {
        return parent_->material(mid);
    }

    return material_manager_.get(mid);
}

bool AssetManager::has_material(MaterialID m) const {
    return material_manager_.contains(m);
}

uint32_t AssetManager::material_count() const {
    return material_manager_.count();
}

TexturePtr AssetManager::new_texture(GarbageCollectMethod garbage_collect) {
    auto ret = texture_manager_.make(this);
    texture_manager_.set_garbage_collection_method(ret, garbage_collect);
    return ret.fetch();
}

TexturePtr AssetManager::new_texture_from_file(const unicode& path, TextureFlags flags, GarbageCollectMethod garbage_collect) {
    //Load the texture
    auto tex = texture(new_texture(GARBAGE_COLLECT_NEVER));

    auto texlock = tex->lock();
    {
        window->loader_for(path, LOADER_HINT_TEXTURE)->into(tex);

        if(flags.flip_vertically) {
            tex->flip_vertically();
        }

        tex->set_mipmap_generation(flags.mipmap);
        tex->set_texture_wrap(flags.wrap, flags.wrap, flags.wrap);
        tex->set_texture_filter(flags.filter);
        tex->set_auto_upload(flags.auto_upload);
        tex->mark_data_changed();
    }

    texture_manager_.set_garbage_collection_method(tex->id(), garbage_collect, true);
    return tex;
}

void AssetManager::delete_texture(TextureID t) {
    texture_manager_.set_garbage_collection_method(t, GARBAGE_COLLECT_PERIODIC);
}

TexturePtr AssetManager::new_texture_with_alias(const std::string& alias, GarbageCollectMethod garbage_collect) {
    auto t = new_texture(garbage_collect);
    try {
        texture_manager_.store_alias(alias, t->id());
    } catch(...) {
        delete_texture(t->id());
        throw;
    }
    return t;
}

TexturePtr AssetManager::new_texture_with_alias_from_file(const std::string &alias, const unicode& path, TextureFlags flags, GarbageCollectMethod garbage_collect) {
    auto t = new_texture_from_file(path, flags, garbage_collect);
    try {
        texture_manager_.store_alias(alias, t->id());
    } catch(...) {
        delete_texture(t->id());
        throw;
    }
    return t;
}

TexturePtr AssetManager::get_texture_with_alias(const std::string& alias) {
    return texture_manager_.get_id_from_alias(alias).fetch();
}

TexturePtr AssetManager::texture(TextureID t) {
    if(parent_ && !has_texture(t)) {
        return parent_->texture(t);
    }

    return TexturePtr(texture_manager_.get(t));
}

const TexturePtr AssetManager::texture(TextureID t) const {
    if(parent_ && !has_texture(t)) {
        return parent_->texture(t);
    }

    return TexturePtr(texture_manager_.get(t));
}

bool AssetManager::has_texture(TextureID t) const {
    return texture_manager_.contains(t);
}

uint32_t AssetManager::texture_count() const {
    return texture_manager_.count();
}

SoundPtr AssetManager::new_sound(GarbageCollectMethod garbage_collect) {
    auto ret = sound_manager_.make(this, window->_sound_driver()).fetch();
    sound_manager_.set_garbage_collection_method(ret->id(), garbage_collect);
    return ret;
}

SoundPtr AssetManager::new_sound_from_file(const unicode& path, GarbageCollectMethod garbage_collect) {
    //Load the sound
    auto snd = sound(new_sound(GARBAGE_COLLECT_NEVER));
    window->loader_for(path.encode())->into(snd);

    sound_manager_.set_garbage_collection_method(snd->id(), garbage_collect, true);

    return snd;
}

SoundPtr AssetManager::new_sound_with_alias(const std::string &alias, GarbageCollectMethod garbage_collect) {
    auto s = new_sound(garbage_collect);
    try {
        sound_manager_.store_alias(alias, s->id());
    } catch(...) {
        delete_sound(s->id());
        throw;
    }
    return s;
}

SoundPtr AssetManager::new_sound_with_alias_from_file(const std::string& alias, const unicode& path, GarbageCollectMethod garbage_collect) {
    auto s = new_sound_from_file(path, garbage_collect);
    try {
        sound_manager_.store_alias(alias, s->id());
    } catch(...) {
        delete_sound(s->id());
        throw;
    }
    return s;
}

SoundPtr AssetManager::get_sound_with_alias(const std::string &alias) {
    return sound_manager_.get_id_from_alias(alias).fetch();
}

SoundPtr AssetManager::sound(SoundID s) {
    if(parent_ && !has_sound(s)) {
        return parent_->sound(s);
    }

    return sound_manager_.get(s);
}

const SoundPtr AssetManager::sound(SoundID s) const {
    if(parent_ && !has_sound(s)) {
        return parent_->sound(s);
    }

    return sound_manager_.get(s);
}

uint32_t AssetManager::sound_count() const {
    return sound_manager_.count();
}

bool AssetManager::has_sound(SoundID s) const {
    return sound_manager_.contains(s);
}

void AssetManager::delete_sound(SoundID t) {
    sound_manager_.set_garbage_collection_method(t, GARBAGE_COLLECT_PERIODIC);
}

TextureID AssetManager::default_texture_id() const {
    if(base_manager() != this) {
        return base_manager()->default_texture_id();
    } else {
        return default_texture_id_;
    }
}

FontID AssetManager::default_font_id(DefaultFontStyle style) const {
    if(base_manager() != this) {
        return base_manager()->default_font_id(style);
    } else {
        switch(style) {
        case DEFAULT_FONT_STYLE_HEADING:
            return default_heading_font_->id();
        default:
            return default_body_font_->id();
        }
    }
}

MaterialID AssetManager::default_material_id() const {
    if(base_manager() != this) {
        return base_manager()->default_material_id();
    } else {
        return default_material_id_;
    }
}

unicode AssetManager::default_material_filename() const {
    return window->vfs->locate_file(Material::BuiltIns::DEFAULT);
}

// ========== FONTS ======================

FontPtr AssetManager::new_font_from_file(const unicode& filename, GarbageCollectMethod garbage_collect) {
    auto font = font_manager_.make(this).fetch();
    auto font_id = font->id();
    font_manager_.set_garbage_collection_method(font_id, GARBAGE_COLLECT_NEVER);

    try {
        LoaderOptions options;
        window->loader_for(filename)->into(font.get(), options);
        font_manager_.set_garbage_collection_method(font_id, garbage_collect, true);
    } catch (...) {
        // Make sure we don't leave the font hanging around
        delete_font(font_id);
        throw;
    }

    return font;
}

FontPtr AssetManager::new_font_with_alias_from_file(const std::string& alias, const unicode& filename, GarbageCollectMethod garbage_collect) {
    auto fid = new_font_from_file(filename, garbage_collect);
    try {
        font_manager_.store_alias(alias, fid);
    } catch(...) {
        delete_font(fid);
        throw;
    }
    return fid;
}

FontPtr AssetManager::new_font_from_ttf(const unicode& filename, uint32_t font_size, CharacterSet charset, GarbageCollectMethod garbage_collect) {
    auto font = font_manager_.make(this).fetch();
    auto font_id = font->id();

    font_manager_.set_garbage_collection_method(font_id, GARBAGE_COLLECT_NEVER);

    try {
        LoaderOptions options;
        options["size"] = font_size;
        options["charset"] = charset;
        window->loader_for(filename)->into(font.get(), options);

        font_manager_.set_garbage_collection_method(font_id, garbage_collect, true);
    } catch (...) {
        // Make sure we don't leave the font hanging around
        delete_font(font_id);
        throw;
    }

    return font;
}

FontPtr AssetManager::new_font_with_alias_from_ttf(const std::string& alias, const unicode& filename, uint32_t font_size, CharacterSet charset, GarbageCollectMethod garbage_collect) {
    auto fid = new_font_from_ttf(filename, font_size, charset, garbage_collect);
    try {
        font_manager_.store_alias(alias, fid);
    } catch(...) {
        delete_font(fid);
        throw;
    }
    return fid;
}

FontPtr AssetManager::get_font_with_alias(const std::string& alias) {
    return font_manager_.get_id_from_alias(alias).fetch();
}

void AssetManager::delete_font(FontID f) {
    font_manager_.set_garbage_collection_method(f, GARBAGE_COLLECT_PERIODIC);
}

FontPtr AssetManager::font(FontID f) {
    if(parent_ && !font_manager_.contains(f)) {
        return parent_->font_manager_.get(f);
    }

    return font_manager_.get(f);
}

const FontPtr AssetManager::font(FontID f) const {
    if(parent_ && !font_manager_.contains(f)) {
        return parent_->font_manager_.get(f);
    }
    return font_manager_.get(f);
}

uint32_t AssetManager::font_count() const {
    return font_manager_.count();
}

bool AssetManager::has_font(FontID f) const {
    return font_manager_.contains(f);
}

}
