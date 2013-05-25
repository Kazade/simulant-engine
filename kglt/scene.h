#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <sigc++/sigc++.h>
#include <stdexcept>
#include <map>

#include "types.h"
#include "resource_manager.h"
#include "generic/manager.h"

#include "scene_base.h"

namespace kglt {

class WindowBase;
class UI;

class Scene;
class SubScene;
class GeomFactory;

typedef generic::TemplatedManager<Scene, SubScene, SubSceneID> SubSceneManager;

class Scene:
    public SceneBase,
    public Loadable,
    public SubSceneManager,
    public Managed<Scene> {

public:
    Scene(WindowBase* window);
    ~Scene();

    SubSceneID new_subscene(AvailablePartitioner partitioner=PARTITIONER_OCTREE);            
    SubScene& subscene() { return subscene(default_subscene_); }
    SubScene& subscene(SubSceneID s);
    void delete_subscene(SubSceneID s);
    uint32_t subscene_count() const;

    bool init();
    void render();
    void update(double dt);

    MaterialID default_material_id() const { return default_material_; }
    TextureID default_texture_id() const { return default_texture_; }

    Pipeline& pipeline() { return *pipeline_; }
    GeomFactory& geom_factory() { return *geom_factory_; }

private:
    SubSceneID default_subscene_;
    TextureID default_texture_;
    MaterialID default_material_;

    void initialize_defaults();

    std::shared_ptr<Pipeline> pipeline_;
    std::shared_ptr<GeomFactory> geom_factory_;
};

}

#endif // SCENE_H_INCLUDED
