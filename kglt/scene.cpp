#include <GLee.h>

#include "scene.h"
#include "renderer.h"
#include "camera.h"
#include "render_sequence.h"
#include "loader.h"
#include "stage.h"
#include "ui_stage.h"

#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"

#include "shaders/default_shaders.h"
#include "window_base.h"

namespace kglt {

Scene::Scene(WindowBase* window):
    ResourceManagerImpl(window),
    default_texture_(0),
    default_material_(0),
    render_sequence_(new RenderSequence(*this)) {

    CameraManager::signal_post_create().connect(sigc::mem_fun(this, &Scene::post_create_callback<Camera, CameraID>));
}

Scene::~Scene() {
    //TODO: Log the unfreed resources (textures, meshes, materials etc.)
}

MaterialID Scene::clone_default_material() {
    return MaterialManager::manager_clone(default_material_->id());
}

MaterialID Scene::default_material_id() const {
    return default_material_->id();
}

TextureID Scene::default_texture_id() const {
    return default_texture_->id();
}

void Scene::initialize_defaults() {
    default_camera_ = new_camera(); //Create a default camera
    default_stage_ = new_stage(kglt::PARTITIONER_NULL);

    default_ui_stage_ = new_ui_stage();
    default_ui_camera_ = new_camera();

    camera(default_ui_camera_).set_orthographic_projection(
        0, window().width(), window().height(), 0, -1, 1
    );

    //Create a default stage for the default stage with the default camera
    render_sequence_->new_pipeline(default_stage_, default_camera_);

    //Add a pipeline for the default UI stage to render
    //after the main pipeline
    render_sequence_->new_pipeline(
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
    default_material_ = material(new_material_from_file("kglt/materials/multitexture_and_lighting.kglm")).__object;

    //Set the default material's first texture to the default (white) texture
    default_material_->technique().pass(0).set_texture_unit(0, default_texture_->id());
}

StageID Scene::new_stage(AvailablePartitioner partitioner) {
    Stage& ss = stage(StageManager::manager_new());

    switch(partitioner) {
        case PARTITIONER_NULL:
        ss.set_partitioner(Partitioner::ptr(new NullPartitioner(ss)));
        break;
        case PARTITIONER_OCTREE:
        ss.set_partitioner(Partitioner::ptr(new OctreePartitioner(ss)));
        break;
        default: {
            delete_stage(ss.id());
            throw std::logic_error("Invalid partitioner type specified");
        }
    }

    return ss.id();
}

uint32_t Scene::stage_count() const {
    return StageManager::manager_count();
}

Stage& Scene::stage(StageID s) {
    if(s == DefaultStageID) {
        return StageManager::manager_get(default_stage_);
    }

    return StageManager::manager_get(s);
}

StageRef Scene::stage_ref(StageID s) {
    if(!StageManager::manager_contains(s)) {
        throw DoesNotExist<Stage>();
    }
    return StageManager::__objects()[s];
}

void Scene::delete_stage(StageID s) {
    Stage& ss = stage(s);
    ss.destroy_children();
    StageManager::manager_delete(s);
}


UIStageID Scene::new_ui_stage() {
    return UIStageManager::manager_new();
}

ProtectedPtr<UIStage> Scene::ui_stage() {
    return ui_stage(default_ui_stage_);
}

ProtectedPtr<UIStage> Scene::ui_stage(UIStageID s) {
    if(!s) {
        return UIStageManager::manager_get_ref(default_ui_stage_);
    } else {
        return UIStageManager::manager_get_ref(s);
    }
}

void Scene::delete_ui_stage(UIStageID s) {
    UIStageManager::manager_delete(s);
}

uint32_t Scene::ui_stage_count() const {
    return UIStageManager::manager_count();
}

CameraRef Scene::camera_ref(CameraID c) {
    if(!CameraManager::manager_contains(c)) {
        throw DoesNotExist<Camera>();
    }
    return CameraManager::__objects()[c];
}

CameraID Scene::new_camera() {
    return CameraManager::manager_new();
}

Camera& Scene::camera(CameraID c) {
    if(c == CameraID()) {
        //Return the default camera
        return CameraManager::manager_get(default_camera_);
    }
    return CameraManager::manager_get(c);
}

void Scene::delete_camera(CameraID cid) {
    Camera& obj = camera(cid);
    obj.destroy_children();
    CameraManager::manager_delete(cid);
}

bool Scene::init() {
    assert(glGetError() == GL_NO_ERROR);

    return true;
}

void Scene::update(double dt) {
    PhysicsEngine* engine = physics_engine();
    if(engine) {
        engine->step(dt);
    }

    //Update the stages
    StageManager::apply_func_to_objects(std::bind(&Object::update, std::placeholders::_1, dt));
    CameraManager::apply_func_to_objects(std::bind(&Object::update, std::placeholders::_1, dt));
}

void Scene::render() {
    render_sequence_->run();
}

}
