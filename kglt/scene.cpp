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

    window().stage()->host_camera(default_camera_);

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


bool SceneImpl::init() {
    return true;
}

void SceneImpl::update(double dt) {
    if(has_physics_engine()) {
        physics()->step(dt);
    }
}

}
