#ifndef STAGE_H
#define STAGE_H

#include <functional>

#include "generic/managed.h"
#include "generic/manager.h"
#include "object.h"
#include "types.h"
#include "resource_manager.h"
#include "scene.h"

namespace kglt {

class Partitioner;
class Scene;
class Debug;

typedef generic::TemplatedManager<Stage, Actor, ActorID> ActorManager;
typedef generic::TemplatedManager<Stage, Light, LightID> LightManager;
typedef generic::TemplatedManager<Stage, CameraProxy, CameraID> CameraProxyManager;

class Stage:
    public Managed<Stage>,
    public generic::Identifiable<StageID>,
    public ResourceManager,
    public Object,
    public ActorManager,
    public LightManager,
    public CameraProxyManager,
    public Loadable {

public:
    Stage(Scene *parent, StageID id, AvailablePartitioner partitioner);

    ActorID new_actor();
    ActorID new_actor(bool make_responsive, bool make_collidable);
    ActorID new_actor(MeshID mid); //FIXME: Deprecate
    ActorID new_actor(MeshID mid, bool make_responsive, bool make_collidable); //FIXME: deprecate

    ActorID new_actor_with_mesh(MeshID mid) { return new_actor(mid); }

    ActorID new_actor_with_parent(ActorID parent);
    ActorID new_actor_with_parent(ActorID parent, MeshID mid);

    ProtectedPtr<Actor> actor(ActorID e);
    const ProtectedPtr<Actor> actor(ActorID e) const;

    bool has_actor(ActorID e) const;
    void delete_actor(ActorID e);
    uint32_t actor_count() const { return ActorManager::manager_count(); }

    LightID new_light(LightType type=LIGHT_TYPE_POINT);
    LightID new_light(Object& parent, LightType type=LIGHT_TYPE_POINT);
    ProtectedPtr<Light> light(LightID light);
    void delete_light(LightID light_id);
    uint32_t light_count() const { return LightManager::manager_count(); }

    void host_camera(CameraID c=CameraID()); ///< Create a representation (CameraProxy) of the designated camera
    void evict_camera(CameraID c=CameraID()); ///< Remove the representation of the camera
    ProtectedPtr<CameraProxy> camera(CameraID c=CameraID());

    kglt::Colour ambient_light() const { return ambient_light_; }
    void set_ambient_light(const kglt::Colour& c) { ambient_light_ = c; }

    sig::signal<void (ActorID)>& signal_actor_created() { return signal_actor_created_; }
    sig::signal<void (ActorID)>& signal_actor_destroyed() { return signal_actor_destroyed_; }

    sig::signal<void (LightID)>& signal_light_created() { return signal_light_created_; }
    sig::signal<void (LightID)>& signal_light_destroyed() { return signal_light_destroyed_; }

    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the stage");
    }

    template<typename T, typename ID>
    void post_create_callback(T& obj, ID id) {
        obj.set_parent(this);
        obj._initialize();
    }

    Partitioner& partitioner() { return *partitioner_; }

    void destroy();

    /*
     *  ResourceManager interface follows
     *  Pass-thrus to the parent
     */

    //Mesh functions
    virtual MeshID new_mesh(bool garbage_collect=true) override { return scene().new_mesh(garbage_collect); }

    virtual MeshID new_mesh_from_file(const unicode& path, bool garbage_collect=true) override {
        return scene().new_mesh_from_file(path, garbage_collect);
    }

    virtual MeshID new_mesh_as_cube(float width, bool garbage_collect=true) {
        return scene().new_mesh_as_cube(width, garbage_collect);
    }

    virtual MeshID new_mesh_as_sphere(float diameter, bool garbage_collect=true) {
        return scene().new_mesh_as_sphere(diameter, garbage_collect);
    }

    virtual ProtectedPtr<Mesh> mesh(MeshID m) { return scene().mesh(m); }
    virtual const ProtectedPtr<Mesh> mesh(MeshID m) const { return scene().mesh(m); }

    virtual bool has_mesh(MeshID m) const { return scene().has_mesh(m); }
    virtual uint32_t mesh_count() const { return scene().mesh_count(); }


    //Texture functions
    virtual TextureID new_texture(bool garbage_collect=true) override { return scene().new_texture(garbage_collect); }
    virtual TextureID new_texture_from_file(const unicode& path, bool garbage_collect=true) {
        return scene().new_texture_from_file(path, garbage_collect);
    }
    virtual TextureID new_texture_with_name(const unicode& name, bool garbage_collect=true) override {
        return scene().new_texture_with_name(name, garbage_collect);
    }
    virtual TextureID new_texture_with_name_from_file(const unicode& name, const unicode& path, bool garbage_collect=true) override {
        return scene().new_texture_with_name_from_file(name, path, garbage_collect);
    }
    virtual TextureID get_texture_with_name(const unicode& name) override {
        return scene().get_texture_with_name(name);
    }
    virtual void delete_texture(TextureID t) override {
        scene().delete_texture(t);
    }

    virtual ProtectedPtr<Texture> texture(TextureID t) { return scene().texture(t); }
    virtual const ProtectedPtr<Texture> texture(TextureID t) const { return scene().texture(t); }

    virtual bool has_texture(TextureID t) const { return scene().has_texture(t); }
    virtual uint32_t texture_count() const { return scene().texture_count(); }
    virtual void mark_texture_as_uncollected(TextureID t) { scene().mark_texture_as_uncollected(t); }

    //Shader functions
    virtual ShaderID new_shader(bool garbage_collect=true) {
        return scene().new_shader(garbage_collect);
    }

    virtual ShaderID new_shader_from_files(const unicode &vert_shader, const unicode &frag_shader, bool garbage_collect=true) {
        return scene().new_shader_from_files(vert_shader, frag_shader, garbage_collect);
    }

    virtual ShaderRef shader(ShaderID s) { return scene().shader(s); }
    virtual const ShaderRef shader(ShaderID s) const { return scene().shader(s); }

    virtual bool has_shader(ShaderID s) const { return scene().has_shader(s); }
    virtual uint32_t shader_count() const { return scene().shader_count(); }


    //Sound functions
    virtual SoundID new_sound(bool garbage_collect=true) override { return scene().new_sound(garbage_collect); }
    virtual SoundID new_sound_from_file(const unicode& path, bool garbage_collect=true) override {
        return scene().new_sound_from_file(path, garbage_collect);
    }

    virtual SoundID new_sound_with_name(const unicode& name, bool garbage_collect=true) override {
        return scene().new_sound_with_name(name, garbage_collect);
    }

    virtual SoundID new_sound_with_name_from_file(const unicode& name, const unicode& path, bool garbage_collect=true) override {
        return scene().new_sound_with_name_from_file(name, path, garbage_collect);
    }

    virtual SoundID get_sound_with_name(const unicode& name) {
        return scene().get_sound_with_name(name);
    }

    virtual void delete_sound(SoundID t) { scene().delete_sound(t); }

    virtual ProtectedPtr<Sound> sound(SoundID s) { return scene().sound(s); }
    virtual const ProtectedPtr<Sound> sound(SoundID s) const { return scene().sound(s); }

    virtual bool has_sound(SoundID s) const { return scene().has_sound(s); }
    virtual uint32_t sound_count() const { return scene().sound_count(); }


    //Material functions
    virtual MaterialID new_material(bool garbage_collect=true) { return scene().new_material(garbage_collect); }
    virtual MaterialID new_material_from_file(const unicode& path, bool garbage_collect=true) {
        return scene().new_material_from_file(path, garbage_collect);
    }

    virtual ProtectedPtr<Material> material(MaterialID m) { return scene().material(m); }
    virtual const ProtectedPtr<Material> material(MaterialID m) const { return scene().material(m); }

    virtual bool has_material(MaterialID m) const { return scene().has_material(m); }
    virtual uint32_t material_count() const { return scene().material_count(); }
    virtual void mark_material_as_uncollected(MaterialID t) { scene().mark_material_as_uncollected(t); }

    virtual WindowBase& window() { return scene().window(); }
    const WindowBase& window() const { return scene().window(); }

    virtual Scene& scene() { return scene_; }
    virtual const Scene& scene() const { return scene_; }

    GeomFactory& geom_factory() { return *geom_factory_; }

    Debug& debug() { assert(debug_); return *debug_; }

    bool init();
private:
    Scene& scene_;

    kglt::Colour ambient_light_;

    sig::signal<void (ActorID)> signal_actor_created_;
    sig::signal<void (ActorID)> signal_actor_destroyed_;

    sig::signal<void (LightID)> signal_light_created_;
    sig::signal<void (LightID)> signal_light_destroyed_;

    std::shared_ptr<Partitioner> partitioner_;

    void set_partitioner(AvailablePartitioner partitioner);

    std::shared_ptr<GeomFactory> geom_factory_;
    std::shared_ptr<Debug> debug_;

    friend class Scene;

    CameraID new_camera_proxy(CameraID cam);
    void delete_camera_proxy(CameraID cam);
};


}
#endif // SUBSCENE_H
