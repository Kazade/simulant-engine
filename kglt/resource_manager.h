#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <map>

#include "generic/refcount_manager.h"
#include "generic/data_carrier.h"
#include "texture.h"
#include "shader.h"
#include "mesh.h"
#include "material.h"
#include "sound.h"

namespace kglt {

class ResourceManager;

typedef generic::RefCountedTemplatedManager<ResourceManager, Mesh, MeshID> MeshManager;
typedef generic::RefCountedTemplatedManager<ResourceManager, ShaderProgram, ShaderID> ShaderManager;
typedef generic::RefCountedTemplatedManager<ResourceManager, Material, MaterialID> MaterialManager;
typedef generic::RefCountedTemplatedManager<ResourceManager, Texture, TextureID> TextureManager;
typedef generic::RefCountedTemplatedManager<ResourceManager, Sound, SoundID> SoundManager;

class ResourceManager:
    public MeshManager,
    public ShaderManager,
    public MaterialManager,
    public TextureManager,
    public SoundManager {

public:
    ResourceManager(WindowBase* window, ResourceManager* parent=nullptr):
        window_(window),
        parent_(parent) {
        ShaderManager::signal_post_create().connect(sigc::mem_fun(this, &ResourceManager::post_create_shader_callback));
    }

    MeshID new_mesh();
    MeshID new_mesh_from_file(const unicode& path);

    MeshRef mesh(MeshID m);
    const MeshRef mesh(MeshID m) const;

    bool has_mesh(MeshID m) const;
    uint32_t mesh_count() const;

    TextureID new_texture();
    TextureID new_texture_from_file(const unicode& path);
    TextureRef texture(TextureID t);
    bool has_texture(TextureID t) const;

    ShaderID new_shader();
    ShaderRef shader(ShaderID s);
    ShaderProgram* __shader(ShaderID s);

    bool has_shader(ShaderID s) const;

    MaterialID new_material();
    MaterialID new_material_from_file(const unicode& path);

    MaterialRef material(MaterialID material);
    bool has_material(MaterialID m) const;

    SoundID new_sound();
    SoundID new_sound_from_file(const unicode& path);
    SoundRef sound(SoundID sound);
    bool has_sound(SoundID s) const;

    std::pair<ShaderID, bool> find_shader(const std::string& name);
    void post_create_shader_callback(ShaderProgram& obj, ShaderID id) {
        shader_lookup_[obj.name()] = id;
    }

    ResourceManager& parent() {
        assert(parent_);
        return *parent_;
    }

    WindowBase& window() { assert(window_); return *window_; }
    Scene& scene();

    void update_materials(double dt) {
        /*
          Update all animated materials
        */
        MaterialManager::apply_func_to_objects(std::bind(&Material::update, std::placeholders::_1, dt));
        /*for(std::pair<MaterialID, Material::ptr> p: MaterialManager::objects_) {
            p.second->update(dt);
        }*/
    }

    generic::DataCarrier& data() { return data_carrier_; }

    void inherit(Resource* resource);

private:
    WindowBase* window_;
    ResourceManager* parent_;

    std::map<std::string, ShaderID> shader_lookup_;

    generic::DataCarrier data_carrier_;
};


}

#endif // RESOURCE_MANAGER_H
