#include <GLee.h>

#include "entity.h"
#include "scene.h"
#include "renderer.h"
#include "camera.h"
#include "pipeline.h"
#include "light.h"

#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"

#include "shaders/default_shaders.h"
#include "window_base.h"

namespace kglt {

SubScene::SubScene(Scene* parent, SubSceneID id):
    generic::Identifiable<SubSceneID>(id),
    SceneBase(&parent->window(), parent),
    Object(nullptr),
    scene_(*parent),
    ambient_light_(1.0, 1.0, 1.0, 1.0){

    EntityManager::signal_post_create().connect(sigc::mem_fun(this, &SubScene::post_create_callback<Entity, EntityID>));
    CameraManager::signal_post_create().connect(sigc::mem_fun(this, &SubScene::post_create_callback<Camera, CameraID>));
    LightManager::signal_post_create().connect(sigc::mem_fun(this, &SubScene::post_create_callback<Light, LightID>));
}

void SubScene::destroy() {
    scene().delete_subscene(id());
}

EntityID SubScene::new_entity(MeshID mid) {
    EntityID result = EntityManager::manager_new();

    //If a mesh was specified, set it
    if(mid) {
        entity(result).set_mesh(mid);
    }

    //Tell everyone about the new entity
    signal_entity_created_(result);

    return result;
}

EntityID SubScene::new_entity(Object& parent, MeshID mid) {
    EntityID result = new_entity(mid);
    entity(result).set_parent(&parent);
    return result;
}

bool SubScene::has_entity(EntityID m) const {
    return EntityManager::manager_contains(m);
}

Entity& SubScene::entity(EntityID e) {
    return EntityManager::manager_get(e);
}

void SubScene::delete_entity(EntityID e) {
    signal_entity_destroyed_(e);

    entity(e).destroy_children();

    EntityManager::manager_delete(e);
}

LightID SubScene::new_light(LightType type) {
    LightID lid = LightManager::manager_new();

    Light& l = light(lid);
    l.set_type(type);

    signal_light_created_(lid);
    return lid;
}

LightID SubScene::new_light(Object &parent, LightType type) {
    LightID lid = LightManager::manager_new();

    Light& l = light(lid);
    l.set_type(type);
    l.set_parent(&parent);

    signal_light_created_(lid);

    return lid;
}

Light& SubScene::light(LightID light_id) {
    return LightManager::manager_get(light_id);
}

void SubScene::delete_light(LightID light_id) {
    Light& obj = light(light_id);
    signal_light_destroyed_(light_id);

    obj.destroy_children();
    LightManager::manager_delete(light_id);
}


CameraID SubScene::new_camera() {
    return CameraManager::manager_new();
}

CameraID SubScene::new_camera(Object& parent) {
    Camera& cam = camera(CameraManager::manager_new());
    cam.set_parent(&parent);
    return cam.id();
}

Camera& SubScene::camera(CameraID c) {
    if(c == CameraID()) {
        //Return the first camera, it's very likely this is the only one
        if(!CameraManager::manager_count()) {
            throw std::logic_error("Attempted to retrieve a camera when none exist");
        }
        return CameraManager::manager_get(CameraManager::objects_.begin()->second->id());
    }
    return CameraManager::manager_get(c);
}

void SubScene::delete_camera(CameraID cid) {
    Camera& obj = camera(cid);
    obj.destroy_children();
    CameraManager::manager_delete(cid);
}

Scene::Scene(WindowBase* window):
    SceneBase(window, nullptr),
    default_texture_(0),
    default_material_(0),
    pipeline_(new Pipeline(*this)) {

}

Scene::~Scene() {
    //TODO: Log the unfreed resources (textures, meshes, materials etc.)
}

void Scene::initialize_defaults() {
    default_subscene_ = new_subscene(kglt::PARTITIONER_NULL);

    //Create a default stage for the default subscene with it's default camera
    pipeline_->add_stage(default_subscene_, subscene().camera().id());

    //Create the default blank texture
    default_texture_ = new_texture();
    Texture& tex = texture(default_texture_);
    tex.resize(1, 1);
    tex.set_bpp(32);

    tex.data()[0] = 255;
    tex.data()[1] = 255;
    tex.data()[2] = 255;
    tex.data()[3] = 255;
    tex.upload();

    default_material_ = new_material();
    Material& mat = material(default_material_);
    this->window().loader_for("kglt/materials/multitexture_and_lighting.kglm")->into(mat);

    //Set the default material's first texture to the default (white) texture
    mat.technique().pass(0).set_texture_unit(0, tex.id());
}

SubSceneID Scene::new_subscene(AvailablePartitioner partitioner) {
    SubScene& ss = subscene(SubSceneManager::manager_new());

    switch(partitioner) {
        case PARTITIONER_NULL:
        ss.set_partitioner(Partitioner::ptr(new NullPartitioner(ss)));
        break;
        case PARTITIONER_OCTREE:
        ss.set_partitioner(Partitioner::ptr(new OctreePartitioner(ss)));
        break;
        default: {
            delete_subscene(ss.id());
            throw std::logic_error("Invalid partitioner type specified");
        }
    }

    //All subscenes should have at least one camera
    ss.new_camera();

    return ss.id();
}

SubScene& Scene::subscene(SubSceneID s) {
    if(s == DefaultSubSceneID) {
        return SubSceneManager::manager_get(default_subscene_);
    }

    return SubSceneManager::manager_get(s);
}

void Scene::delete_subscene(SubSceneID s) {
    SubScene& ss = subscene(s);
    ss.destroy_children();
    SubSceneManager::manager_delete(s);
}

bool Scene::init() {
    assert(glGetError() == GL_NO_ERROR);

    initialize_defaults();

    return true;
}

void Scene::update(double dt) {
    update_materials(dt);

    //Update the subscenes
    SubSceneManager::apply_func_to_objects(std::bind(&Object::update, std::tr1::placeholders::_1, dt));
}

void Scene::render() {
    pipeline_->run();
}

void SubScene::set_partitioner(Partitioner::ptr partitioner) {
    assert(partitioner);

    partitioner_ = partitioner;

    assert(partitioner_);

    //Keep the partitioner updated with new meshes and lights
    signal_entity_created().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::add_entity));
    signal_entity_destroyed().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::remove_entity));

    signal_light_created().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::add_light));
    signal_light_destroyed().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::remove_light));
}

}
