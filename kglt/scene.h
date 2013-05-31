#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <sigc++/sigc++.h>
#include <stdexcept>
#include <map>

#include "types.h"
#include "resource_manager.h"
#include "generic/manager.h"

namespace kglt {

class WindowBase;
class UI;

class Scene;
class SubScene;
class GeomFactory;

typedef generic::TemplatedManager<Scene, SubScene, StageID> SubSceneManager;

class Scene:
    public ResourceManagerImpl,
    public Loadable,
    public SubSceneManager,
    public Managed<Scene> {

public:
    Scene(WindowBase* window);
    ~Scene();

    StageID new_subscene(AvailablePartitioner partitioner=PARTITIONER_OCTREE);            
    SubScene& subscene() { return subscene(default_subscene_); }
    SubScene& subscene(StageID s);
    SubSceneRef subscene_ref(StageID s);

    void delete_subscene(StageID s);
    uint32_t subscene_count() const;

    bool init();
    void render();
    void update(double dt);

    MaterialID default_material_id() const;
    TextureID default_texture_id() const;

    Pipeline& pipeline() { return *pipeline_; }
    GeomFactory& geom_factory() { return *geom_factory_; }

private:
    StageID default_subscene_;
    TexturePtr default_texture_;
    MaterialPtr default_material_;

    void initialize_defaults();

    std::shared_ptr<Pipeline> pipeline_;
    std::shared_ptr<GeomFactory> geom_factory_;

    friend class WindowBase;
};

}

#endif // SCENE_H_INCLUDED
