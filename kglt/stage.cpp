#include "stage.h"
#include "window_base.h"
#include "scene.h"
#include "partitioner.h"
#include "actor.h"
#include "light.h"
#include "camera.h"
#include "debug.h"
#include "sprite.h"

#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"
#include "procedural/geom_factory.h"
#include "utils/ownable.h"

namespace kglt {

Stage::Stage(Scene* parent, StageID id, AvailablePartitioner partitioner):
    generic::Identifiable<StageID>(id),
    scene_(*parent),
    ambient_light_(1.0, 1.0, 1.0, 1.0),
    geom_factory_(new GeomFactory(*this)) {

    set_partitioner(partitioner);

    ActorManager::signal_post_create().connect(std::bind(&Stage::post_create_callback<Actor, ActorID>, this, std::placeholders::_1, std::placeholders::_2));
    LightManager::signal_post_create().connect(std::bind(&Stage::post_create_callback<Light, LightID>, this, std::placeholders::_1, std::placeholders::_2));
}

bool Stage::init() {
    debug_ = Debug::create(*this);

    return true;
}

void Stage::cleanup() {
    SpriteManager::objects_.clear();
    LightManager::objects_.clear();
    ActorManager::objects_.clear();
    CameraProxyManager::objects_.clear();
}

void Stage::ask_owner_for_destruction() {
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
    actor(result)->set_parent(this);

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

ActorID Stage::new_actor_with_parent_and_mesh(SpriteID parent, MeshID mid) {
    ActorID new_id = new_actor(mid);
    actor(new_id)->set_parent(parent);
    return new_id;
}

ActorID Stage::new_actor_with_parent_and_mesh(ActorID parent, MeshID mid) {
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

//=============== SPRITES ===================

SpriteID Stage::new_sprite() {
    SpriteID s = SpriteManager::manager_new();
    sprite(s)->set_parent(this);
    signal_sprite_created_(s);
    return s;
}

SpriteID Stage::new_sprite_from_file(const unicode& filename, uint32_t frame_width, uint32_t frame_height, uint32_t margin, uint32_t spacing, std::pair<uint32_t, uint32_t> padding) {
    SpriteID s = new_sprite();
    TextureID t = new_texture_from_file(filename);
    try {
        sprite(s)->set_spritesheet(t, frame_width, frame_height, margin, spacing, padding);
    } catch(...) {
        delete_sprite(s);
        throw;
    }

    return s;
}

ProtectedPtr<Sprite> Stage::sprite(SpriteID s) {
    return SpriteManager::manager_get(s);
}

bool Stage::has_sprite(SpriteID s) const {
    return SpriteManager::manager_contains(s);
}

void Stage::delete_sprite(SpriteID s) {   
    sprite(s)->apply_recursively_leaf_first(&ownable_tree_node_destroy, false);
    sprite(s)->detach();
    SpriteManager::manager_delete(s);
}

uint32_t Stage::sprite_count() const {
    return SpriteManager::manager_count();
}

//============== END SPRITES ================


LightID Stage::new_light(LightType type) {
    LightID lid = LightManager::manager_new();
    light(lid)->set_type(type);
    light(lid)->set_parent(this);
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

    if(scene().camera(c)->has_proxy()) {
        //Destroy any existing proxy
        scene().camera(c)->proxy().stage().evict_camera(c);
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

void Stage::set_partitioner(AvailablePartitioner partitioner) {
    switch(partitioner) {
        case PARTITIONER_NULL:
            partitioner_ = Partitioner::ptr(new NullPartitioner(*this));
        break;
        case PARTITIONER_OCTREE:
            partitioner_ = Partitioner::ptr(new OctreePartitioner(*this));
        break;
        default: {
            throw std::logic_error("Invalid partitioner type specified");
        }
    }

    assert(partitioner_);

    //Keep the partitioner updated with new meshes and lights
    signal_actor_created().connect(std::bind(&Partitioner::add_actor, partitioner_.get(), std::placeholders::_1));
    signal_actor_destroyed().connect(std::bind(&Partitioner::remove_actor, partitioner_.get(), std::placeholders::_1));

    signal_light_created().connect(std::bind(&Partitioner::add_light, partitioner_.get(), std::placeholders::_1));
    signal_light_destroyed().connect(std::bind(&Partitioner::remove_light, partitioner_.get(), std::placeholders::_1));
}


void Stage::update(double dt) {

}

}
