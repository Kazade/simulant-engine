//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "stage.h"
#include "window.h"
#include "partitioner.h"
#include "nodes/actor.h"
#include "nodes/light.h"
#include "nodes/camera.h"
#include "debug.h"

#include "nodes/sprite.h"
#include "nodes/particle_system.h"
#include "nodes/geom.h"
#include "nodes/camera.h"

#include "nodes/ui/ui_manager.h"

#include "loader.h"
#include "partitioners/null_partitioner.h"
#include "partitioners/spatial_hash.h"
#include "renderers/batching/render_queue.h"

namespace smlt {

// Apparently this is the colour of a high noon sun (colour temp 5400 - 255, 255, 251)
const Colour DEFAULT_LIGHT_COLOUR = Colour(1.0, 1.0, 251.0 / 255.0, 1.0);

Stage::Stage(StageID id, Window *parent, AvailablePartitioner partitioner):
    WindowHolder(parent),
    StageNode(this),
    generic::Identifiable<StageID>(id),
    CameraManager(this),
    ui_(new ui::UIManager(this)),
    resource_manager_(ResourceManager::create(parent, parent->shared_assets.get())),
    ambient_light_(smlt::Colour::WHITE),
    geom_manager_(new GeomManager()),
    sky_manager_(new SkyManager(parent, this)),
    sprite_manager_(new SpriteManager(parent, this)) {

    set_partitioner(partitioner);
    render_queue_.reset(new batcher::RenderQueue(this, parent->renderer.get()));
}

Stage::~Stage() {

}

bool Stage::init() {    
    debug_ = Debug::create(*this);
    return true;
}

void Stage::cleanup() {    
    ui_.reset();

    //Recurse through the tree, destroying all children
    this->each_descendent_lf([](uint32_t, TreeNode* node) {
        StageNode* stage_node = static_cast<StageNode*>(node);
        stage_node->ask_owner_for_destruction();
    });

    LightManager::clear();
    ActorManager::clear();
    CameraManager::delete_all_cameras();
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

    ActorID result = ActorManager::make(this, window->_sound_driver());
    auto a = result.fetch();

    a->set_renderable_culling_mode(mode);
    a->set_parent(this);
    a->signal_subactor_material_changed().connect(
        std::bind(&Stage::on_subactor_material_changed, this, _1, _2, _3, _4)
    );

    /* Whenever the actor moves, we need to tell the stage's partitioner */
    a->signal_bounds_updated().connect([this, result](const AABB& new_bounds) {
        this->partitioner->update_actor(result, new_bounds);
    });

    //Tell everyone about the new actor
    signal_actor_created_(result);
    return result;
}

ActorID Stage::new_actor_with_mesh(MeshID mid, RenderableCullingMode mode) {
    using namespace std::placeholders;

    ActorID result = ActorManager::make(this, window->_sound_driver());
    auto a = result.fetch();

    a->set_renderable_culling_mode(mode);
    a->set_parent(this);
    a->signal_subactor_material_changed().connect(
        std::bind(&Stage::on_subactor_material_changed, this, _1, _2, _3, _4)
    );

    /* Whenever the actor moves, we need to tell the stage's partitioner */
    a->signal_bounds_updated().connect([this, result](const AABB& new_bounds) {
        this->partitioner->update_actor(result, new_bounds);
    });

    //If a mesh was specified, set it
    if(mid) {
        a->set_mesh(mid);
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
    return ActorManager::contains(m);
}

ActorPtr Stage::actor(ActorID e) {
    return ActorManager::get(e);
}

const ActorPtr Stage::actor(ActorID e) const {
    return ActorManager::get(e);
}

void Stage::delete_actor(ActorID e) {
    signal_actor_destroyed_(e);
    ActorManager::destroy(e);
}

//=============== GEOMS =====================

GeomID Stage::new_geom_with_mesh(MeshID mid) {
    auto gid = geom_manager_->make(this, window->_sound_driver(), mid);
    geom(gid)->set_parent(this);

    signal_geom_created_(gid);

    return gid;
}

GeomPtr Stage::geom(const GeomID gid) const {
    return geom_manager_->get(gid);
}

GeomID Stage::new_geom_with_mesh_at_position(MeshID mid, const Vec3& position, const Quaternion& rotation) {
    auto gid = geom_manager_->make(this, window->_sound_driver(), mid, position, rotation);
    geom(gid)->set_parent(this);

    signal_geom_created_(gid);

    return gid;
}

bool Stage::has_geom(GeomID geom_id) const {
    return geom_manager_->contains(geom_id);
}

void Stage::delete_geom(GeomID geom_id) {
    signal_geom_destroyed_(geom_id);

    geom_manager_->destroy(geom_id);
}

uint32_t Stage::geom_count() const {
    return geom_manager_->count();
}

//=============== PARTICLES =================

ParticleSystemID Stage::new_particle_system() {
    ParticleSystemID new_id = ParticleSystemManager::make(this, window->_sound_driver());

    auto p = new_id.fetch();

    /* Whenever the particle system moves, we need to tell the stage's partitioner */
    p->signal_bounds_updated().connect([this, new_id](const AABB& new_bounds) {
        this->partitioner->update_particle_system(new_id, new_bounds);
    });

    signal_particle_system_created_(new_id);
    return new_id;
}

ParticleSystemID Stage::new_particle_system_from_file(const unicode& filename, bool destroy_on_completion) {
    ParticleSystemID new_id = new_particle_system();

    auto ps = particle_system(new_id)->shared_from_this();
    ps->set_parent(this);
    ps->set_destroy_on_completion(destroy_on_completion);

    window->loader_for(filename)->into(ps);

    return new_id;
}

ParticleSystemID Stage::new_particle_system_with_parent_from_file(ActorID parent, const unicode& filename, bool destroy_on_completion) {
    ParticleSystemID new_id = new_particle_system();

    auto ps = particle_system(new_id)->shared_from_this();
    ps->set_parent(parent);
    ps->set_destroy_on_completion(destroy_on_completion);

    window->loader_for(filename)->into(ps);

    return new_id;
}

ParticleSystemPtr Stage::particle_system(ParticleSystemID pid) {
    return ParticleSystemManager::get(pid).lock().get();
}

bool Stage::has_particle_system(ParticleSystemID pid) const {
    return ParticleSystemManager::contains(pid);
}

void Stage::delete_particle_system(ParticleSystemID pid) {
    signal_particle_system_destroyed_(pid);
    ParticleSystemManager::destroy(pid);
}

LightID Stage::new_light_as_directional(const Vec3& direction, const smlt::Colour& colour) {
    auto light = LightManager::make(this).fetch();
    auto light_id = light->id();

    light->set_type(smlt::LIGHT_TYPE_DIRECTIONAL);
    light->set_direction(direction.normalized());
    light->set_diffuse(colour);
    light->set_parent(this);

    /* Whenever the light moves, we need to tell the stage's partitioner */
    light->signal_bounds_updated().connect([this, light_id](const AABB& new_bounds) {
        this->partitioner->update_light(light_id, new_bounds);
    });

    signal_light_created_(light->id());
    return light->id();
}

LightID Stage::new_light_as_point(const Vec3& position, const smlt::Colour& colour) {
    auto light = LightManager::make(this).fetch();
    auto light_id = light->id();

    light->set_type(smlt::LIGHT_TYPE_POINT);
    light->move_to(position);

    light->set_diffuse(colour);
    light->set_parent(this);

    /* Whenever the light moves, we need to tell the stage's partitioner */
    light->signal_bounds_updated().connect([this, light_id](const AABB& new_bounds) {
        this->partitioner->update_light(light_id, new_bounds);
    });

    signal_light_created_(light->id());
    return light->id();
}

LightPtr Stage::light(LightID light_id) {
    return LightManager::get(light_id);
}

void Stage::delete_light(LightID light_id) {
    signal_light_destroyed_(light_id);
    LightManager::destroy(light_id);
}

void Stage::set_partitioner(AvailablePartitioner partitioner) {
    /*
     * FIXME: Calling this twice will probably break because signals aren't disconnected!
     */

    switch(partitioner) {
        case PARTITIONER_NULL:
            partitioner_ = Partitioner::ptr(new NullPartitioner(this));
        break;
        case PARTITIONER_HASH:
            partitioner_ = std::make_shared<SpatialHashPartitioner>(this);
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

void Stage::update(float dt) {
    resource_manager_->update(dt);
}

void Stage::on_actor_created(ActorID actor_id) {

}

void Stage::on_actor_destroyed(ActorID actor_id) {

}

}
