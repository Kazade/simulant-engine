#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <map>

#include "generic/refcount_manager.h"
#include "generic/protected_ptr.h"
#include "managers/window_holder.h"
#include "loaders/heightmap_loader.h"

#include "texture.h"
#include "mesh.h"
#include "material.h"
#include "sound.h"

namespace kglt {

class ResourceManagerImpl;

typedef generic::RefCountedTemplatedManager<Mesh, MeshID> MeshManager;
typedef generic::RefCountedTemplatedManager<Material, MaterialID> MaterialManager;
typedef generic::RefCountedTemplatedManager<Texture, TextureID> TextureManager;
typedef generic::RefCountedTemplatedManager<Sound, SoundID> SoundManager;

class ResourceManager : public virtual WindowHolder {
public:
    ResourceManager(WindowBase* window):
        WindowHolder(window) {}

    virtual ~ResourceManager() {}

    //Mesh functions
    virtual MeshID new_mesh(VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_from_file(const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_from_tmx_file(const unicode& tmx_file, const unicode& layer_name, float tile_render_size=1.0, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;

    virtual MeshID new_mesh_from_heightmap(
        const unicode& image_file, const HeightmapSpecification& spec=HeightmapSpecification(),
        GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC
    ) = 0;

    virtual MeshID new_mesh_as_cube(float width, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_as_box(float width, float height, float depth, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_as_sphere(float diameter, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_as_rectangle(float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_as_cylinder(float diameter, float length, int segments = 20, int stacks = 20, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_as_capsule(float diameter, float length, int segments=20, int stacks=20, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_from_vertices(VertexSpecification vertex_specification, const std::vector<kglt::Vec2>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_from_vertices(VertexSpecification vertex_specification, const std::vector<kglt::Vec3>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;

    virtual MeshID new_mesh_with_alias(const unicode& alias, VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_with_alias_from_file(const unicode& alias, const unicode &path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_with_alias_as_cube(const unicode& alias, float width, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_with_alias_as_sphere(const unicode& alias, float diameter, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_with_alias_as_rectangle(const unicode& alias, float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID new_mesh_with_alias_as_cylinder(const unicode& alias, float diameter, float length, int segments = 20, int stacks = 20, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MeshID get_mesh_with_alias(const unicode& alias) = 0;

    virtual void delete_mesh(MeshID m) = 0;

    //FIXME: More factory methods

    virtual ProtectedPtr<Mesh> mesh(MeshID m) = 0;
    virtual const ProtectedPtr<Mesh> mesh(MeshID m) const = 0;

    virtual bool has_mesh(MeshID m) const = 0;
    virtual uint32_t mesh_count() const = 0;


    //Texture functions
    virtual TextureID new_texture(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual TextureID new_texture_from_file(const unicode& path, TextureFlags flags=0, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual TextureID new_texture_with_alias(const unicode& alias, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual TextureID new_texture_with_alias_from_file(const unicode& alias, const unicode& path, TextureFlags flags=0, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual TextureID get_texture_with_alias(const unicode& alias) = 0;

    virtual TexturePtr texture(TextureID t) = 0;
    virtual const TexturePtr texture(TextureID t) const = 0;

    virtual bool has_texture(TextureID m) const = 0;
    virtual uint32_t texture_count() const = 0;
    virtual void mark_texture_as_uncollected(TextureID t) = 0;
    virtual void delete_texture(TextureID t) = 0;

    //Sound functions
    virtual SoundID new_sound(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual SoundID new_sound_from_file(const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;

    virtual SoundID new_sound_with_alias(const unicode& alias, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual SoundID new_sound_with_alias_from_file(const unicode& alias, const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual SoundID get_sound_with_alias(const unicode& alias) = 0;

    virtual ProtectedPtr<Sound> sound(SoundID t) = 0;
    virtual const ProtectedPtr<Sound> sound(SoundID t) const = 0;

    virtual bool has_sound(SoundID m) const = 0;
    virtual uint32_t sound_count() const = 0;

    virtual void delete_sound(SoundID t) = 0;

    //Material functions
    virtual MaterialID new_material(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MaterialID new_material_from_file(const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MaterialID new_material_with_alias(const unicode& alias, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MaterialID new_material_with_alias_from_file(const unicode& alias, const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;
    virtual MaterialID get_material_with_alias(const unicode& alias) = 0;
    virtual MaterialID new_material_from_texture(TextureID texture, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;

    virtual MaterialPtr material(MaterialID t) = 0;
    virtual const MaterialPtr material(MaterialID t) const = 0;

    virtual bool has_material(MaterialID m) const = 0;
    virtual uint32_t material_count() const = 0;
    virtual void mark_material_as_uncollected(MaterialID m) = 0;
    virtual void delete_material(MaterialID m) = 0;

    virtual MaterialID clone_default_material(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) = 0;

    virtual TextureID default_texture_id() const = 0;
    virtual MaterialID default_material_id() const = 0;

    virtual unicode default_material_filename() const = 0;

};

class ResourceManagerImpl:
    public virtual ResourceManager,
    public MeshManager,
    public MaterialManager,
    public TextureManager,
    public SoundManager,
    public Managed<ResourceManagerImpl> {

public:
    ResourceManagerImpl(WindowBase* window);

    bool init() override;

    MeshID new_mesh(VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_from_file(const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_from_tmx_file(const unicode& tmx_file, const unicode& layer_name, float tile_render_size=1.0, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_from_heightmap(const unicode& image_file, const HeightmapSpecification &spec=HeightmapSpecification(),
        GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC
    ) override;
    MeshID new_mesh_as_cube(float width, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_as_box(float width, float height, float depth, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_as_sphere(float diameter, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_as_rectangle(float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_as_cylinder(float diameter, float length, int segments = 20, int stacks = 20, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_as_capsule(float diameter, float length, int segments=20, int stacks=20, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_from_vertices(VertexSpecification vertex_specification, const std::vector<kglt::Vec2>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_from_vertices(VertexSpecification vertex_specification, const std::vector<kglt::Vec3>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;

    MeshID new_mesh_with_alias(const unicode& alias, VertexSpecification vertex_specification, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_with_alias_from_file(const unicode& alias, const unicode &path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_with_alias_as_cube(const unicode& alias, float width, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_with_alias_as_sphere(const unicode& alias, float diameter, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_with_alias_as_rectangle(const unicode &name, float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID new_mesh_with_alias_as_cylinder(const unicode& name, float diameter, float length, int segments = 20, int stacks = 20, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MeshID get_mesh_with_alias(const unicode& alias) override;

    ProtectedPtr<Mesh> mesh(MeshID m) override;
    const ProtectedPtr<Mesh> mesh(MeshID m) const override;

    bool has_mesh(MeshID m) const override;
    uint32_t mesh_count() const override;
    void delete_mesh(MeshID m) override;


    TextureID new_texture(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    TextureID new_texture_from_file(const unicode& path, TextureFlags flags=0, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;

    TextureID new_texture_with_alias(const unicode& alias, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    TextureID new_texture_with_alias_from_file(const unicode& alias, const unicode& path, TextureFlags flags=0, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    TextureID get_texture_with_alias(const unicode& alias) override;

    TexturePtr texture(TextureID t) override;
    const TexturePtr texture(TextureID t) const override;
    bool has_texture(TextureID t) const override;
    uint32_t texture_count() const override;
    void mark_texture_as_uncollected(TextureID t) override;
    void delete_texture(TextureID t) override;

    MaterialID new_material(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MaterialID new_material_from_file(const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MaterialID new_material_with_alias(const unicode& alias, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MaterialID new_material_with_alias_from_file(const unicode& alias, const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MaterialID new_material_from_texture(TextureID texture, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    MaterialID get_material_with_alias(const unicode& alias) override;    

    MaterialPtr material(MaterialID material) override;
    const MaterialPtr material(MaterialID material) const override;
    bool has_material(MaterialID m) const override;
    uint32_t material_count() const override;
    void mark_material_as_uncollected(MaterialID t) override;
    void delete_material(MaterialID m) override;

    SoundID new_sound(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    SoundID new_sound_from_file(const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;

    SoundID new_sound_with_alias(const unicode& alias, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    SoundID new_sound_with_alias_from_file(const unicode& alias, const unicode& path, GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override;
    SoundID get_sound_with_alias(const unicode& alias) override;

    void delete_sound(SoundID t) override;

    ProtectedPtr<Sound> sound(SoundID sound) override;
    const ProtectedPtr<Sound> sound(SoundID sound) const override;
    bool has_sound(SoundID s) const override;
    uint32_t sound_count() const override;

    void update(double dt);

    unicode default_material_filename() const override;

    MaterialID clone_default_material(GarbageCollectMethod garbage_collect=GARBAGE_COLLECT_PERIODIC) override {
        return material(default_material_id_)->new_clone(garbage_collect);
    }

    MaterialID default_material_id() const override;
    TextureID default_texture_id() const override;

private:
    MaterialID default_material_id_;
    TextureID default_texture_id_;
};


}

#endif // RESOURCE_MANAGER_H
