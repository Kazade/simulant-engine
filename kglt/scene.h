#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <sigc++/sigc++.h>
#include <boost/any.hpp>

#include <stdexcept>
#include <map>

#include "kglt/kazbase/list_utils.h"

#include "types.h"
#include "object.h"
#include "resource_manager.h"

#include "generic/visitor.h"
#include "generic/manager.h"

namespace kglt {

class WindowBase;
class UI;

class Scene;
class SubScene;

typedef generic::TemplatedManager<SubScene, Entity, EntityID> EntityManager;
typedef generic::TemplatedManager<SubScene, Light, LightID> LightManager;
typedef generic::TemplatedManager<SubScene, Camera, CameraID> CameraManager;

class SceneBase:
    public ResourceManager {
public:
    SceneBase(WindowBase* window, ResourceManager* parent=nullptr):
        ResourceManager(window, parent) {}

    virtual ~SceneBase() {}
};

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

    EntityID new_entity(MeshID mid=MeshID());
    EntityID new_entity(Object& parent, MeshID mid=MeshID());

    Entity& entity(EntityID e);
    bool has_entity(EntityID e) const;
    void delete_entity(EntityID e);

    LightID new_light(LightType type=LIGHT_TYPE_POINT);
    LightID new_light(Object& parent, LightType type=LIGHT_TYPE_POINT);
    Light& light(LightID light);
    void delete_light(LightID light_id);

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

private:
    Scene& scene_;

    kglt::Colour ambient_light_;

    sigc::signal<void, EntityID> signal_entity_created_;
    sigc::signal<void, EntityID> signal_entity_destroyed_;

    sigc::signal<void, LightID> signal_light_created_;
    sigc::signal<void, LightID> signal_light_destroyed_;

    std::tr1::shared_ptr<Partitioner> partitioner_;

    void set_partitioner(std::tr1::shared_ptr<Partitioner> partitioner);

    void do_update(double dt) override {
        update_materials(dt);
    }

    friend class Scene;
};

typedef generic::TemplatedManager<Scene, SubScene, SubSceneID> SubSceneManager;

class Scene:
    public SceneBase,
    public Loadable,
    public SubSceneManager,
    public Managed<Scene>,
    public LuaClass<Scene> {

public:
    Scene(WindowBase* window);
    ~Scene();

    SubSceneID new_subscene(AvailablePartitioner partitioner=PARTITIONER_OCTREE);
    SubScene& subscene(SubSceneID s = DefaultSubSceneID);
    void delete_subscene(SubSceneID s);

    bool init();
    void render();
    void update(double dt);

    //DEPRECATED
    MaterialID default_material() const __attribute__((deprecated("Use default_material_id()"))) { return default_material_; }

    MaterialID default_material_id() const { return default_material_; }

    Pipeline& pipeline() { return *pipeline_; }

    static void do_lua_export(lua_State &state);
private:
    SubSceneID default_subscene_;
    TextureID default_texture_;
    MaterialID default_material_;

    void initialize_defaults();

    std::tr1::shared_ptr<Pipeline> pipeline_;
};

}

#endif // SCENE_H_INCLUDED
