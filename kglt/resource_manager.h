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

class ResourceManagerImpl;

typedef generic::RefCountedTemplatedManager<ResourceManagerImpl, Mesh, MeshID> MeshManager;
typedef generic::RefCountedTemplatedManager<ResourceManagerImpl, ShaderProgram, ShaderID> ShaderManager;
typedef generic::RefCountedTemplatedManager<ResourceManagerImpl, Material, MaterialID> MaterialManager;
typedef generic::RefCountedTemplatedManager<ResourceManagerImpl, Texture, TextureID> TextureManager;
typedef generic::RefCountedTemplatedManager<ResourceManagerImpl, Sound, SoundID> SoundManager;

template<typename T>
struct ProtectedPtr {
    typedef std::lock_guard<std::recursive_mutex> guard_type;

    std::shared_ptr<T> __object;

    ProtectedPtr() = default;

    ProtectedPtr(std::weak_ptr<T> ref):
        __object(ref.lock()),
        lock_(__object ?
            std::make_shared<guard_type>(__object->mutex()) :
            std::shared_ptr<guard_type>()
        ) {

    }

    ~ProtectedPtr() {
        __object.reset(); //Release reference
        lock_.reset(); //Unlock
    }

    T* operator->() { return __object.get(); }
    T& operator*() { return *__object; }

    explicit operator bool() const {
        return __object.get() != nullptr;
    }

private:
    std::shared_ptr<guard_type> lock_;
};

class ResourceManager {
public:
    virtual ~ResourceManager() {}

    //Mesh functions
    virtual MeshID new_mesh() = 0;
    virtual MeshID new_mesh_from_file(const unicode& path) = 0;

    virtual MeshRef mesh(MeshID m) = 0;
    virtual const MeshRef mesh(MeshID m) const = 0;

    virtual bool has_mesh(MeshID m) const = 0;
    virtual uint32_t mesh_count() const = 0;


    //Texture functions
    virtual TextureID new_texture() = 0;
    virtual TextureID new_texture_from_file(const unicode& path) = 0;

    virtual ProtectedPtr<Texture> texture(TextureID t) = 0;
    virtual const ProtectedPtr<Texture> texture(TextureID t) const = 0;

    virtual bool has_texture(TextureID m) const = 0;
    virtual uint32_t texture_count() const = 0;


    //Shader functions
    virtual ShaderID new_shader() = 0;

    virtual ShaderRef shader(ShaderID t) = 0;
    virtual const ShaderRef shader(ShaderID t) const = 0;

    virtual bool has_shader(ShaderID m) const = 0;
    virtual uint32_t shader_count() const = 0;


    //Sound functions
    virtual SoundID new_sound() = 0;
    virtual SoundID new_sound_from_file(const unicode& path) = 0;

    virtual SoundRef sound(SoundID t) = 0;
    virtual const SoundRef sound(SoundID t) const = 0;

    virtual bool has_sound(SoundID m) const = 0;
    virtual uint32_t sound_count() const = 0;


    //Material functions
    virtual MaterialID new_material() = 0;
    virtual MaterialID new_material_from_file(const unicode& path) = 0;

    virtual ProtectedPtr<Material> material(MaterialID t) = 0;
    virtual const ProtectedPtr<Material> material(MaterialID t) const = 0;

    virtual bool has_material(MaterialID m) const = 0;
    virtual uint32_t material_count() const = 0;

    virtual WindowBase& window() = 0;
    virtual const WindowBase& window() const = 0;

    virtual Scene& scene() = 0;
    virtual const Scene& scene() const = 0;
};

class ResourceManagerImpl:
    public ResourceManager,
    public MeshManager,
    public ShaderManager,
    public MaterialManager,
    public TextureManager,
    public SoundManager {

public:
    ResourceManagerImpl(WindowBase* window);

    MeshID new_mesh();
    MeshID new_mesh_from_file(const unicode& path);

    MeshRef mesh(MeshID m);
    const MeshRef mesh(MeshID m) const;

    bool has_mesh(MeshID m) const;
    uint32_t mesh_count() const;

    TextureID new_texture();
    TextureID new_texture_from_file(const unicode& path);
    ProtectedPtr<Texture> texture(TextureID t);
    const ProtectedPtr<Texture> texture(TextureID t) const;
    bool has_texture(TextureID t) const;
    uint32_t texture_count() const;

    ShaderID new_shader();
    ShaderRef shader(ShaderID s);
    const ShaderRef shader(ShaderID s) const;
    uint32_t shader_count() const;
    ShaderProgram* __shader(ShaderID s);    

    bool has_shader(ShaderID s) const;

    MaterialID new_material();
    MaterialID new_material_from_file(const unicode& path);

    ProtectedPtr<Material> material(MaterialID material);
    const ProtectedPtr<Material> material(MaterialID material) const;
    bool has_material(MaterialID m) const;
    uint32_t material_count() const;

    SoundID new_sound();
    SoundID new_sound_from_file(const unicode& path);
    SoundRef sound(SoundID sound);
    const SoundRef sound(SoundID sound) const;
    bool has_sound(SoundID s) const;
    uint32_t sound_count() const;

    std::pair<ShaderID, bool> find_shader(const std::string& name);
    void post_create_shader_callback(ShaderProgram& obj, ShaderID id) {
        shader_lookup_[obj.name()] = id;
    }

    WindowBase& window() { assert(window_); return *window_; }
    const WindowBase& window() const { return *window_; }

    Scene& scene();
    const Scene& scene() const;

    void update();

    generic::DataCarrier& data() { return data_carrier_; }

private:
    WindowBase* window_;
    std::map<std::string, ShaderID> shader_lookup_;

    generic::DataCarrier data_carrier_;

    template<typename Func>
    void apply_func_to_materials(Func func) {
        for(auto p: MaterialManager::__objects()) {
            auto mat = material(p.first);
            std::bind(func, mat.__object)();
        }
    }
};


}

#endif // RESOURCE_MANAGER_H
