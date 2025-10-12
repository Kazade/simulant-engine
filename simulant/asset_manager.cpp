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

#include "asset_manager.h"
#include "application.h"
#include "assets/prefab.h"
#include "generic/lru_cache.h"
#include "loader.h"
#include "loaders/heightmap_loader.h"
#include "procedural/mesh.h"
#include "utils/gl_thread_check.h"
#include "utils/simple_memstream.h"
#include "vfs.h"
#include <iterator>

/** FIXME
 *
 * - Write tests to show that all load_X methods mark resources as uncollected before returning
 * - Think of better GC logic, perhaps collect on every frame?
 */

namespace smlt {

AssetManager::AssetManager(AssetManager* parent):
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

            for(auto& child: children_) {
                child->parent_ = nullptr;
            }
        }

        S_DEBUG("Destroyed base manager: {0}", this);
    }
}

void AssetManager::destroy_all() {
    mesh_manager_.destroy_all();
    material_manager_.destroy_all();
    texture_manager_.destroy_all();
    sound_manager_.destroy_all();
    font_manager_.destroy_all();
    particle_script_manager_.destroy_all();
    binary_manager_.destroy_all();
    run_garbage_collection();
}

void AssetManager::update(float dt) {
    _S_UNUSED(dt);
}

void SharedAssetManager::set_default_material_filename(const Path& filename) {
    default_material_filename_ = filename;
    default_material_.reset();
}

Path SharedAssetManager::default_material_filename() const {
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


static TexturePtr create_texture_with_color(AssetManager* manager, const Color& c) {
    auto tex = manager->create_texture(8, 8, TEXTURE_FORMAT_RGB_3UB_888, GARBAGE_COLLECT_NEVER);

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


bool SharedAssetManager::on_init() {
    S_DEBUG("Initializing default materials, textures, and fonts (AssetManager: {0})", this);
    set_default_material_filename(Material::BuiltIns::DEFAULT);

    white_tex_ = create_texture_with_color(this, smlt::Color::white());
    white_tex_->set_name("s_white_texture");

    black_tex_ = create_texture_with_color(this, smlt::Color::black());
    black_tex_->set_name("s_black_texture");

    z_tex_ = create_texture_with_color(this, smlt::Color::from_hex_string("#8080FF"));
    z_tex_->set_name("s_znormal_texture");

    S_DEBUG("Finished initializing defaults");
    return true;
}

MaterialPtr SharedAssetManager::default_material() const {
    assert(is_base_manager());

    if(!default_material_) {
        default_material_ = const_cast<SharedAssetManager*>(this)->load_material(
            default_material_filename_
        );
    }

    return default_material_;
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
    binary_manager_.update();
}

bool AssetManager::is_base_manager() const {
    return !parent_;
}


#define DOCONCAT(x, y) x##y
#define CONCAT(x, y) DOCONCAT(x, y)

template<typename T, typename M, typename... Args>
auto new_x(T self, M& manager, GarbageCollectMethod garbage_collect,
           Args... args) -> auto {
    auto result = manager.make(self, args...);
    manager.set_garbage_collection_method(result->id(), garbage_collect);
    return result;
}

template<typename T, typename M>
auto new_x(T self, M& manager, GarbageCollectMethod garbage_collect) -> auto {
    auto result = manager.make(self);
    manager.set_garbage_collection_method(result->id(), garbage_collect);
    return result;
}

#define GET_X(klass, name, manager_name) \
    if(parent_ && !CONCAT(has_, name)(id)) { \
        return parent_-> name (id); \
    } \
    return manager_name.get(id)

MeshPtr AssetManager::create_mesh(VertexSpecification vertex_specification,
                                  GarbageCollectMethod garbage_collect) {
    return new_x(this, mesh_manager_, garbage_collect, vertex_specification);
}

MeshPtr AssetManager::create_mesh(VertexDataPtr vertex_data, GarbageCollectMethod garbage_collect) {
    return new_x(this, mesh_manager_, garbage_collect, vertex_data);
}

MeshPtr AssetManager::mesh(AssetID id) {
    GET_X(Mesh, mesh, mesh_manager_);
}

const MeshPtr AssetManager::mesh(AssetID id) const {
    GET_X(Mesh, mesh, mesh_manager_);
}

MeshPtr AssetManager::create_mesh_from_submesh(SubMesh* submesh, GarbageCollectMethod garbage_collect) {
    if(submesh->type() == SUBMESH_TYPE_RANGED) {
        /* FIXME: Implement this! */
        S_ERROR("Ranged submeshes can't currently be turned into meshes");
        return MeshPtr();
    }

    auto source_vdata = submesh->mesh->vertex_data.get();

    VertexSpecification spec = source_vdata->vertex_specification();

    auto mesh = create_mesh(spec, garbage_collect);
    auto target_vdata = mesh->vertex_data.get();

    SubMesh* target = mesh->create_submesh(
        submesh->name(),
        submesh->material(),
        submesh->index_data->index_type(),
        submesh->arrangement()
    );

    std::unordered_map<Index, Index> old_to_new;

    for(uint32_t i = 0; i < submesh->index_data->count(); ++i) {
        auto old_index = submesh->index_data->at(i);

        if(old_to_new.count(old_index)) {
            target->index_data->index(old_to_new[old_index]);
        } else {
            auto j = source_vdata->copy_vertex_to_another(
                *target_vdata, submesh->index_data->at(i)
            );

            old_to_new[old_index] = j;
            target->index_data->index(j);
        }
    }

    target_vdata->done();
    target->index_data->done();

    return mesh;
}

MeshPtr AssetManager::load_mesh(const Path& path,
    const VertexSpecification& desired_specification,
    const MeshLoadOptions& options,
    GarbageCollectMethod garbage_collect) {

    //Load the material
    auto mesh = create_mesh(desired_specification, garbage_collect);
    auto loader = get_app()->loader_for(path);
    assert(loader && "Unable to locate a loader for the specified mesh file");

    if(!loader) {
        S_ERROR("No mesh loader found for path: {0}", path.str());
        return MeshPtr();
    }

    LoaderOptions loader_options;
    loader_options[MESH_LOAD_OPTIONS_KEY] = options;

    if(!loader->into(mesh, loader_options)) {
        return nullptr;
    }

    return mesh;
}

MeshPtr AssetManager::create_mesh_from_heightmap(const Path& image_file, const HeightmapSpecification& spec, GarbageCollectMethod garbage_collect) {
    auto loader = get_app()->loader_for("heightmap_loader", image_file);

    if(!loader) {
        return nullptr;
    }

    auto mesh = create_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);

    loader->into(mesh, {
        { "spec", spec},
    });

    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect);

    return mesh;
}

MeshPtr AssetManager::create_mesh_from_heightmap(const TexturePtr& texture, const HeightmapSpecification& spec, GarbageCollectMethod garbage_collect) {
    auto mesh = create_mesh(VertexSpecification::DEFAULT, GARBAGE_COLLECT_NEVER);

    loaders::HeightmapLoader loader(texture);

    if(!loader.into(*mesh, {
                               {"spec", spec},
    })) {
        return nullptr;
    }
    mesh_manager_.set_garbage_collection_method(mesh->id(), garbage_collect);

    return mesh;
}

MeshPtr AssetManager::create_mesh_as_cube_with_submesh_per_face(float width, GarbageCollectMethod garbage_collect) {
    auto m = create_mesh(
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

void AssetManager::destroy_mesh(AssetID m) {
    mesh_manager_.set_garbage_collection_method(m, GARBAGE_COLLECT_PERIODIC);
}

bool AssetManager::has_mesh(AssetID m) const {
    return mesh_manager_.contains(m);
}

std::size_t AssetManager::mesh_count() const {
    return mesh_manager_.count();
}

MaterialPtr AssetManager::create_material(GarbageCollectMethod garbage_collect) {
    return new_x(this, material_manager_, garbage_collect);
}

void AssetManager::destroy_material(const AssetID& m) {
    material_manager_.set_garbage_collection_method(m, GARBAGE_COLLECT_PERIODIC);
}

MaterialPtr AssetManager::load_material(const Path& path, GarbageCollectMethod garbage_collect) {
    auto new_mat = create_material();
    auto loader = get_app()->loader_for(path);
    if(!loader) {
        S_ERROR("Unable to find loader for {0}", path);
        return MaterialPtr();
    }

    if(!loader->into(new_mat)) {
        return nullptr;
    }
    material_manager_.set_garbage_collection_method(new_mat->id(), garbage_collect);
    return new_mat;
}

MaterialPtr AssetManager::create_material_from_texture(TexturePtr texture, GarbageCollectMethod garbage_collect) {
    auto m = load_material(Material::BuiltIns::TEXTURE_ONLY, GARBAGE_COLLECT_NEVER);
    assert(m);

    m->set_base_color_map(texture);

    material_manager_.set_garbage_collection_method(m->id(), garbage_collect);
    return m;
}

MaterialPtr AssetManager::find_material(const std::string& name) {
    return material_manager_.find_object(name);
}

MaterialPtr AssetManager::material(const AssetID& id) {
    GET_X(Material, material, material_manager_);
}

const MaterialPtr AssetManager::material(const AssetID& id) const {
    GET_X(Material, material, material_manager_);
}

bool AssetManager::has_material(const AssetID& m) const {
    return material_manager_.contains(m);
}

std::size_t AssetManager::material_count() const {
    return material_manager_.count();
}

TexturePtr AssetManager::create_texture(uint16_t width, uint16_t height, TextureFormat format, GarbageCollectMethod garbage_collect) {
    return new_x(this, texture_manager_, garbage_collect, width, height,
                 format);
}

TexturePtr AssetManager::load_texture(const Path& path, GarbageCollectMethod garbage_collect) {
    return load_texture(path, TextureFlags(), garbage_collect);
}

TexturePtr AssetManager::load_texture(const Path& path, TextureFlags flags, GarbageCollectMethod garbage_collect) {
    //Load the texture
    S_DEBUG("Loading texture from file: {0}", path);
    smlt::TexturePtr tex = create_texture(8, 8, TEXTURE_FORMAT_RGBA_4UB_8888, garbage_collect);

    {
        S_DEBUG("Finding loader for: {0}", path);
        auto loader = get_app()->loader_for(path, LOADER_HINT_TEXTURE);
        if(!loader) {
            S_WARN("Couldn't find loader for texture");
            return smlt::TexturePtr();
        }

        S_DEBUG("Loader found, loading...");
        if(!loader->into(tex)) {
            return nullptr;
        }

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

void AssetManager::destroy_texture(AssetID t) {
    texture_manager_.set_garbage_collection_method(t, GARBAGE_COLLECT_PERIODIC);
}

TexturePtr AssetManager::find_texture(const std::string& name) {
    return texture_manager_.find_object(name);
}

TexturePtr AssetManager::texture(AssetID id) {
    GET_X(Texture, texture, texture_manager_);
}

const TexturePtr AssetManager::texture(AssetID id) const {
    GET_X(Texture, texture, texture_manager_);
}

bool AssetManager::has_texture(AssetID t) const {
    return texture_manager_.contains(t);
}

std::size_t AssetManager::texture_count() const {
    return texture_manager_.count();
}

SoundPtr AssetManager::load_sound(const Path& path, const SoundFlags &flags, GarbageCollectMethod garbage_collect) {
    //Load the sound
    auto snd = sound_manager_.make(this, get_app()->sound_driver);
    sound_manager_.set_garbage_collection_method(snd->id(), garbage_collect);

    auto loader = get_app()->loader_for(path);

    LoaderOptions opts;
    opts["stream"] = flags.stream_audio;

    if(loader) {
        if(!loader->into(snd, opts)) {
            return nullptr;
        }
    } else {
        S_ERROR("Unsupported file type: ", path);
    }

    sound_manager_.set_garbage_collection_method(snd->id(), garbage_collect);

    return snd;
}

SoundPtr AssetManager::find_sound(const std::string &name) {
    return sound_manager_.find_object(name);
}

SoundPtr AssetManager::sound(AssetID id) {
    GET_X(Sound, sound, sound_manager_);
}

const SoundPtr AssetManager::sound(AssetID id) const {
    GET_X(Sound, sound, sound_manager_);
}

std::size_t AssetManager::sound_count() const {
    return sound_manager_.count();
}

bool AssetManager::has_sound(AssetID s) const {
    return sound_manager_.contains(s);
}

void AssetManager::destroy_sound(AssetID t) {
    sound_manager_.set_garbage_collection_method(t, GARBAGE_COLLECT_PERIODIC);
}

BinaryPtr AssetManager::load_binary(const Path& filename, GarbageCollectMethod garbage_collect) {

    auto stream = smlt::get_app()->vfs->read_file(filename);
    if(!stream) {
        return BinaryPtr();
    }

    std::vector<uint8_t> data;
    std::noskipws(*stream);
    std::copy(std::istream_iterator<uint8_t>(*stream), std::istream_iterator<uint8_t>(), back_inserter(data));

    stream.reset();

    auto bin = binary_manager_.make(this, std::move(data));
    binary_manager_.set_garbage_collection_method(bin->id(), garbage_collect);

    return bin;
}

BinaryPtr AssetManager::binary(AssetID id) const {
    GET_X(Binary, binary, binary_manager_);
}

std::size_t AssetManager::binary_count() const {
    return binary_manager_.count();
}

bool AssetManager::has_binary(AssetID id) const {
    return binary_manager_.contains(id);
}

BinaryPtr AssetManager::find_binary(const std::string& name) {
    return binary_manager_.find_object(name);
}

void AssetManager::destroy_binary(AssetID id) {
    binary_manager_.set_garbage_collection_method(id, GARBAGE_COLLECT_PERIODIC);
}

MaterialPtr AssetManager::clone_material(const AssetID& mat_id, GarbageCollectMethod garbage_collect) {
    assert(mat_id && "No default material, called to early?");

    auto manager = &material_manager_;
    if(!manager->contains(mat_id) && parent_) {
        manager = &parent_->material_manager_;
    }

    auto new_mat = manager->clone(mat_id);
    assert(new_mat);

    manager->set_garbage_collection_method(new_mat->id(), garbage_collect);
    return new_mat;
}

MaterialPtr AssetManager::clone_default_material(GarbageCollectMethod garbage_collect) {
    return clone_material(base_manager()->default_material()->id(), garbage_collect);
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

FontPtr AssetManager::create_font_from_memory(
    const uint8_t* data, const std::size_t size, const FontFlags& flags,
    GarbageCollectMethod garbage_collect) {

    auto font = font_manager_.make(this);
    auto font_id = font->id();

    LoaderOptions options;
    options["size"] = flags.size ? flags.size : get_app()->config->ui.font_size;
    options["weight"] = flags.weight;
    options["style"] = flags.style;
    options["charset"] = flags.charset;
    options["blur_radius"] = flags.blur_radius;

    // FIXME: We should see if it's a ttf_font or something else not just
    // assume a TTF.
    auto loadert = get_app()->loader_type("ttf_font");
    assert(loadert);

    auto stream = stream_from_memory(data, size);
    stream->seekg(0);

    if(!loadert->loader_for("", stream)->into(font.get(), options)) {
        destroy_font(font_id);
        return nullptr;
    }
    font_manager_.set_garbage_collection_method(font_id, garbage_collect);

    return font;
}

FontPtr AssetManager::create_font_from_family(const std::string& family, const FontFlags& flags, GarbageCollectMethod garbage_collect) {
    uint16_t size = flags.size ? flags.size : get_app()->config->ui.font_size;
    const std::string px_as_string = smlt::to_string(size);
    const std::string weight_string = font_weight_name(flags.weight);
    const std::string style_string = (flags.style == FONT_STYLE_NORMAL) ? "" : font_style_name(flags.style);

    std::string potentials [] = {
        family + "-" + weight_string + style_string + ".ttf",
        family + "-" + weight_string + style_string + "-" + px_as_string + ".fnt",
    };

    std::string alias = Font::generate_name(family, size, flags.weight, flags.style);

    /* This LRUCache prevents us continually searching for the font */
    static LRUCache<std::string, smlt::Path> location_cache;
    location_cache.set_max_size(8);

    optional<Path> loc = location_cache.get(alias);
    if(!loc) {
        for(auto& filename: potentials) {
            loc = get_app()->vfs->locate_file(filename, true, /*fail_silently=*/true);
            if(loc) {
                break;
            }

            /* Try a font directory prefix */
            loc = get_app()->vfs->locate_file(kfs::path::join("fonts", filename), true, /*fail_silently=*/true);
            if(loc) {
                break;
            }

            /* Finally try a family name dir within fonts */
            loc = get_app()->vfs->locate_file(
                kfs::path::join(kfs::path::join("fonts", family), filename),
                true, /*fail_silently=*/true
            );

            if(loc) {
                location_cache.insert(alias, loc.value());
                break;
            }
        }
    }

    if(loc) {
        return load_font(loc.value(), flags, garbage_collect);
    } else {
        S_WARN("Unable to locate font file with family {0} and size {1}", family, size);
        return FontPtr();
    }
}

PrefabPtr AssetManager::create_prefab(const smlt::StageNode* root,
                                      GarbageCollectMethod garbage_collect) {
    auto prefab = prefab_manager_.make(this);
    auto prefab_id = prefab->id();
    prefab_manager_.set_garbage_collection_method(prefab_id, garbage_collect);

    auto create_prefab_node = [](const smlt::StageNode* node) -> PrefabNode {
        auto pn = PrefabNode();
        pn.id = node->id();
        pn.node_type_name = node->node_type_name();
        pn.params = node->create_params();
        return pn;
    };

    if(root) {
        prefab->push_node(create_prefab_node(root));
        for(auto& node: root->each_descendent()) {
            prefab->push_node(create_prefab_node(&node), node.parent()->id());
        }
    }

    return prefab;
}

PrefabPtr AssetManager::load_prefab(const Path& filename,
                                    GarbageCollectMethod garbage_collect) {

    auto prefab = prefab_manager_.make(this);
    auto prefab_id = prefab->id();
    prefab_manager_.set_garbage_collection_method(prefab_id, garbage_collect);

    LoaderOptions opts;
    if(!get_app()->loader_for(filename)->into(prefab.get(), opts)) {
        return nullptr;
    }

    return prefab;
}

PrefabPtr AssetManager::prefab(AssetID id) {
    GET_X(Prefab, prefab, prefab_manager_);
}

const PrefabPtr AssetManager::prefab(AssetID id) const {
    GET_X(Prefab, prefab, prefab_manager_);
}

std::size_t AssetManager::prefab_count() const {
    return prefab_manager_.count();
}

bool AssetManager::has_prefab(AssetID id) const {
    return prefab_manager_.contains(id);
}

PrefabPtr AssetManager::find_prefab(const std::string& name) {
    return prefab_manager_.find_object(name);
}

void AssetManager::destroy_prefab(AssetID id) {
    prefab_manager_.set_garbage_collection_method(id, GARBAGE_COLLECT_PERIODIC);
}

FontPtr AssetManager::load_font(const Path& filename, const FontFlags& flags, GarbageCollectMethod garbage_collect) {
    auto font = font_manager_.make(this);
    auto font_id = font->id();

    LoaderOptions options;
    options["size"] = flags.size ? flags.size : get_app()->config->ui.font_size;
    options["weight"] = flags.weight;
    options["style"] = flags.style;
    options["charset"] = flags.charset;
    options["blur_radius"] = flags.blur_radius;
    if(!get_app()->loader_for(filename)->into(font.get(), options)) {
        return nullptr;
    }

    font_manager_.set_garbage_collection_method(font_id, garbage_collect);

    return font;
}

FontPtr AssetManager::find_font(const std::string& name) {
    return font_manager_.find_object(name);
}

void AssetManager::destroy_font(AssetID f) {
    font_manager_.set_garbage_collection_method(f, GARBAGE_COLLECT_PERIODIC);
}

FontPtr AssetManager::font(AssetID id) {
    GET_X(Font, font, font_manager_);
}

const FontPtr AssetManager::font(AssetID id) const {
    GET_X(Font, font, font_manager_);
}

std::size_t AssetManager::font_count() const {
    return font_manager_.count();
}

bool AssetManager::has_font(AssetID f) const {
    return font_manager_.contains(f);
}

ParticleScriptPtr AssetManager::load_particle_script(const Path& filename, GarbageCollectMethod garbage_collect) {
    auto ps = particle_script_manager_.make(this);

    LoaderOptions options;
    if(!get_app()->loader_for(filename)->into(ps.get(), options)) {
        return nullptr;
    }

    return ps;
}

ParticleScriptPtr AssetManager::find_particle_script(const std::string& name) {
    return particle_script_manager_.find_object(name);
}

void AssetManager::destroy_particle_script(AssetID id) {
    particle_script_manager_.set_garbage_collection_method(id, GARBAGE_COLLECT_PERIODIC);
}

ParticleScriptPtr AssetManager::particle_script(AssetID id) {
    GET_X(ParticleScript, particle_script, particle_script_manager_);
}

std::size_t AssetManager::particle_script_count() const {
    return particle_script_manager_.count();
}

bool AssetManager::has_particle_script(AssetID id) const {
    return particle_script_manager_.contains(id);
}

}
