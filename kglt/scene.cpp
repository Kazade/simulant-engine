#include "utils/glcompat.h"
#include "utils/ownable.h"
#include "background.h"

#include "render_sequence.h"
#include "scene.h"
#include "renderer.h"
#include "camera.h"
#include "loader.h"
#include "stage.h"
#include "ui_stage.h"
#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"
#include "physics/physics_engine.h"
#include "shaders/default_shaders.h"
#include "window_base.h"

namespace kglt {

SceneImpl::SceneImpl(WindowBase* window):
    ResourceManagerImpl(window),
    default_texture_(0),
    default_material_(0) {

}

SceneImpl::~SceneImpl() {
    //TODO: Log the unfreed resources (textures, meshes, materials etc.)    

    //Clear the stages first, they may hold references to cameras, materials
    //etc.
    BackgroundManager::__objects().clear();
    StageManager::__objects().clear();    
    CameraManager::__objects().clear();
}

void SceneImpl::enable_physics(std::shared_ptr<PhysicsEngine> engine) {
    physics_engine_ = engine;
}

PhysicsEnginePtr SceneImpl::physics() {
    if(!physics_engine_) {
        throw std::logic_error("Tried to access the physics engine when one has not been enabled");
    }
    return physics_engine_;
}

const bool SceneImpl::has_physics_engine() const {
    return bool(physics_engine_);
}

void SceneImpl::print_tree() {
    for(auto stage: StageManager::objects_) {
        uint32_t counter = 0;
        print_tree(stage.second.get(), counter);
    }
}

StageID SceneImpl::default_stage_id() const {
    return default_stage_;
}

MaterialID SceneImpl::default_material_id() const {
    return default_material_->id();
}

TextureID SceneImpl::default_texture_id() const {
    return default_texture_->id();
}

CameraID SceneImpl::default_camera_id() const {
    return default_camera_;
}

unicode SceneImpl::default_material_filename() const {
    return window().resource_locator().locate_file("kglt/materials/multitexture_and_lighting.kglm");
}

void SceneImpl::initialize_defaults() {
    default_camera_ = new_camera(); //Create a default camera
    default_stage_ = new_stage(kglt::PARTITIONER_NULL);

    stage(default_stage_)->host_camera(default_camera_);

    default_ui_stage_ = new_ui_stage();
    default_ui_camera_ = new_camera();

    camera(default_ui_camera_)->set_orthographic_projection(
        0, window().width(), window().height(), 0, -1, 1
    );

    //Create a default stage for the default stage with the default camera
    window().render_sequence()->new_pipeline(default_stage_, default_camera_);

    //Add a pipeline for the default UI stage to render
    //after the main pipeline
    window().render_sequence()->new_pipeline(
        default_ui_stage_, default_ui_camera_,
        ViewportID(), TextureID(), 100
    );

    //FIXME: Should lock the default texture and material during construction!

    //Create the default blank texture
    default_texture_ = texture(new_texture()).__object;
    default_texture_->resize(1, 1);
    default_texture_->set_bpp(32);

    default_texture_->data()[0] = 255;
    default_texture_->data()[1] = 255;
    default_texture_->data()[2] = 255;
    default_texture_->data()[3] = 255;
    default_texture_->upload();

    //Maintain ref-count
    default_material_ = material(new_material_from_file(default_material_filename())).__object;

    //Set the default material's first texture to the default (white) texture
    default_material_->technique().pass(0).set_texture_unit(0, default_texture_->id());
}

StageID SceneImpl::new_stage(AvailablePartitioner partitioner) {
    return StageManager::manager_new(StageID(), partitioner);
}

uint32_t SceneImpl::stage_count() const {
    return StageManager::manager_count();
}

/**
 * @brief SceneImpl::stage
 * @return A shared_ptr to the default stage
 *
 * We don't return a ProtectedPtr because it makes usage a nightmare. Stages don't suffer the same potential
 * threading issues as other objects as they are the highest level object. Returning a weak_ptr means that
 * we retain ownership, and calling code won't die if the stage goes missing.
 */
StagePtr SceneImpl::stage() {
    return StageManager::manager_get(default_stage_);
}

StagePtr SceneImpl::stage(StageID s) {
    if(!s) {
        return stage();
    }

    return StageManager::manager_get(s);
}

void SceneImpl::delete_stage(StageID s) {
    //Recurse through the tree, destroying all children
    stage(s)->apply_recursively_leaf_first(&ownable_tree_node_destroy, false);

    StageManager::manager_delete(s);
}


UIStageID SceneImpl::new_ui_stage() {
    return UIStageManager::manager_new();
}

UIStagePtr SceneImpl::ui_stage() {
    return UIStageManager::manager_get(default_ui_stage_);
}

UIStagePtr SceneImpl::ui_stage(UIStageID s) {
    if(!s) {
        return ui_stage();
    }
    return UIStageManager::manager_get(s);
}

void SceneImpl::delete_ui_stage(UIStageID s) {
    UIStageManager::manager_delete(s);
}

uint32_t SceneImpl::ui_stage_count() const {
    return UIStageManager::manager_count();
}

//=============== START CAMERAS ============

CameraID SceneImpl::new_camera() {
    CameraID new_camera = CameraManager::manager_new();

    return new_camera;
}

CameraPtr SceneImpl::camera() {
    return CameraManager::manager_get(default_camera_);
}

CameraPtr SceneImpl::camera(CameraID c) {
    if(!c) {
        //Return the default camera if we are passed a null ID
        return camera();
    }

    return CameraManager::manager_get(c);
}

void SceneImpl::delete_camera(CameraID cid) {
    //Remove any associated proxy
    if(camera(cid)->has_proxy()) {
        camera(cid)->proxy().stage()->evict_camera(cid);
    }

    CameraManager::manager_delete(cid);
}

uint32_t SceneImpl::camera_count() const {
    return CameraManager::manager_count();
}
//============== END CAMERAS ================
//============== START BACKGROUNDS ==========

BackgroundID SceneImpl::new_background() {
    BackgroundID bid = BackgroundManager::manager_new();
    return bid;
}

BackgroundID SceneImpl::new_background_from_file(const unicode& filename, float scroll_x, float scroll_y) {
    BackgroundID result = new_background();
    try {
        background(result)->set_texture(new_texture_from_file(filename));
        background(result)->set_horizontal_scroll_rate(scroll_x);
        background(result)->set_vertical_scroll_rate(scroll_y);
    } catch(...) {
        delete_background(result);
        throw;
    }

    return result;
}

BackgroundPtr SceneImpl::background(BackgroundID bid) {
    return BackgroundManager::manager_get(bid);
}

bool SceneImpl::has_background(BackgroundID bid) const {
    return BackgroundManager::manager_contains(bid);
}

void SceneImpl::delete_background(BackgroundID bid) {
    BackgroundManager::manager_delete(bid);
}

uint32_t SceneImpl::background_count() const {
    return BackgroundManager::manager_count();
}

//============== END BACKGROUNDS ============


bool SceneImpl::init() {
    return true;
}

void SceneImpl::update(double dt) {
    if(has_physics_engine()) {
        physics()->step(dt);
    }

    //Update the backgrounds
    for(auto background_pair: BackgroundManager::__objects()) {
        auto* bg = background_pair.second.get();
        bg->update(dt);
    }

    //Update the stages
    for(auto stage_pair: StageManager::__objects()) {
        GenericTreeNode* root = stage_pair.second.get();
        root->apply_recursively([=](GenericTreeNode* node) -> void {
            node->as<Updateable>()->update(dt);
        });
    }
}

}
