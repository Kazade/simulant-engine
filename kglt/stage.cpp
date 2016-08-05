#include "stage.h"
#include "window_base.h"
#include "partitioner.h"
#include "actor.h"
#include "light.h"
#include "camera.h"
#include "debug.h"
#include "sprite.h"
#include "particles.h"
#include "geom.h"

#include "loader.h"
#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"
#include "utils/ownable.h"
#include "renderers/batching/render_queue.h"

namespace kglt {

Stage::Stage(StageID id, WindowBase *parent, AvailablePartitioner partitioner):
    WindowHolder(parent),
    generic::Identifiable<StageID>(id),
    SkyboxManager(parent, this),
    resource_manager_(ResourceManager::create(parent, parent->shared_assets.get())),
    ambient_light_(kglt::Colour::WHITE),
    geom_manager_(new GeomManager()) {

    set_partitioner(partitioner);
    render_queue_.reset(new batcher::RenderQueue(this, parent->renderer.get()));

    ActorManager::signal_post_create().connect(std::bind(&Stage::post_create_callback<Actor, ActorID>, this, std::placeholders::_1, std::placeholders::_2));
    LightManager::signal_post_create().connect(std::bind(&Stage::post_create_callback<Light, LightID>, this, std::placeholders::_1, std::placeholders::_2));    
}

Stage::~Stage() {

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
    window->delete_stage(id());
}

void Stage::on_subactor_material_changed(
    ActorID actor_id, SubActor* subactor, MaterialID old, MaterialID newM
) {
    ActorChangeEvent evt;
    evt.type = ACTOR_CHANGE_TYPE_SUBACTOR_MATERIAL_CHANGED;
    evt.subactor_material_changed = { old, newM };

    signal_actor_changed_(actor_id, evt);
}

ActorID Stage::new_actor(RenderableCullingMode mode) {
    using namespace std::placeholders;

    ActorID result = ActorManager::manager_new(this);
    actor(result)->set_renderable_culling_mode(mode);
    actor(result)->set_parent(this);
    actor(result)->signal_subactor_material_changed().connect(
        std::bind(&Stage::on_subactor_material_changed, this, _1, _2, _3, _4)
    );

    //Tell everyone about the new actor
    signal_actor_created_(result);
    return result;
}

ActorID Stage::new_actor_with_mesh(MeshID mid, RenderableCullingMode mode) {
    using namespace std::placeholders;

    ActorID result = ActorManager::manager_new(this);
    actor(result)->set_renderable_culling_mode(mode);
    actor(result)->set_parent(this);
    actor(result)->signal_subactor_material_changed().connect(
        std::bind(&Stage::on_subactor_material_changed, this, _1, _2, _3, _4)
    );

    //If a mesh was specified, set it
    if(mid) {
        actor(result)->set_mesh(mid);
    }

    //Tell everyone about the new actor
    signal_actor_created_(result);

    return result;
}

ActorID Stage::new_actor_with_parent(ActorID parent, RenderableCullingMode mode) {
    ActorID new_id = new_actor(mode);
    actor(new_id)->set_parent(parent);
    return new_id;
}

ActorID Stage::new_actor_with_parent_and_mesh(SpriteID parent, MeshID mid, RenderableCullingMode mode) {
    ActorID new_id = new_actor_with_mesh(mid, mode);
    actor(new_id)->set_parent(parent);
    return new_id;
}

ActorID Stage::new_actor_with_parent_and_mesh(ActorID parent, MeshID mid, RenderableCullingMode mode) {
    ActorID new_id = new_actor_with_mesh(mid, mode);
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

//=============== GEOMS =====================

GeomID Stage::new_geom_with_mesh(MeshID mid) {
    auto gid = geom_manager_->manager_new(this, mid);
    geom(gid)->set_parent(this);

    signal_geom_created_(gid);

    return gid;
}

GeomPtr Stage::geom(const GeomID gid) const {
    return GeomPtr(geom_manager_->manager_get(gid));
}

GeomID Stage::new_geom_with_mesh_at_position(MeshID mid, const Vec3& position, const Quaternion& rotation) {
    auto gid = geom_manager_->manager_new(this, mid, position, rotation);
    geom(gid)->set_parent(this);

    signal_geom_created_(gid);

    return gid;
}

bool Stage::has_geom(GeomID geom_id) const {
    return geom_manager_->manager_contains(geom_id);
}

void Stage::delete_geom(GeomID geom_id) {
    signal_geom_destroyed_(geom_id);

    geom_manager_->manager_delete(geom_id);    
}

uint32_t Stage::geom_count() const {
    return geom_manager_->manager_count();
}

//=============== PARTICLES =================

ParticleSystemID Stage::new_particle_system() {
    ParticleSystemID new_id = ParticleSystemManager::manager_new(this);

    signal_particle_system_created_(new_id);
    return new_id;
}

ParticleSystemID Stage::new_particle_system_from_file(const unicode& filename, bool destroy_on_completion) {
    ParticleSystemID new_id = new_particle_system();

    auto ps = particle_system(new_id);
    ps->set_parent(this);
    ps->set_destroy_on_completion(destroy_on_completion);

    window->loader_for(filename)->into(ps);

    return new_id;
}

ParticleSystemID Stage::new_particle_system_with_parent_from_file(ActorID parent, const unicode& filename, bool destroy_on_completion) {
    ParticleSystemID new_id = new_particle_system();

    auto ps = particle_system(new_id);
    ps->set_parent(parent);
    ps->set_destroy_on_completion(destroy_on_completion);

    window->loader_for(filename)->into(ps);

    return new_id;
}

ParticleSystemPtr Stage::particle_system(ParticleSystemID pid) {
    return ParticleSystemManager::manager_get(pid);
}

bool Stage::has_particle_system(ParticleSystemID pid) const {
    return ParticleSystemManager::manager_contains(pid);
}

void Stage::delete_particle_system(ParticleSystemID pid) {
    signal_particle_system_destroyed_(pid);

    particle_system(pid)->destroy_children();

    ParticleSystemManager::manager_delete(pid);
}

//=============== SPRITES ===================

SpriteID Stage::new_sprite() {
    SpriteID s = SpriteManager::manager_new(this);
    sprite(s)->set_parent(this);
    signal_sprite_created_(s);
    return s;
}

SpriteID Stage::new_sprite_from_file(const unicode& filename, uint32_t frame_width, uint32_t frame_height, uint32_t margin, uint32_t spacing, std::pair<uint32_t, uint32_t> padding) {
    SpriteID s = new_sprite();
    TextureID t = assets->new_texture_from_file(
        filename,
        TextureFlags(MIPMAP_GENERATE_NONE, TEXTURE_WRAP_CLAMP_TO_EDGE, TEXTURE_FILTER_NEAREST)
    );
    try {
        sprite(s)->set_spritesheet(t, frame_width, frame_height, margin, spacing, padding);
    } catch(...) {
        delete_sprite(s);
        throw;
    }

    return s;
}

SpritePtr Stage::sprite(SpriteID s) {
    return SpriteManager::manager_get(s).lock();
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
    LightID lid = LightManager::manager_new(this);
    light(lid)->set_type(type);

    // If this is a new directional light, make sure we set a decent
    // direction to start with so that users can get a decent
    // effect without doing anything
    if(type == LIGHT_TYPE_DIRECTIONAL) {
        light(lid)->set_direction(kglt::Vec3(-1, -0.5, 0).normalized());
    }

    light(lid)->set_parent(this);
    signal_light_created_(lid);
    return lid;
}

LightID Stage::new_light(MoveableObject &parent, LightType type) {
    LightID lid = LightManager::manager_new(this);

    {
        auto l = light(lid);
        l->set_type(type);
        l->set_parent(&parent);
    }

    signal_light_created_(lid);

    return lid;
}

LightPtr Stage::light(LightID light_id) {
    return LightManager::manager_get(light_id).lock();
}

void Stage::delete_light(LightID light_id) {
    signal_light_destroyed_(light_id);
    light(light_id)->destroy_children();
    LightManager::manager_delete(light_id);
}

void Stage::host_camera(CameraID c) {
    if(window->camera(c)->has_proxy()) {
        //Destroy any existing proxy
        window->camera(c)->proxy().stage->evict_camera(c);
    }

    //Create a camera proxy for the camera ID
    CameraProxyManager::manager_new(c, this);

    camera(c)->set_parent(this);
}

void Stage::evict_camera(CameraID c) {
    //Delete the camera proxy
    CameraProxyManager::manager_delete(c);
}

ProtectedPtr<CameraProxy> Stage::camera(CameraID c) {
    return ProtectedPtr<CameraProxy>(CameraProxyManager::manager_get(c));
}

void Stage::set_partitioner(AvailablePartitioner partitioner) {
    /*
     * FIXME: Calling this twice will probably break because signals aren't disconnected!
     */

    switch(partitioner) {
        case PARTITIONER_NULL:
            partitioner_ = Partitioner::ptr(new NullPartitioner(this));
        break;
        case PARTITIONER_OCTREE:
            partitioner_ = Partitioner::ptr(new OctreePartitioner(this));
        break;
        default: {
            throw std::logic_error("Invalid partitioner type specified");
        }
    }

    assert(partitioner_);

    //Keep the partitioner updated with new meshes and lights
    signal_actor_created().connect(std::bind(&Partitioner::add_actor, partitioner_.get(), std::placeholders::_1));
    signal_actor_destroyed().connect(std::bind(&Partitioner::remove_actor, partitioner_.get(), std::placeholders::_1));

    signal_geom_created().connect(std::bind(&Partitioner::add_geom, partitioner_, std::placeholders::_1));
    signal_geom_destroyed().connect(std::bind(&Partitioner::remove_geom, partitioner_, std::placeholders::_1));

    signal_light_created().connect(std::bind(&Partitioner::add_light, partitioner_.get(), std::placeholders::_1));
    signal_light_destroyed().connect(std::bind(&Partitioner::remove_light, partitioner_.get(), std::placeholders::_1));

    signal_particle_system_created().connect(std::bind(&Partitioner::add_particle_system, partitioner_.get(), std::placeholders::_1));
    signal_particle_system_destroyed().connect(std::bind(&Partitioner::remove_particle_system, partitioner_.get(), std::placeholders::_1));
}

void Stage::update(double dt) {

}

void Stage::on_actor_created(ActorID actor_id) {
    auto act = actor(actor_id);

    act->each([](uint32_t i, SubActor* actor) {

    });
}

void Stage::on_actor_destroyed(ActorID actor_id) {

}

}
