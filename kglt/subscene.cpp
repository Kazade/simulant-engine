#include "subscene.h"
#include "window_base.h"
#include "scene.h"
#include "partitioner.h"

#include "camera.h"
#include "entity.h"
#include "light.h"

namespace kglt {

void SubScene::do_lua_export(lua_State &state) {
    luabind::module(&state) [
        luabind::class_<SubScene>("SubScene")
            .property("scene", &SubScene::scene)
            .property("entity_count", &SubScene::entity_count)
            .property("light_count", &SubScene::light_count)
    ];
}

WindowBase& SubScene::window() {
    return scene_.window();
}

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
