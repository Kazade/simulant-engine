#include <GLee.h>

#include "scene.h"
#include "renderer.h"
#include "camera.h"
#include "pipeline.h"
#include "loader.h"
#include "stage.h"
#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"

#include "shaders/default_shaders.h"
#include "window_base.h"

namespace kglt {

Scene::Scene(WindowBase* window):
    ResourceManagerImpl(window),
    default_texture_(0),
    default_material_(0),
    pipeline_(new Pipeline(*this)) {

    CameraManager::signal_post_create().connect(sigc::mem_fun(this, &Scene::post_create_callback<Camera, CameraID>));
}

Scene::~Scene() {
    //TODO: Log the unfreed resources (textures, meshes, materials etc.)
}

MaterialID Scene::default_material_id() const {
    return default_material_->id();
}

TextureID Scene::default_texture_id() const {
    return default_texture_->id();
}

void Scene::initialize_defaults() {
    new_camera(); //Create a default camera

    default_stage_ = new_stage(kglt::PARTITIONER_NULL);

    //Create a default stage for the default stage with the default camera
    pipeline_->add_stage(default_stage_, camera().id());

    //Create the default blank texture
    default_texture_ = texture(new_texture()).lock();
    default_texture_->resize(1, 1);
    default_texture_->set_bpp(32);

    default_texture_->data()[0] = 255;
    default_texture_->data()[1] = 255;
    default_texture_->data()[2] = 255;
    default_texture_->data()[3] = 255;
    default_texture_->upload();

    default_material_ = material(new_material_from_file("kglt/materials/multitexture_and_lighting.kglm")).lock();

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
        //Return the first camera, it's very likely this is the only one
        if(!CameraManager::manager_count()) {
            throw std::logic_error("Attempted to retrieve a camera when none exist");
        }
        return CameraManager::manager_get(CameraManager::objects_.begin()->second->id());
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
    //Update the stages
    StageManager::apply_func_to_objects(std::bind(&Object::update, std::tr1::placeholders::_1, dt));
}

void Scene::render() {
    pipeline_->run();
}

}
