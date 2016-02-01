#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <map>

#include "generic/refcount_manager.h"
#include "generic/protected_ptr.h"

#include "texture.h"
#include "mesh.h"
#include "material.h"
#include "sound.h"

namespace kglt {

class ResourceManagerImpl;

typedef generic::RefCountedTemplatedManager<ResourceManagerImpl, Mesh, MeshID> MeshManager;
typedef generic::RefCountedTemplatedManager<ResourceManagerImpl, Material, MaterialID> MaterialManager;
typedef generic::RefCountedTemplatedManager<ResourceManagerImpl, Texture, TextureID> TextureManager;
typedef generic::RefCountedTemplatedManager<ResourceManagerImpl, Sound, SoundID> SoundManager;

class ResourceManager {
public:
    virtual ~ResourceManager() {}

    //Mesh functions
    virtual MeshID new_mesh(bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_from_file(const unicode& path, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_from_tmx_file(const unicode& tmx_file, const unicode& layer_name, float tile_render_size=1.0, bool garbage_collect=true) = 0;

    virtual MeshID new_mesh_from_heightmap(
        const unicode& image_file, float spacing=1.0, float min_height=-64,
        float max_height=64.0, const HeightmapDiffuseGenerator& generator=HeightmapDiffuseGenerator(),
        bool garbage_collect=true
    ) = 0;

    virtual MeshID new_mesh_as_cube(float width, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_as_box(float width, float height, float depth, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_as_sphere(float diameter, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_as_rectangle(float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_as_cylinder(float diameter, float length, int segments = 20, int stacks = 20, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_as_capsule(float diameter, float length, int segments=20, int stacks=20, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_from_vertices(const std::vector<kglt::Vec2>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_from_vertices(const std::vector<kglt::Vec3>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool garbage_collect=true) = 0;

    virtual MeshID new_mesh_with_alias(const unicode& alias, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_with_alias_from_file(const unicode& alias, const unicode &path, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_with_alias_as_cube(const unicode& alias, float width, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_with_alias_as_sphere(const unicode& alias, float diameter, bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_with_alias_as_rectangle(const unicode& alias, float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), bool garbage_collect=true) = 0;
    virtual MeshID new_mesh_with_alias_as_cylinder(const unicode& alias, float diameter, float length, int segments = 20, int stacks = 20, bool garbage_collect=true) = 0;
    virtual MeshID get_mesh_with_alias(const unicode& alias) = 0;

    virtual void delete_mesh(MeshID m) = 0;

    //FIXME: More factory methods

    virtual ProtectedPtr<Mesh> mesh(MeshID m) = 0;
    virtual const ProtectedPtr<Mesh> mesh(MeshID m) const = 0;

    virtual bool has_mesh(MeshID m) const = 0;
    virtual uint32_t mesh_count() const = 0;


    //Texture functions
    virtual TextureID new_texture(bool garbage_collect=true) = 0;
    virtual TextureID new_texture_from_file(const unicode& path, TextureFlags flags=0, bool garbage_collect=true) = 0;
    virtual TextureID new_texture_with_alias(const unicode& alias, bool garbage_collect=true) = 0;
    virtual TextureID new_texture_with_alias_from_file(const unicode& alias, const unicode& path, TextureFlags flags=0, bool garbage_collect=true) = 0;
    virtual TextureID get_texture_with_alias(const unicode& alias) = 0;

    virtual ProtectedPtr<Texture> texture(TextureID t) = 0;
    virtual const ProtectedPtr<Texture> texture(TextureID t) const = 0;

    virtual bool has_texture(TextureID m) const = 0;
    virtual uint32_t texture_count() const = 0;
    virtual void mark_texture_as_uncollected(TextureID t) = 0;
    virtual void delete_texture(TextureID t) = 0;

    //Sound functions
    virtual SoundID new_sound(bool garbage_collect=true) = 0;
    virtual SoundID new_sound_from_file(const unicode& path, bool garbage_collect=true) = 0;

    virtual SoundID new_sound_with_alias(const unicode& alias, bool garbage_collect=true) = 0;
    virtual SoundID new_sound_with_alias_from_file(const unicode& alias, const unicode& path, bool garbage_collect=true) = 0;
    virtual SoundID get_sound_with_alias(const unicode& alias) = 0;

    virtual ProtectedPtr<Sound> sound(SoundID t) = 0;
    virtual const ProtectedPtr<Sound> sound(SoundID t) const = 0;

    virtual bool has_sound(SoundID m) const = 0;
    virtual uint32_t sound_count() const = 0;

    virtual void delete_sound(SoundID t) = 0;

    //Material functions
    virtual MaterialID new_material(bool garbage_collect=true) = 0;
    virtual MaterialID new_material_from_file(const unicode& path, bool garbage_collect=true) = 0;
    virtual MaterialID new_material_with_alias(const unicode& alias, bool garbage_collect=true) = 0;
    virtual MaterialID new_material_with_alias_from_file(const unicode& alias, const unicode& path, bool garbage_collect=true) = 0;
    virtual MaterialID get_material_with_alias(const unicode& alias) = 0;
    virtual MaterialID new_material_from_texture(TextureID texture, bool garbage_collect=true) = 0;

    virtual ProtectedPtr<Material> material(MaterialID t) = 0;
    virtual const ProtectedPtr<Material> material(MaterialID t) const = 0;

    virtual bool has_material(MaterialID m) const = 0;
    virtual uint32_t material_count() const = 0;
    virtual void mark_material_as_uncollected(MaterialID m) = 0;
    virtual void delete_material(MaterialID m) = 0;

    virtual WindowBase& window() = 0;
    virtual const WindowBase& window() const = 0;

    virtual MaterialID clone_default_material(bool garbage_collect=true) = 0;

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

    MeshID new_mesh(bool garbage_collect=true) override;
    MeshID new_mesh_from_file(const unicode& path, bool garbage_collect=true) override;
    MeshID new_mesh_from_tmx_file(const unicode& tmx_file, const unicode& layer_name, float tile_render_size=1.0, bool garbage_collect=true) override;
    MeshID new_mesh_from_heightmap(
        const unicode& image_file, float spacing=1.0, float min_height=-64,
        float max_height=64.0, const HeightmapDiffuseGenerator& generator=HeightmapDiffuseGenerator(),
        bool garbage_collect=true
    ) override;
    MeshID new_mesh_as_cube(float width, bool garbage_collect=true) override;
    MeshID new_mesh_as_box(float width, float height, float depth, bool garbage_collect=true) override;
    MeshID new_mesh_as_sphere(float diameter, bool garbage_collect=true) override;
    MeshID new_mesh_as_rectangle(float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), bool garbage_collect=true) override;
    MeshID new_mesh_as_cylinder(float diameter, float length, int segments = 20, int stacks = 20, bool garbage_collect=true);
    MeshID new_mesh_as_capsule(float diameter, float length, int segments=20, int stacks=20, bool garbage_collect=true) override;
    MeshID new_mesh_from_vertices(const std::vector<kglt::Vec2>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool garbage_collect=true) override;
    MeshID new_mesh_from_vertices(const std::vector<kglt::Vec3>& vertices, MeshArrangement arrangement=MESH_ARRANGEMENT_TRIANGLES, bool garbage_collect=true) override;

    MeshID new_mesh_with_alias(const unicode& alias, bool garbage_collect=true) override;
    MeshID new_mesh_with_alias_from_file(const unicode& alias, const unicode &path, bool garbage_collect=true) override;
    MeshID new_mesh_with_alias_as_cube(const unicode& alias, float width, bool garbage_collect=true) override;
    MeshID new_mesh_with_alias_as_sphere(const unicode& alias, float diameter, bool garbage_collect=true) override;
    MeshID new_mesh_with_alias_as_rectangle(const unicode &name, float width, float height, const Vec2& offset=Vec2(), MaterialID material=MaterialID(), bool garbage_collect=true) override;
    MeshID new_mesh_with_alias_as_cylinder(const unicode& name, float diameter, float length, int segments = 20, int stacks = 20, bool garbage_collect=true) override;
    MeshID get_mesh_with_alias(const unicode& alias) override;

    ProtectedPtr<Mesh> mesh(MeshID m);
    const ProtectedPtr<Mesh> mesh(MeshID m) const;

    bool has_mesh(MeshID m) const;
    uint32_t mesh_count() const;
    void delete_mesh(MeshID m) override;


    TextureID new_texture(bool garbage_collect=true) override;
    TextureID new_texture_from_file(const unicode& path, TextureFlags flags=0, bool garbage_collect=true) override;

    TextureID new_texture_with_alias(const unicode& alias, bool garbage_collect=true) override;
    TextureID new_texture_with_alias_from_file(const unicode& alias, const unicode& path, TextureFlags flags=0, bool garbage_collect=true) override;
    TextureID get_texture_with_alias(const unicode& alias) override;

    ProtectedPtr<Texture> texture(TextureID t);
    const ProtectedPtr<Texture> texture(TextureID t) const;
    bool has_texture(TextureID t) const;
    uint32_t texture_count() const;
    void mark_texture_as_uncollected(TextureID t) override;
    void delete_texture(TextureID t) override;

    MaterialID new_material(bool garbage_collect=true) override;
    MaterialID new_material_from_file(const unicode& path, bool garbage_collect=true) override;
    MaterialID new_material_with_alias(const unicode& alias, bool garbage_collect=true) override;
    MaterialID new_material_with_alias_from_file(const unicode& alias, const unicode& path, bool garbage_collect=true) override;
    MaterialID new_material_from_texture(TextureID texture, bool garbage_collect=true) override;
    MaterialID get_material_with_alias(const unicode& alias) override;    

    MaterialID clone_material(MaterialID mat);

    ProtectedPtr<Material> material(MaterialID material);
    const ProtectedPtr<Material> material(MaterialID material) const;
    bool has_material(MaterialID m) const;
    uint32_t material_count() const;
    void mark_material_as_uncollected(MaterialID t) override;
    void delete_material(MaterialID m) override;

    SoundID new_sound(bool garbage_collect=true);
    SoundID new_sound_from_file(const unicode& path, bool garbage_collect=true);

    SoundID new_sound_with_alias(const unicode& alias, bool garbage_collect=true);
    SoundID new_sound_with_alias_from_file(const unicode& alias, const unicode& path, bool garbage_collect=true);
    SoundID get_sound_with_alias(const unicode& alias);

    void delete_sound(SoundID t);

    ProtectedPtr<Sound> sound(SoundID sound);
    const ProtectedPtr<Sound> sound(SoundID sound) const;
    bool has_sound(SoundID s) const;
    uint32_t sound_count() const;

    WindowBase& window() { assert(window_); return *window_; }
    const WindowBase& window() const { return *window_; }

    void update();

    unicode default_material_filename() const;

    MaterialID clone_default_material(bool garbage_collect=true) {
        return material(default_material_id_)->new_clone(garbage_collect);
    }

    MaterialID default_material_id() const;
    TextureID default_texture_id() const;

private:
    WindowBase* window_;

    MaterialID default_material_id_;
    TextureID default_texture_id_;
};


}

#endif // RESOURCE_MANAGER_H
