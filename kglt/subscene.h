#ifndef SUBSCENE_H
#define SUBSCENE_H

#include "generic/manager.h"
#include "scene_base.h"
#include "object.h"
#include "types.h"

namespace kglt {

class Partitioner;
class Scene;

typedef generic::TemplatedManager<SubScene, Entity, EntityID> EntityManager;
typedef generic::TemplatedManager<SubScene, Light, LightID> LightManager;
typedef generic::TemplatedManager<SubScene, Camera, CameraID> CameraManager;

class SubScene:
    public Managed<SubScene>,
    public generic::Identifiable<SubSceneID>,
    public SceneBase,
    public Object,
    public EntityManager,
    public LightManager,
    public CameraManager,
    public Loadable {

public:
    SubScene(Scene *parent, SubSceneID id);

    EntityID new_entity();
    EntityID new_entity(MeshID mid);
    EntityID new_entity_with_parent(Entity& parent);
    EntityID new_entity_with_parent(Entity& parent, MeshID mid);

    Entity& entity(EntityID e);
    bool has_entity(EntityID e) const;
    void delete_entity(EntityID e);
    uint32_t entity_count() const { return EntityManager::manager_count(); }

    LightID new_light(LightType type=LIGHT_TYPE_POINT);
    LightID new_light(Object& parent, LightType type=LIGHT_TYPE_POINT);
    Light& light(LightID light);
    void delete_light(LightID light_id);
    uint32_t light_count() const { return LightManager::manager_count(); }

    CameraID new_camera();
    CameraID new_camera(Object& parent);
    Camera& camera(CameraID c=CameraID());
    void delete_camera(CameraID cid);

    kglt::Colour ambient_light() const { return ambient_light_; }
    void set_ambient_light(const kglt::Colour& c) { ambient_light_ = c; }

    sigc::signal<void, EntityID>& signal_entity_created() { return signal_entity_created_; }
    sigc::signal<void, EntityID>& signal_entity_destroyed() { return signal_entity_destroyed_; }

    sigc::signal<void, LightID>& signal_light_created() { return signal_light_created_; }
    sigc::signal<void, LightID>& signal_light_destroyed() { return signal_light_destroyed_; }

    void move(float x, float y, float z) {
        throw std::logic_error("You cannot move the subscene");
    }

    template<typename T, typename ID>
    void post_create_callback(T& obj, ID id) {
        obj.set_parent(this);
        obj._initialize();
    }

    Partitioner& partitioner() { return *partitioner_; }

    void destroy();

    Scene& scene() { return scene_; }
    WindowBase& window();

private:
    Scene& scene_;

    kglt::Colour ambient_light_;

    sigc::signal<void, EntityID> signal_entity_created_;
    sigc::signal<void, EntityID> signal_entity_destroyed_;

    sigc::signal<void, LightID> signal_light_created_;
    sigc::signal<void, LightID> signal_light_destroyed_;

    std::shared_ptr<Partitioner> partitioner_;

    void set_partitioner(std::shared_ptr<Partitioner> partitioner);

    void do_update(double dt) override {
        update_materials(dt);
    }

    friend class Scene;
};


}
#endif // SUBSCENE_H
