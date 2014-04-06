#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <stdexcept>
#include <map>

#include "types.h"
#include "resource_manager.h"
#include "generic/manager.h"
#include "generic/generic_tree.h"
#include "physics/physics_engine.h"
#include "interfaces.h"

namespace kglt {

class WindowBase;
class UI;

class Scene;
class Stage;
class GeomFactory;
class Background;

typedef generic::TemplatedManager<Scene, Stage, StageID> StageManager;
typedef generic::TemplatedManager<Scene, Camera, CameraID> CameraManager;
typedef generic::TemplatedManager<Scene, UIStage, UIStageID> UIStageManager;
typedef generic::TemplatedManager<Scene, Background, BackgroundID> BackgroundManager;

class Scene:
    public ResourceManagerImpl,
    public Loadable,
    public StageManager,
    public CameraManager,
    public UIStageManager,
    public BackgroundManager,
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

    //Camera functions
    CameraID new_camera();
    ProtectedPtr<Camera> camera();
    ProtectedPtr<Camera> camera(CameraID c);
    void delete_camera(CameraID cid);
    uint32_t camera_count() const;
    //End camera

    template<typename T, typename ID>
    void post_create_callback(T& obj, ID id) {
        obj.set_parent(nullptr);
        obj._initialize();
    }

    void render_tree();

    //Background functions
    BackgroundID new_background();
    BackgroundID new_background_from_file(const unicode& filename, float scroll_x=0.0, float scroll_y=0.0);
    ProtectedPtr<Background> background(BackgroundID bid);
    bool has_background(BackgroundID bid) const;
    void delete_background(BackgroundID bid);
    uint32_t background_count() const;
    // End background

private:    
    void render_tree(GenericTreeNode* node, uint32_t& level) {
        for(uint32_t i = 0; i < level; ++i) {
            std::cout << "    ";
        }

        std::cout << *dynamic_cast<Printable*>(node) << std::endl;

        level += 1;
        for(auto child: node->children()) {
            render_tree(child, level);
        }
        level -= 1;
    }

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
