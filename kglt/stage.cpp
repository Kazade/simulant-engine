#include "stage.h"
#include "window_base.h"
#include "scene.h"
#include "partitioner.h"
#include "actor.h"
#include "light.h"
#include "camera.h"

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

ActorID Stage::new_actor() {
    return new_actor(false, false);
}

ActorID Stage::new_actor(bool make_responsive, bool make_collidable) {
    ActorID result = ActorManager::manager_new();

    if(make_responsive) {
        actor(result)->make_responsive();
    }

    if(make_collidable) {
        actor(result)->make_collidable();
    }

    //Tell everyone about the new actor
    signal_actor_created_(result);
    return result;
}

ActorID Stage::new_actor(MeshID mid) {
    return new_actor(mid, false, false);
}

ActorID Stage::new_actor(MeshID mid, bool make_responsive, bool make_collidable) {
    ActorID result = ActorManager::manager_new();

    //If a mesh was specified, set it
    if(mid) {
        actor(result)->set_mesh(mid);
    }

    if(make_responsive) {
        actor(result)->make_responsive();
    }

    if(make_collidable) {
        actor(result)->make_collidable();
    }

    //Tell everyone about the new actor
    signal_actor_created_(result);

    return result;
}

ActorID Stage::new_actor_with_parent(ActorID parent) {
    ActorID new_id = new_actor();
    actor(new_id)->set_parent(parent);
    return new_id;
}

ActorID Stage::new_actor_with_parent(ActorID parent, MeshID mid) {
    ActorID new_id = new_actor(mid);
    actor(new_id)->set_parent(parent);
    return new_id;
}

bool Stage::has_actor(ActorID m) const {
    return ActorManager::manager_contains(m);
}

ProtectedPtr<Actor> Stage::actor(ActorID e) {
    return ProtectedPtr<Actor>(ActorManager::manager_get(e));
}

const ProtectedPtr<Actor> Stage::actor(ActorID e) const {
    return ProtectedPtr<Actor>(ActorManager::manager_get(e));
}

void Stage::delete_actor(ActorID e) {
    signal_actor_destroyed_(e);

    actor(e)->destroy_children();

    ActorManager::manager_delete(e);
}

LightID Stage::new_light(LightType type) {
    LightID lid = LightManager::manager_new();
    light(lid)->set_type(type);
    signal_light_created_(lid);
    return lid;
}

LightID Stage::new_light(Object &parent, LightType type) {
    LightID lid = LightManager::manager_new();

    {
        auto l = light(lid);
        l->set_type(type);
        l->set_parent(&parent);
    }

    signal_light_created_(lid);

    return lid;
}

ProtectedPtr<Light> Stage::light(LightID light_id) {
    return ProtectedPtr<Light>(LightManager::manager_get(light_id));
}

void Stage::delete_light(LightID light_id) {
    signal_light_destroyed_(light_id);
    light(light_id)->destroy_children();
    LightManager::manager_delete(light_id);
}

void Stage::host_camera(CameraID c) {
    if(!c) {
        c = scene().default_camera_id();
    }

    if(scene().camera(c).has_proxy()) {
        //Destroy any existing proxy
        scene().camera(c).proxy().stage().evict_camera(c);
    }

    //Create a camera proxy for the camera ID
    CameraProxyManager::manager_new(c);

    camera(c)->set_parent(this);
}

void Stage::evict_camera(CameraID c) {
    if(!c) {
        c = scene().default_camera_id();
    }

    //Delete the camera proxy
    CameraProxyManager::manager_delete(c);
}

ProtectedPtr<CameraProxy> Stage::camera(CameraID c) {
    if(!c) {
        c = scene().default_camera_id();
    }

    return ProtectedPtr<CameraProxy>(CameraProxyManager::manager_get(c));
}

void Stage::set_partitioner(Partitioner::ptr partitioner) {
    assert(partitioner);

    partitioner_ = partitioner;

    assert(partitioner_);

    //Keep the partitioner updated with new meshes and lights
    signal_actor_created().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::add_actor));
    signal_actor_destroyed().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::remove_actor));

    signal_light_created().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::add_light));
    signal_light_destroyed().connect(sigc::mem_fun(partitioner_.get(), &Partitioner::remove_light));
}


}
