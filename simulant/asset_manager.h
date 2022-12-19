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
#include "assets/material.h"
#include "sound.h"
#include "font.h"
#include "assets/particle_script.h"
#include "path.h"
#include "assets/binary_data.h"

namespace smlt {

class AssetManager;

typedef ObjectManager<MeshID, Mesh, DO_REFCOUNT> MeshManager;
typedef ObjectManager<MaterialID, Material, DO_REFCOUNT> MaterialManager;
typedef ObjectManager<TextureID, Texture, DO_REFCOUNT> TextureManager;
typedef ObjectManager<SoundID, Sound, DO_REFCOUNT> SoundManager;
typedef ObjectManager<FontID, Font, DO_REFCOUNT> FontManager;
typedef ObjectManager<ParticleScriptID, ParticleScript, DO_REFCOUNT> ParticleScriptManager;
typedef ObjectManager<BinaryID, Binary, DO_REFCOUNT> BinaryManager;

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

struct FontFlags {
    uint16_t size = 0;
    FontWeight weight = FONT_WEIGHT_NORMAL;
    FontStyle style = FONT_STYLE_NORMAL;
    CharacterSet charset = CHARACTER_SET_LATIN;

    /* If non-zero, this will apply a blur to the font texture
     * before upload. Useful for drop shadows */
    std::size_t blur_radius = 0;
};

struct SoundFlags {
    bool stream_audio = true;
};

/* Majority of the API definitions have been generated using this Python code:
 *
 * TEMPLATE="""
    // %(klass)s API
    %(klass)sPtr new_%(name)s_from_file(const Path& filename, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    void destroy_%(name)s(%(klass)sID id);
    %(klass)sPtr %(name)s(%(klass)sID id);
    const %(klass)sPtr %(name)s (%(klass)sID id) const;
    std::size_t %(name)s_count() const;
    bool has_%(name)s(%(klass)sID id) const;
    %(klass)sPtr find_%(name)s(const std::string& name);
"""

for klass, name in (
    ("ParticleScript", "particle_script"),
    ("Texture", "texture"),
    ("Mesh", "mesh"),
    ("Material", "material"),
    ("Sound", "sound"),
    ("Font", "font"),
):
    print(TEMPLATE % {"klass": klass, "name": name})
*/

class AssetManager {
public:
    AssetManager(AssetManager* parent=nullptr);
    virtual ~AssetManager();

    // Generated API

    /* ParticleScript API */
    ParticleScriptPtr new_particle_script_from_file(const Path &filename, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    void destroy_particle_script(ParticleScriptID id);
    ParticleScriptPtr particle_script(ParticleScriptID id);
    const ParticleScriptPtr particle_script (ParticleScriptID id) const;
    std::size_t particle_script_count() const;
    bool has_particle_script(ParticleScriptID id) const;
    ParticleScriptPtr find_particle_script(const std::string& name);

    /* Texture API */
    TexturePtr new_texture_from_file(const Path& filename, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    void destroy_texture(TextureID id);
    TexturePtr texture(TextureID id);
    const TexturePtr texture (TextureID id) const;
    std::size_t texture_count() const;
    bool has_texture(TextureID id) const;
    TexturePtr find_texture(const std::string& alias);


    /* Mesh API */
    void destroy_mesh(MeshID id);
    MeshPtr mesh(MeshID id);
    const MeshPtr mesh (MeshID id) const;
    std::size_t mesh_count() const;
    bool has_mesh(MeshID id) const;
    MeshPtr find_mesh(const std::string& name);


    /* Material API */
    MaterialPtr new_material_from_file(const Path &filename, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    void destroy_material(const MaterialID& id);
    MaterialPtr material(const MaterialID& id);
    const MaterialPtr material (const MaterialID& id) const;
    std::size_t material_count() const;
    bool has_material(const MaterialID& id) const;
    MaterialPtr find_material(const std::string& name);


    /* Sound API */
    SoundPtr new_sound_from_file(
        const Path& filename,
        const SoundFlags& flags=SoundFlags(),
        GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC
    );
    void destroy_sound(SoundID id);
    SoundPtr sound(SoundID id);
    const SoundPtr sound (SoundID id) const;
    std::size_t sound_count() const;
    bool has_sound(SoundID id) const;
    SoundPtr find_sound(const std::string& name);

    /* Load raw binary data from a file */
    BinaryPtr new_binary_from_file(const Path& filename, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    BinaryPtr binary(BinaryID id) const;
    std::size_t binary_count() const;
    bool has_binary(BinaryID id) const;
    BinaryPtr find_binary(const std::string& name);
    void destroy_binary(BinaryID id);

    /* Font API */
    /** Loads a font by searching asset paths for a match.
     * Searches for standard variations of the filename depending on the family,
     * weight, style, and size. We look for the following (example) variations:
     *
     * - Kanit-Regular.ttf
     * - Kanit-RegularItalic.ttf
     * - Kanit-BlackItalic.ttf
     * - Kanit-BlackItalic-18.fnt
     *
     * and in the following example paths:
     *
     * $path/$filename
     * $path/fonts/$filename
     * $path/fonts/family/$filename
     */
    FontPtr new_font_from_family(const std::string& family, const FontFlags& flags, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    FontPtr new_font_from_file(const Path &filename, const FontFlags& flags=FontFlags(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    void destroy_font(FontID id);
    FontPtr font(FontID id);
    const FontPtr font (FontID id) const;
    std::size_t font_count() const;
    bool has_font(FontID id) const;
    FontPtr find_font(const std::string& alias);

    // Customisations
    TexturePtr new_texture(uint16_t width, uint16_t height, TextureFormat format=TEXTURE_FORMAT_RGBA_4UB_8888, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    TexturePtr new_texture_from_file(const Path& path, TextureFlags flags, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    MaterialPtr new_material(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    MeshPtr new_mesh(VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshPtr new_mesh(VertexDataPtr vertex_data, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshPtr new_mesh_from_file(const Path& path, const VertexSpecification& desired_specification=VertexSpecification::DEFAULT, const MeshLoadOptions& options=MeshLoadOptions(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    /*
     * Given a submesh, this creates a new mesh with just that single submesh
     */
    MeshPtr new_mesh_from_submesh(SubMesh* submesh, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    MeshPtr new_mesh_from_heightmap(const Path& image_file, const HeightmapSpecification &spec=HeightmapSpecification(),
        GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC
    );

    MeshPtr new_mesh_from_heightmap(
        const TextureID& texture,
        const HeightmapSpecification& spec=HeightmapSpecification(),
        GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC
    );

    MeshPtr new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<smlt::Vec2>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshPtr new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<smlt::Vec3>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshPtr new_mesh_as_cube_with_submesh_per_face(float width, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MaterialPtr new_material_from_texture(TextureID texture, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    void update(float dt);

    virtual MaterialPtr default_material() const;

    MaterialPtr clone_material(const MaterialID& mat_id, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MaterialPtr clone_default_material(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    AssetManager* base_manager() const;

    void destroy_all();
    void run_garbage_collection();

    bool is_base_manager() const;

    std::size_t child_manager_count() const {
        return children_.size();
    }

    const AssetManager* child_manager(std::size_t i) const {
        return children_.at(i);
    }

private:
    AssetManager* parent_ = nullptr;

    TextureManager texture_manager_;
    MaterialManager material_manager_;
    FontManager font_manager_;
    MeshManager mesh_manager_;
    SoundManager sound_manager_;
    ParticleScriptManager particle_script_manager_;
    BinaryManager binary_manager_;

    thread::Mutex template_material_lock_;
    std::unordered_map<Path, MaterialID> template_materials_;
    std::set<MaterialID> materials_loading_;

    MaterialPtr get_template_material(const Path &path);

    std::vector<AssetManager*> children_;
    void register_child(AssetManager* child) {
        children_.push_back(child);
    }

    void unregister_child(AssetManager* child) {
        children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
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
        } else if(auto p = dynamic_cast<const Binary*>(resource)) {
            binary_manager_.set_garbage_collection_method(p->id(), method);
        } else {
            S_ERROR("Unhandled asset type. GC method not set");
        }
    }

    friend class Asset;
};

class LocalAssetManager:
    public AssetManager,
    public RefCounted<LocalAssetManager> {

public:
    LocalAssetManager(AssetManager* parent=nullptr):
        AssetManager(parent) {}

    bool init() { return true; }
    void clean_up() {}

};

class SharedAssetManager:
    public AssetManager,
    public RefCounted<SharedAssetManager> {
public:
    SharedAssetManager() = default;

    bool init();

    void cleanup() {}

    virtual MaterialPtr default_material() const;

    void set_default_material_filename(const Path &filename);
    Path default_material_filename() const;

private:
    mutable MaterialPtr default_material_;
    Path default_material_filename_;

    mutable FontPtr default_body_font_;
    Path default_body_font_filename_;

    mutable FontPtr default_heading_font_;
    Path default_heading_font_filename_;

    /*
     * Default textures for materials, these are all
     * 8x8 textures in white, black, and 0.5,0.5,1 (for normal maps)
     */
    TexturePtr white_tex_;
    TexturePtr black_tex_;
    TexturePtr z_tex_;
};


#undef ASSET_METHOD_DEFINITIONS

}

