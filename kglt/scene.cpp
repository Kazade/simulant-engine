#include "glee/GLee.h"
#include "scene.h"
#include "renderer.h"
#include "partitioners/null_partitioner.h"
#include "shaders/default_shaders.h"
#include "window_base.h"

namespace kglt {

Scene::Scene(WindowBase* window):
    Object(nullptr),    
    ResourceManager(window, nullptr),
    default_camera_(0),
    default_scene_group_(0),
    default_texture_(0),
    default_material_(0),
    ambient_light_(1.0, 1.0, 1.0, 1.0),    
    pipeline_(new Pipeline(*this)) {

    TemplatedManager<Scene, Entity, EntityID>::signal_post_create().connect(sigc::mem_fun(this, &Scene::post_create_callback<Entity, EntityID>));
    TemplatedManager<Scene, Camera, CameraID>::signal_post_create().connect(sigc::mem_fun(this, &Scene::post_create_callback<Camera, CameraID>));    
}

Scene::~Scene() {
    //TODO: Log the unfreed resources (textures, meshes, materials etc.)
}

void Scene::initialize_defaults() {
    default_camera_ = new_camera(); //Create a default camera

    default_scene_group_ = new_scene_group();

    //Create a default pass for the default scene group
    pipeline_->add_pass(default_scene_group_);

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
    this->window().loader_for("kglt/materials/generic_multitexture.kglm")->into(mat);

    //Set the default material's first texture to the default (white) texture
    mat.technique().pass(0).set_texture_unit(0, tex.id());
}

EntityID Scene::new_entity(MeshID mid) {
    EntityID result = TemplatedManager<Scene, Entity, EntityID>::manager_new();

    //If a mesh was specified, set it
    if(mid) {
        entity(result).set_mesh(mid);
    }

    //Set the scene group to the default (otherwise nothing gets rendered)
    entity(result).scene_group = scene_group(default_scene_group_);

    //Tell everyone about the new entity
    signal_entity_created_(result);

    return result;
}

EntityID Scene::new_entity(Object& parent, MeshID mid) {
    EntityID result = new_entity(mid);
    entity(result).set_parent(&parent);
    return result;
}

bool Scene::has_entity(EntityID m) const {
    return TemplatedManager<Scene, Entity, EntityID>::manager_contains(m);
}

Entity& Scene::entity(EntityID e) {
    return TemplatedManager<Scene, Entity, EntityID>::manager_get(e);
}

void Scene::delete_entity(EntityID e) {
    signal_entity_destroyed_(e);

    TemplatedManager<Scene, Entity, EntityID>::manager_delete(e);
}


CameraID Scene::new_camera() {
    return TemplatedManager<Scene, Camera, CameraID>::manager_new();
}

Camera& Scene::camera(CameraID c) {
    if(c == 0) {
        return camera(default_camera_);
    }

    return TemplatedManager<Scene, Camera, CameraID>::manager_get(c);
}

void Scene::delete_camera(CameraID cid) {
    Camera& obj = camera(cid);
    obj.destroy_children();
    TemplatedManager<Scene, Camera, CameraID>::manager_delete(cid);
}

SceneGroupID Scene::new_scene_group() {
    return TemplatedManager<Scene, SceneGroup, SceneGroupID>::manager_new();
}

SceneGroup& Scene::scene_group(SceneGroupID group) {
    if(group) {
        return TemplatedManager<Scene, SceneGroup, SceneGroupID>::manager_get(group);
    } else {
        return TemplatedManager<Scene, SceneGroup, SceneGroupID>::manager_get(default_scene_group_);
    }
}

void Scene::delete_scene_group(SceneGroupID group) {
    TemplatedManager<Scene, SceneGroup, SceneGroupID>::manager_delete(group);
}

LightID Scene::new_light(Object* parent, LightType type) {
    LightID lid = TemplatedManager<Scene, Light, LightID>::manager_new();

    Light& l = light(lid);
    l.set_type(type);
    if(parent) {
        l.set_parent(parent);
    }

    signal_light_created_(lid);

    return lid;
}

Light& Scene::light(LightID light_id) {
    return TemplatedManager<Scene, Light, LightID>::manager_get(light_id);
}

void Scene::delete_light(LightID light_id) {    
    Light& obj = light(light_id);
    signal_light_destroyed_(light_id);

    obj.destroy_children();
    TemplatedManager<Scene, Light, LightID>::manager_delete(light_id);
}

void Scene::init() {
    assert(glGetError() == GL_NO_ERROR);

    initialize_defaults();
    pipeline_->init(); //Initialize the pipeline

}

void Scene::update(double dt) {
    /*
      Update all animated materials
    */
    for(std::pair<MaterialID, Material::ptr> p: MaterialManager::objects_) {
        p.second->update(dt);
    }

    for(uint32_t i = 0; i < child_count(); ++i) {
        Object& c = child(i);
        c.update(dt);
    }
}

void Scene::render() {
    pipeline_->run();
}

}
