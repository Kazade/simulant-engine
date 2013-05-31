#include "subscene.h"
#include "window_base.h"
#include "scene.h"
#include "partitioner.h"

#include "camera.h"
#include "entity.h"
#include "light.h"

namespace kglt {

Stage::Stage(Scene* parent, StageID id):
    generic::Identifiable<StageID>(id),
    Object(nullptr),
    scene_(*parent),
    ambient_light_(1.0, 1.0, 1.0, 1.0){

    EntityManager::signal_post_create().connect(sigc::mem_fun(this, &Stage::post_create_callback<Entity, EntityID>));
    CameraManager::signal_post_create().connect(sigc::mem_fun(this, &Stage::post_create_callback<Camera, CameraID>));
    LightManager::signal_post_create().connect(sigc::mem_fun(this, &Stage::post_create_callback<Light, LightID>));
}

void Stage::destroy() {
    scene().delete_subscene(id());
}

EntityID Stage::new_entity() {
    EntityID result = EntityManager::manager_new();
    //Tell everyone about the new entity
    signal_entity_created_(result);
    return result;
}

EntityID Stage::new_entity(MeshID mid) {
    EntityID result = EntityManager::manager_new();

    //If a mesh was specified, set it
    if(mid) {
        entity(result).set_mesh(mid);
    }

    //Tell everyone about the new entity
    signal_entity_created_(result);

    return result;
}

EntityID Stage::new_entity_with_parent(Entity& parent) {
    Entity& ent = entity(new_entity());
    ent.set_parent(parent);
    return ent.id();
}

EntityID Stage::new_entity_with_parent(Entity& parent, MeshID mid) {
    Entity& ent = entity(new_entity(mid));
    ent.set_parent(parent);
    return ent.id();
}

bool Stage::has_entity(EntityID m) const {
    return EntityManager::manager_contains(m);
}

Entity& Stage::entity(EntityID e) {
    return EntityManager::manager_get(e);
}

void Stage::delete_entity(EntityID e) {
    signal_entity_destroyed_(e);

    entity(e).destroy_children();

    EntityManager::manager_delete(e);
}

LightID Stage::new_light(LightType type) {
    LightID lid = LightManager::manager_new();

    Light& l = light(lid);
    l.set_type(type);

    signal_light_created_(lid);
    return lid;
}

LightID Stage::new_light(Object &parent, LightType type) {
    LightID lid = LightManager::manager_new();

    Light& l = light(lid);
    l.set_type(type);
    l.set_parent(&parent);

    signal_light_created_(lid);

    return lid;
}

Light& Stage::light(LightID light_id) {
    return LightManager::manager_get(light_id);
}

void Stage::delete_light(LightID light_id) {
    Light& obj = light(light_id);
    signal_light_destroyed_(light_id);

    obj.destroy_children();
    LightManager::manager_delete(light_id);
}


CameraID Stage::new_camera() {
    return CameraManager::manager_new();
}

CameraID Stage::new_camera(Object& parent) {
    Camera& cam = camera(CameraManager::manager_new());
    cam.set_parent(&parent);
    return cam.id();
}

Camera& Stage::camera(CameraID c) {
    if(c == CameraID()) {
        //Return the first camera, it's very likely this is the only one
        if(!CameraManager::manager_count()) {
            throw std::logic_error("Attempted to retrieve a camera when none exist");
        }
        return CameraManager::manager_get(CameraManager::objects_.begin()->second->id());
    }
    return CameraManager::manager_get(c);
}

void Stage::delete_camera(CameraID cid) {
    Camera& obj = camera(cid);
    obj.destroy_children();
    CameraManager::manager_delete(cid);
}

void Stage::set_partitioner(Partitioner::ptr partitioner) {
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
