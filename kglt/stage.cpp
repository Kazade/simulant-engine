#include "stage.h"
#include "window_base.h"
#include "scene.h"
#include "partitioner.h"
#include "entity.h"
#include "light.h"

#include "procedural/geom_factory.h"

namespace kglt {

Stage::Stage(Scene* parent, StageID id):
    generic::Identifiable<StageID>(id),
    Object(nullptr),
    scene_(*parent),
    ambient_light_(1.0, 1.0, 1.0, 1.0),
    geom_factory_(new GeomFactory(*this)) {

    ActorManager::signal_post_create().connect(sigc::mem_fun(this, &Stage::post_create_callback<Actor, ActorID>));    
    LightManager::signal_post_create().connect(sigc::mem_fun(this, &Stage::post_create_callback<Light, LightID>));
}

void Stage::destroy() {
    scene().delete_stage(id());
}

ActorID Stage::new_entity() {
    ActorID result = ActorManager::manager_new();
    //Tell everyone about the new entity
    signal_entity_created_(result);
    return result;
}

ActorID Stage::new_entity(MeshID mid) {
    ActorID result = ActorManager::manager_new();

    //If a mesh was specified, set it
    if(mid) {
        entity(result).set_mesh(mid);
    }

    //Tell everyone about the new entity
    signal_entity_created_(result);

    return result;
}

ActorID Stage::new_entity_with_parent(Actor& parent) {
    Actor& ent = entity(new_entity());
    ent.set_parent(parent);
    return ent.id();
}

ActorID Stage::new_entity_with_parent(Actor& parent, MeshID mid) {
    Actor& ent = entity(new_entity(mid));
    ent.set_parent(parent);
    return ent.id();
}

bool Stage::has_entity(ActorID m) const {
    return ActorManager::manager_contains(m);
}

Actor& Stage::entity(ActorID e) {
    return ActorManager::manager_get(e);
}

ActorRef Stage::entity_ref(ActorID e) {
    if(!ActorManager::manager_contains(e)) {
        throw DoesNotExist<Stage>();
    }
    return ActorManager::__objects()[e];
}

void Stage::delete_entity(ActorID e) {
    signal_entity_destroyed_(e);

    entity(e).destroy_children();

    ActorManager::manager_delete(e);
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
