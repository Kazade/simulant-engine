#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <sigc++/sigc++.h>
#include <stdexcept>
#include <map>

#include "types.h"
#include "resource_manager.h"
#include "generic/manager.h"
#include "physics/physics_engine.h"

namespace kglt {

class WindowBase;
class UI;

class Scene;
class Stage;
class GeomFactory;

typedef generic::TemplatedManager<Scene, Stage, StageID> StageManager;
typedef generic::TemplatedManager<Scene, Camera, CameraID> CameraManager;
typedef generic::TemplatedManager<Scene, UIStage, UIStageID> UIStageManager;

class Scene:
    public ResourceManagerImpl,
    public Loadable,
    public StageManager,
    public CameraManager,
    public UIStageManager,
    public Managed<Scene> {

public:
    Scene(WindowBase* window);
    ~Scene();

    void enable_physics(std::shared_ptr<PhysicsEngine> engine) {
        physics_engine_ = engine;
    }

    PhysicsEngine& physics() const {
        if(!physics_engine_) {
            throw std::logic_error("Tried to access the physics engine when one has not been enabled");
        }
        return *physics_engine_.get();
    }

    bool physics_enabled() const {
        return bool(physics_engine_);
    }

    StageID new_stage(AvailablePartitioner partitioner=PARTITIONER_OCTREE);            
    Stage& stage() { return stage(default_stage_); }
    Stage& stage(StageID s);
    StageRef stage_ref(StageID s);

    void delete_stage(StageID s);
    uint32_t stage_count() const;

    //UI Stages
    UIStageID new_ui_stage();
    ProtectedPtr<UIStage> ui_stage();
    ProtectedPtr<UIStage> ui_stage(UIStageID s);
    void delete_ui_stage(UIStageID s);
    uint32_t ui_stage_count() const;

    bool init();
    void render();
    void update(double dt);

    MaterialID clone_default_material();
    MaterialID default_material_id() const;
    TextureID default_texture_id() const;
    CameraID default_camera_id() const;

    RenderSequence& render_sequence() { return *render_sequence_; }

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

    UIStageID default_ui_stage_;
    CameraID default_ui_camera_;

    TexturePtr default_texture_;
    MaterialPtr default_material_;

    void initialize_defaults();

    std::shared_ptr<RenderSequence> render_sequence_;
    std::shared_ptr<PhysicsEngine> physics_engine_;

    friend class WindowBase;
};

}

#endif // SCENE_H_INCLUDED
