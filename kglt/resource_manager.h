#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <map>

#include "generic/refcount_manager.h"
#include "managers/window_holder.h"
#include "loaders/heightmap_loader.h"

#include "texture.h"
#include "mesh.h"
#include "material.h"
#include "sound.h"

namespace kglt {

class ResourceManager;

typedef generic::RefCountedTemplatedManager<Mesh, MeshID> MeshManager;
typedef generic::RefCountedTemplatedManager<Material, MaterialID> MaterialManager;
typedef generic::RefCountedTemplatedManager<Texture, TextureID> TextureManager;
typedef generic::RefCountedTemplatedManager<Sound, SoundID> SoundManager;

struct TextureFlags {
    TextureFlags(
        MipmapGenerate mipmap = MIPMAP_GENERATE_COMPLETE,
        TextureWrap wrap = TEXTURE_WRAP_REPEAT,
        TextureFilter filter = TEXTURE_FILTER_NEAREST):
        mipmap(mipmap),
        wrap(wrap),
        filter(filter) {

    }

    MipmapGenerate mipmap = MIPMAP_GENERATE_COMPLETE;
    TextureWrap wrap = TEXTURE_WRAP_REPEAT;
    TextureFilter filter = TEXTURE_FILTER_NEAREST;
    bool flip_vertically = false;
};


class ResourceManager:
    public virtual WindowHolder,
    public MeshManager,
    public MaterialManager,
    public TextureManager,
    public SoundManager,
    public Managed<ResourceManager> {

public:
    ResourceManager(WindowBase* window, ResourceManager* parent=nullptr);

    bool init();

    MeshID new_mesh(VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_from_file(const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    /*
     * Given a submesh, this creates a new mesh with just that single submesh
     */
    MeshID new_mesh_from_submesh(SubMesh* submesh, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    MeshID new_mesh_from_tmx_file(const unicode& tmx_file, const unicode& layer_name, float tile_render_size=1.0, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_from_heightmap(const unicode& image_file, const HeightmapSpecification &spec=HeightmapSpecification(),
        GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC
    );
    MeshID new_mesh_as_cube(float width, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_as_box(float width, float height, float depth, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_as_sphere(float diameter, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_as_rectangle(float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_as_cylinder(float diameter, float length, int segments = 20, int stacks = 20, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_as_capsule(float diameter, float length, int segments=20, int stacks=20, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<kglt::Vec2>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_from_vertices(VertexSpecification vertex_specification, const std::string& submesh_name, const std::vector<kglt::Vec3>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    MeshID new_mesh_with_alias(const std::string &alias, VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_with_alias_from_file(const std::string& alias, const unicode &path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_with_alias_as_cube(const std::string &alias, float width, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_with_alias_as_sphere(const std::string& alias, float diameter, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_with_alias_as_rectangle(const std::string &name, float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID new_mesh_with_alias_as_cylinder(const std::string& name, float diameter, float length, int segments = 20, int stacks = 20, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MeshID get_mesh_with_alias(const std::string& alias);

    MeshPtr mesh(MeshID m);
    const MeshPtr mesh(MeshID m) const;

    bool has_mesh(MeshID m) const;
    uint32_t mesh_count() const;
    void delete_mesh(MeshID m);


    TextureID new_texture(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    TextureID new_texture_from_file(const unicode& path, TextureFlags flags=TextureFlags(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    TextureID new_texture_with_alias(const std::string &alias, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    TextureID new_texture_with_alias_from_file(const std::string& alias, const unicode& path, TextureFlags flags=TextureFlags(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    TextureID get_texture_with_alias(const std::string &alias);

    TexturePtr texture(TextureID t);
    const TexturePtr texture(TextureID t) const;
    bool has_texture(TextureID t) const;
    uint32_t texture_count() const;
    void mark_texture_as_uncollected(TextureID t);
    void delete_texture(TextureID t);

    MaterialID new_material(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MaterialID new_material_from_file(const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MaterialID new_material_with_alias(const std::string &alias, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MaterialID new_material_with_alias_from_file(const std::string& alias, const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MaterialID new_material_from_texture(TextureID texture, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    MaterialID get_material_with_alias(const std::string &alias);

    MaterialPtr material(MaterialID material);
    const MaterialPtr material(MaterialID material) const;
    bool has_material(MaterialID m) const;
    uint32_t material_count() const;
    void mark_material_as_uncollected(MaterialID t);
    void delete_material(MaterialID m);

    SoundID new_sound(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    SoundID new_sound_from_file(const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);

    SoundID new_sound_with_alias(const std::string& alias, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    SoundID new_sound_with_alias_from_file(const std::string &alias, const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC);
    SoundID get_sound_with_alias(const std::string& alias);

    void delete_sound(SoundID t);

    SoundPtr sound(SoundID sound);
    const SoundPtr sound(SoundID sound) const;
    bool has_sound(SoundID s) const;
    uint32_t sound_count() const;

    void update(double dt);

    unicode default_material_filename() const;

    MaterialID clone_default_material(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) {
        return base_manager()->material(base_manager()->default_material_id())->new_clone(this, garbage_collect);
    }

    MaterialID default_material_id() const;
    TextureID default_texture_id() const;

    ResourceManager* base_manager() const {
        // Constness applies to the resource manager itself, not the returned base manager
        ResourceManager* ret = const_cast<ResourceManager*>(this);
        while(ret->parent_) {
            ret = ret->parent_;
        }
        return ret;
    }

private:
    ResourceManager* parent_ = nullptr;

    MaterialID default_material_id_;
    TextureID default_texture_id_;

    std::mutex template_material_lock_;
    std::unordered_map<unicode, MaterialID> template_materials_;
    std::set<MaterialID> materials_loading_;

    MaterialID get_template_material(const unicode& path);
};


}

#endif // RESOURCE_MANAGER_H
