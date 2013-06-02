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
class Stage;
class GeomFactory;

typedef generic::TemplatedManager<Scene, Stage, StageID> StageManager;
typedef generic::TemplatedManager<Scene, Camera, CameraID> CameraManager;

class Scene:
    public ResourceManagerImpl,
    public Loadable,
    public StageManager,
    public CameraManager,
    public Managed<Scene> {

public:
    Scene(WindowBase* window);
    ~Scene();

    StageID new_stage(AvailablePartitioner partitioner=PARTITIONER_OCTREE);            
    Stage& stage() { return stage(default_stage_); }
    Stage& stage(StageID s);
    StageRef stage_ref(StageID s);

    void delete_stage(StageID s);
    uint32_t stage_count() const;

    bool init();
    void render();
    void update(double dt);

    MaterialID default_material_id() const;
    TextureID default_texture_id() const;
    CameraID default_camera_id() const;

    Pipeline& pipeline() { return *pipeline_; }

    CameraID new_camera();
    Camera& camera(CameraID c=CameraID());
    CameraRef camera_ref(CameraID c);
    void delete_camera(CameraID cid);

    template<typename T, typename ID>
    void post_create_callback(T& obj, ID id) {
        obj.set_parent(nullptr);
        obj._initialize();
    }
private:
    StageID default_stage_;
    CameraID default_camera_;

    TexturePtr default_texture_;
    MaterialPtr default_material_;

    void initialize_defaults();

    std::shared_ptr<Pipeline> pipeline_;

    friend class WindowBase;
};

}

#endif // SCENE_H_INCLUDED
