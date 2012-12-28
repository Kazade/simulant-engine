#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <map>

#include "generic/manager.h"

#include "texture.h"
#include "shader.h"
#include "mesh.h"
#include "material.h"

namespace kglt {

class ResourceManager;

typedef generic::TemplatedManager<ResourceManager, Mesh, MeshID> MeshManager;
typedef generic::TemplatedManager<ResourceManager, ShaderProgram, ShaderID> ShaderManager;
typedef generic::TemplatedManager<ResourceManager, Material, MaterialID> MaterialManager;
typedef generic::TemplatedManager<ResourceManager, Texture, TextureID> TextureManager;

class ResourceManager:
    public MeshManager,
    public ShaderManager,
    public MaterialManager,
    public TextureManager {

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
    void delete_texture(TextureID tid);

    ShaderID new_shader();
    ShaderProgram& shader(ShaderID s);
    void delete_shader(ShaderID s);

    MaterialID new_material(MaterialID clone_from=MaterialID());
    Material& material(MaterialID material);
    void delete_material(MaterialID m);

    std::pair<ShaderID, bool> find_shader(const std::string& name);
    void post_create_shader_callback(ShaderProgram& obj, ShaderID id) {
        shader_lookup_[obj.name()] = id;
    }

    ResourceManager& parent() {
        assert(parent_);
        return *parent_;
    }

    WindowBase& window() { assert(window_); return *window_; }
private:
    WindowBase* window_;
    ResourceManager* parent_;

    std::map<std::string, ShaderID> shader_lookup_;
};


}

#endif // RESOURCE_MANAGER_H
