#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <map>

#include "generic/manager.h"
#include "generic/data_carrier.h"
#include "texture.h"
#include "shader.h"
#include "mesh.h"
#include "material.h"
#include "sound.h"

namespace kglt {

class ResourceManager;

typedef generic::TemplatedManager<ResourceManager, Mesh, MeshID> MeshManager;
typedef generic::TemplatedManager<ResourceManager, ShaderProgram, ShaderID> ShaderManager;
typedef generic::TemplatedManager<ResourceManager, Material, MaterialID> MaterialManager;
typedef generic::TemplatedManager<ResourceManager, Texture, TextureID> TextureManager;
typedef generic::TemplatedManager<ResourceManager, Sound, SoundID> SoundManager;

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
    Mesh& mesh(MeshID m);
    bool has_mesh(MeshID m) const;
    const Mesh& mesh(MeshID m) const;
    void delete_mesh(MeshID mid);

    TextureID new_texture();
    Texture& texture(TextureID t);
    bool has_texture(TextureID t) const;
    void delete_texture(TextureID tid);

    ShaderID new_shader();
    ShaderProgram& shader(ShaderID s);
    bool has_shader(ShaderID s) const;
    void delete_shader(ShaderID s);

    MaterialID new_material(MaterialID clone_from=MaterialID());
    Material& material(MaterialID material);
    bool has_material(MaterialID m) const;
    void delete_material(MaterialID m);

    SoundID new_sound();
    Sound& sound(SoundID sound);
    bool has_sound(SoundID s) const;
    void delete_sound(SoundID s);

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
        for(std::pair<MaterialID, Material::ptr> p: MaterialManager::objects_) {
            p.second->update(dt);
        }
    }

    generic::DataCarrier& data() { return data_carrier_; }

private:
    WindowBase* window_;
    ResourceManager* parent_;

    std::map<std::string, ShaderID> shader_lookup_;

    generic::DataCarrier data_carrier_;
};


}

#endif // RESOURCE_MANAGER_H
