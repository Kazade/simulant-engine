/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <map>

#include "generic/object_manager.h"
#include "managers/window_holder.h"
#include "loaders/heightmap_loader.h"
#include "loader.h"
#include "texture.h"
#include "meshes/mesh.h"
#include "material.h"
#include "sound.h"
#include "font.h"
#include "assets/particle_script.h"

namespace smlt {

class AssetManager;

typedef ObjectManager<MeshID, Mesh, DO_REFCOUNT> MeshManager;
typedef ObjectManager<MaterialID, Material, DO_REFCOUNT> MaterialManager;
typedef ObjectManager<TextureID, Texture, DO_REFCOUNT> TextureManager;
typedef ObjectManager<SoundID, Sound, DO_REFCOUNT> SoundManager;
typedef ObjectManager<FontID, Font, DO_REFCOUNT> FontManager;
typedef ObjectManager<ParticleScriptID, ParticleScript, DO_REFCOUNT> ParticleScriptManager;

struct TextureFlags {
    TextureFlags(
        MipmapGenerate mipmap = MIPMAP_GENERATE_COMPLETE,
        TextureWrap wrap = TEXTURE_WRAP_REPEAT,
        TextureFilter filter = TEXTURE_FILTER_POINT):
        mipmap(mipmap),
        wrap(wrap),
        filter(filter) {

    }

    MipmapGenerate mipmap = MIPMAP_GENERATE_COMPLETE;
    TextureWrap wrap = TEXTURE_WRAP_REPEAT;
    TextureFilter filter = TEXTURE_FILTER_POINT;
    TextureFreeData free_data = TEXTURE_FREE_DATA_AFTER_UPLOAD;
    bool flip_vertically = false;
    bool auto_upload = true; // Should the texture be uploaded automatically?
};

enum DefaultFontStyle {
    DEFAULT_FONT_STYLE_HEADING,
    DEFAULT_FONT_STYLE_BODY
};

#define DOCONCAT(x, y) x##y
#define CONCAT(x, y) DOCONCAT(x, y)

/* Definitions for assets without a default VA_ARGS is required args */
#define ASSET_METHOD_DEFINITIONS(klass, name, ...) \
    CONCAT(klass, Ptr) CONCAT(CONCAT(new_, name), _from_file) (const unicode& filename, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC); \
    CONCAT(klass, Ptr) CONCAT(CONCAT(new_, name), _with_alias_from_file) (const std::string &alias, const unicode& filename, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC); \
    void CONCAT(destroy_, name) (CONCAT(klass, ID) id); \
    CONCAT(klass, Ptr) name (CONCAT(klass, ID) id); \
    const CONCAT(klass, Ptr) name (CONCAT(klass, ID) id) const; \
    std::size_t CONCAT(name, _count)() const; \
    bool CONCAT(has_, name) (CONCAT(klass, ID) id) const; \
    CONCAT(klass, Ptr) CONCAT(CONCAT(get_, name), _with_alias)(const std::string& alias)

/* Definitions for assets which can be created without a file */
#define ASSET_METHOD_DEFINITIONS_WITH_DEFAULT(klass, name, ...) \
    CONCAT(klass, Ptr) CONCAT(new_, name) (__VA_ARGS__ __VA_OPT__(,) GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC); \
    CONCAT(klass, Ptr) CONCAT(CONCAT(new_, name), _with_alias) (const std::string &alias, __VA_ARGS__ __VA_OPT__(,) GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC); \
    ASSET_METHOD_DEFINITIONS(klass, name, __VA_ARGS__)


class AssetManager:
    public virtual WindowHolder,
    public RefCounted<AssetManager> {

public:
    AssetManager(Window* window, AssetManager* parent=nullptr);
    virtual ~AssetManager();

    bool init();

    /* new_material() will clone the default material */
    ASSET_METHOD_DEFINITIONS_WITH_DEFAULT(Material, material);
    ASSET_METHOD_DEFINITIONS_WITH_DEFAULT(Mesh, mesh, VertexSpecification vertex_specification);
    ASSET_METHOD_DEFINITIONS(ParticleScript, particle_script);
    ASSET_METHOD_DEFINITIONS(Sound, sound);
    ASSET_METHOD_DEFINITIONS(Font, font);
    ASSET_METHOD_DEFINITIONS_WITH_DEFAULT(Texture, texture, uint16_t width, uint16_t height, TextureFormat format=TEXTURE_FORMAT_RGBA8888);

    MeshPtr new_mesh_from_file(const unicode& path, const MeshLoadOptions& options, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshPtr new_mesh_with_alias_from_file(const std::string &alias, const unicode& path, const MeshLoadOptions& options, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    /*
     * Given a submesh, this creates a new mesh with just that single submesh
     */
    MeshPtr new_mesh_from_submesh(SubMesh* submesh, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    MeshPtr new_mesh_from_tmx_file(const unicode& tmx_file, const unicode& layer_name, float tile_render_size=1.0, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshPtr new_mesh_from_heightmap(const unicode& image_file, const HeightmapSpecification &spec=HeightmapSpecification(),
        GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC
    );
    MeshPtr new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<smlt::Vec2>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshPtr new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<smlt::Vec3>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshPtr new_mesh_as_cube_with_submesh_per_face(float width, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    TexturePtr new_texture_from_file(const unicode& path, TextureFlags flags, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    TexturePtr new_texture_with_alias_from_file(const std::string &alias, const unicode& path, TextureFlags flags, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    MaterialPtr new_material_from_texture(TextureID texture, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    FontPtr new_font_from_ttf(const unicode& filename, uint32_t font_size, CharacterSet charset=CHARACTER_SET_LATIN, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    FontPtr new_font_with_alias_from_ttf(const std::string& alias, const unicode& filename, uint32_t font_size, CharacterSet charset=CHARACTER_SET_LATIN, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    void update(float dt);

    unicode default_material_filename() const;

    MaterialPtr clone_default_material(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    MaterialID default_material_id() const;
    TextureID default_texture_id() const;
    FontID default_font_id(DefaultFontStyle style=DEFAULT_FONT_STYLE_BODY) const;

    AssetManager* base_manager() const {
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

    void run_garbage_collection();

private:
    AssetManager* parent_ = nullptr;

    MaterialID default_material_id_;
    TextureID default_texture_id_;
    FontPtr default_body_font_;
    FontPtr default_heading_font_;

    TextureManager texture_manager_;
    MaterialManager material_manager_;
    FontManager font_manager_;
    MeshManager mesh_manager_;
    SoundManager sound_manager_;
    ParticleScriptManager particle_script_manager_;

    std::mutex template_material_lock_;
    std::unordered_map<unicode, MaterialID> template_materials_;
    std::set<MaterialID> materials_loading_;

    MaterialPtr get_template_material(const unicode& path);

    std::set<AssetManager*> children_;
    void register_child(AssetManager* child) {
        children_.insert(child);
    }

    void unregister_child(AssetManager* child) {
        children_.erase(child);
    }

    void set_garbage_collection_method(const Asset* resource, GarbageCollectMethod method) {
        // FIXME: This is ugly, but I'm struggling to think of another way to get from the Resource
        // to the right manager while keeping Resource as a non-template

        if(auto p = dynamic_cast<const Mesh*>(resource)) {
            mesh_manager_.set_garbage_collection_method(p->id(), method);
        } else if(auto p = dynamic_cast<const Material*>(resource)) {
            material_manager_.set_garbage_collection_method(p->id(), method);
        } else if(auto p = dynamic_cast<const Font*>(resource)) {
            font_manager_.set_garbage_collection_method(p->id(), method);
        } else if(auto p = dynamic_cast<const Sound*>(resource)) {
            sound_manager_.set_garbage_collection_method(p->id(), method);
        } else if(auto p = dynamic_cast<const Texture*>(resource)) {
            texture_manager_.set_garbage_collection_method(p->id(), method);
        } else if(auto p = dynamic_cast<const ParticleScript*>(resource)) {
            particle_script_manager_.set_garbage_collection_method(p->id(), method);
        } else {
            L_ERROR("Unhandled asset type. GC method not set");
        }
    }

    friend class Asset;
};

#undef ASSET_METHOD_DEFINITIONS

}

