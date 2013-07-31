#ifndef STAGE_H
#define STAGE_H

#include "generic/managed.h"
#include "generic/manager.h"
#include "object.h"
#include "types.h"
#include "resource_manager.h"
#include "scene.h"

namespace kglt {

class Partitioner;
class Scene;

typedef generic::TemplatedManager<Stage, Actor, ActorID> ActorManager;
typedef generic::TemplatedManager<Stage, Light, LightID> LightManager;

class Stage:
    public Managed<Stage>,
    public generic::Identifiable<StageID>,
    public ResourceManager,
    public Object,
    public ActorManager,
    public LightManager,
    public Loadable {

public:
    Stage(Scene *parent, StageID id);

    ActorID new_actor();
    ActorID new_actor(bool make_responsive);
    ActorID new_actor(MeshID mid);
    ActorID new_actor(MeshID mid, bool make_responsive);

    ActorID new_actor_with_parent(Actor& parent);
    ActorID new_actor_with_parent(Actor& parent, MeshID mid);

    Actor& actor(ActorID e);
    const Actor& actor(ActorID e) const;

    ActorRef actor_ref(ActorID e);

    bool has_actor(ActorID e) const;
    void delete_actor(ActorID e);
    uint32_t actor_count() const { return ActorManager::manager_count(); }

    LightID new_light(LightType type=LIGHT_TYPE_POINT);
    LightID new_light(Object& parent, LightType type=LIGHT_TYPE_POINT);
    Light& light(LightID light);
    void delete_light(LightID light_id);
    uint32_t light_count() const { return LightManager::manager_count(); }

    kglt::Colour ambient_light() const { return ambient_light_; }
    void set_ambient_light(const kglt::Colour& c) { ambient_light_ = c; }

    sigc::signal<void, ActorID>& signal_actor_created() { return signal_actor_created_; }
    sigc::signal<void, ActorID>& signal_actor_destroyed() { return signal_actor_destroyed_; }

    sigc::signal<void, LightID>& signal_light_created() { return signal_light_created_; }
    sigc::signal<void, LightID>& signal_light_destroyed() { return signal_light_destroyed_; }

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
    virtual MeshID new_mesh() { return scene().new_mesh(); }
    virtual MeshID new_mesh_from_file(const unicode& path) { return scene().new_mesh_from_file(path); }

    virtual MeshRef mesh(MeshID m) { return scene().mesh(m); }
    virtual const MeshRef mesh(MeshID m) const { return scene().mesh(m); }

    virtual bool has_mesh(MeshID m) const { return scene().has_mesh(m); }
    virtual uint32_t mesh_count() const { return scene().mesh_count(); }


    //Texture functions
    virtual TextureID new_texture() { return scene().new_texture(); }
    virtual TextureID new_texture_from_file(const unicode& path) { return scene().new_texture_from_file(path); }

    virtual ProtectedPtr<Texture> texture(TextureID t) { return scene().texture(t); }
    virtual const ProtectedPtr<Texture> texture(TextureID t) const { return scene().texture(t); }

    virtual bool has_texture(TextureID t) const { return scene().has_texture(t); }
    virtual uint32_t texture_count() const { return scene().texture_count(); }
    virtual void mark_texture_as_uncollected(TextureID t) { scene().mark_texture_as_uncollected(t); }

    //Shader functions
    virtual ShaderID new_shader() { return scene().new_shader(); }

    virtual ShaderRef shader(ShaderID s) { return scene().shader(s); }
    virtual const ShaderRef shader(ShaderID s) const { return scene().shader(s); }

    virtual bool has_shader(ShaderID s) const { return scene().has_shader(s); }
    virtual uint32_t shader_count() const { return scene().shader_count(); }


    //Sound functions
    virtual SoundID new_sound() { return scene().new_sound(); }
    virtual SoundID new_sound_from_file(const unicode& path) { return scene().new_sound_from_file(path); }

    virtual SoundRef sound(SoundID s) { return scene().sound(s); }
    virtual const SoundRef sound(SoundID s) const { return scene().sound(s); }

    virtual bool has_sound(SoundID s) const { return scene().has_sound(s); }
    virtual uint32_t sound_count() const { return scene().sound_count(); }


    //Material functions
    virtual MaterialID new_material() { return scene().new_material(); }
    virtual MaterialID new_material_from_file(const unicode& path) { return scene().new_material_from_file(path); }

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
private:
    Scene& scene_;

    kglt::Colour ambient_light_;

    sigc::signal<void, ActorID> signal_actor_created_;
    sigc::signal<void, ActorID> signal_actor_destroyed_;

    sigc::signal<void, LightID> signal_light_created_;
    sigc::signal<void, LightID> signal_light_destroyed_;

    std::shared_ptr<Partitioner> partitioner_;

    void set_partitioner(std::shared_ptr<Partitioner> partitioner);

    std::shared_ptr<GeomFactory> geom_factory_;

    friend class Scene;
};


}
#endif // SUBSCENE_H
