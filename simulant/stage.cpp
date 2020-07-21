//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "stage.h"
#include "core.h"
#include "partitioner.h"
#include "nodes/actor.h"
#include "nodes/light.h"
#include "nodes/camera.h"
#include "debug.h"
#include "viewport.h"
#include "nodes/sprite.h"
#include "nodes/particle_system.h"
#include "nodes/geom.h"
#include "nodes/camera.h"

#include "nodes/ui/ui_manager.h"

#include "loader.h"
#include "partitioners/null_partitioner.h"
#include "partitioners/spatial_hash.h"
#include "partitioners/frustum_partitioner.h"

namespace smlt {

// Apparently this is the colour of a high noon sun (colour temp 5400 - 255, 255, 251)
const Colour DEFAULT_LIGHT_COLOUR = Colour(1.0, 1.0, 251.0 / 255.0, 1.0);

Stage::Stage(Core *parent, AvailablePartitioner partitioner, uint32_t pool_size):
    CoreHolder(parent),
    TypedDestroyableObject<Stage, Core>(parent),
    ContainerNode(this, STAGE_NODE_TYPE_STAGE),
    node_pool_(std::make_shared<StageNodePool>(pool_size)),
    ui_(new ui::UIManager(this, node_pool_.get())),
    asset_manager_(AssetManager::create(parent, parent->shared_assets.get())),
    fog_(new FogSettings()),
    geom_manager_(new GeomManager(node_pool_.get())),
    sky_manager_(new SkyManager(parent, this, node_pool_.get())),
    sprite_manager_(new SpriteManager(parent, this, node_pool_.get())),
    actor_manager_(new ActorManager(node_pool_.get())),
    particle_system_manager_(new ParticleSystemManager(node_pool_.get())),
    light_manager_(new LightManager(node_pool_.get())),
    camera_manager_(new CameraManager(node_pool_.get())) {

    set_partitioner(partitioner);

    clean_up_signal_ = parent->signal_post_idle().connect(
        std::bind(&Stage::clean_up_dead_objects, this)
    );
}

Stage::~Stage() {
    sprite_manager_.reset();
    sky_manager_.reset();
}

bool Stage::init() {

    return true;
}

void Stage::clean_up() {
    clean_up_signal_.disconnect();

    ui_.reset();
    debug_.reset();

    //Recurse through the tree, destroying all children
    for(auto& stage_node: each_descendent()) {
        stage_node.destroy();
    }

    light_manager_->clear();
    actor_manager_->clear();
    camera_manager_->clear();
    geom_manager_->clear();

    L_DEBUG(_F("Stage {0} destroyed").format(id()));
}

ActorPtr Stage::new_actor() {
    using namespace std::placeholders;

    auto a = actor_manager_->make(this, core->_sound_driver());

    a->set_parent(this);

    auto id = a->id();
    /* Whenever the actor moves, we need to tell the stage's partitioner */
    a->signal_bounds_updated().connect([this, id](const AABB& new_bounds) {
        this->partitioner->update_actor(id, new_bounds);
    });

    //Tell everyone about the new actor
    signal_actor_created_(a->id());
    return a;
}

ActorPtr Stage::new_actor_with_name(const std::string& name) {
    auto a = new_actor();
    a->set_name(name);
    return a;
}

ActorPtr Stage::new_actor_with_mesh(MeshID mid) {
    using namespace std::placeholders;

    auto a = actor_manager_->make(this, core->_sound_driver());
    a->set_parent(this);

    auto id = a->id();
    /* Whenever the actor moves, we need to tell the stage's partitioner */
    a->signal_bounds_updated().connect([this, id](const AABB& new_bounds) {
        this->partitioner->update_actor(id, new_bounds);
    });

    // Tell everyone about the new actor
    // It's important this happens *before* we set the mesh that will
    // trigger a bounds update.
    signal_actor_created_(id);

    //If a mesh was specified, set it
    if(mid) {
        a->set_mesh(mid);
    }

    return a;
}

ActorPtr Stage::new_actor_with_name_and_mesh(const std::string& name, MeshID mid) {
    auto a = new_actor_with_mesh(mid);
    a->set_name(name);
    return a;
}

ActorPtr Stage::new_actor_with_parent(ActorID parent) {
    auto a = new_actor();
    a->set_parent(parent);
    return a;
}

ActorPtr Stage::new_actor_with_parent_and_mesh(SpriteID parent, MeshID mid) {
    auto a = new_actor_with_mesh(mid);
    a->set_parent(parent);
    return a;
}

ActorPtr Stage::new_actor_with_parent_and_mesh(ActorID parent, MeshID mid) {
    auto a = new_actor_with_mesh(mid);
    a->set_parent(parent);
    return a;
}

bool Stage::has_actor(ActorID m) const {
    return actor_manager_->contains(m);
}

ActorPtr Stage::actor(ActorID e) {
    return actor_manager_->get(e);
}

ActorPtr Stage::actor(ActorID e) const {
    return actor_manager_->get(e);
}

void Stage::destroy_actor(ActorID e) {
    auto a = actor(e);
    if(a) {
        a->destroy();
    }
}

std::size_t Stage::actor_count() const {
    return actor_manager_->size();
}

CameraPtr Stage::new_camera() {
    auto new_camera = camera_manager_->make(this);
    new_camera->set_parent(this);
    return new_camera;
}

CameraPtr Stage::new_camera_with_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    /*
     *  Instantiates a camera with an orthographic projection. If both left and right are zero then they default to 0 and window.width()
     *  respectively. If top and bottom are zero, then they default to window.height() and 0 respectively. So top left is 0,0
     */
    auto new_cam = new_camera();

    if(!left && !right) {
        right = core->width();
    }

    if(!bottom && !top) {
        top = core->height();
    }

    new_cam->set_orthographic_projection(left, right, bottom, top, near, far);

    return new_cam;
}

CameraPtr Stage::new_camera_for_viewport(const Viewport& vp) {
    float x, y, width, height;
    calculate_ratios_from_viewport(vp.type(), x, y, width, height);

    auto camera = new_camera();
    camera->set_perspective_projection(Degrees(45.0), width / height);

    return camera;
}

CameraPtr Stage::new_camera_for_ui() {
    return new_camera_with_orthographic_projection(
        0, core->width(), 0, core->height(), -1, 1
    );
}

CameraPtr Stage::camera(CameraID c) {
    return camera_manager_->get(c);
}

void Stage::destroy_camera(CameraID cid) {
    auto c = camera(cid);
    if(c) {
        c->destroy();
    }
}

uint32_t Stage::camera_count() const {
    return camera_manager_->size();
}

bool Stage::has_camera(CameraID id) const {
    return camera_manager_->contains(id);
}

void Stage::destroy_all_cameras() {
    camera_manager_->destroy_all();
}

//=============== GEOMS =====================

GeomPtr Stage::new_geom_with_mesh(MeshID mid, const GeomCullerOptions& culler_options) {
    return new_geom_with_mesh_at_position(mid, smlt::Vec3(), smlt::Quaternion(), culler_options);
}

GeomPtr Stage::geom(const GeomID gid) const {
    return geom_manager_->get(gid);
}

GeomPtr Stage::new_geom_with_mesh_at_position(MeshID mid, const Vec3& position, const Quaternion& rotation, const GeomCullerOptions& culler_options) {
    auto gid = geom_manager_->make(
        this, core->_sound_driver(),
        mid, position, rotation,
        culler_options
    );
    gid->set_parent(this);

    signal_geom_created_(gid->id());

    return gid;
}

bool Stage::has_geom(GeomID geom_id) const {
    return geom_manager_->contains(geom_id);
}

void Stage::destroy_geom(GeomID geom_id) {
    auto g = geom(geom_id);
    if(g) {
        g->destroy();
    }
}

std::size_t Stage::geom_count() const {
    return geom_manager_->size();
}

//=============== PARTICLES =================

ParticleSystemPtr Stage::new_particle_system(ParticleScriptID particle_script) {
    auto p = particle_system_manager_->make(
        this,
        core->_sound_driver(),
        assets->particle_script(particle_script)
    );

    auto new_id = p->id();

    /* Whenever the particle system moves, we need to tell the stage's partitioner */
    p->signal_bounds_updated().connect([this, new_id](const AABB& new_bounds) {
        this->partitioner->update_particle_system(new_id, new_bounds);
    });

    p->set_parent(this);

    signal_particle_system_created_(new_id);
    return p;
}


ParticleSystemPtr Stage::new_particle_system_with_parent(ParticleScriptID particle_script, ActorID parent) {
    auto p = new_particle_system(particle_script);
    p->set_parent(parent);
    return p;
}

ParticleSystemPtr Stage::particle_system(ParticleSystemID pid) {
    return particle_system_manager_->get(pid);
}

bool Stage::has_particle_system(ParticleSystemID pid) const {
    return particle_system_manager_->contains(pid);
}

void Stage::destroy_particle_system(ParticleSystemID pid) {
    auto p = particle_system(pid);
    if(p) {
        p->destroy();
    }
}

std::size_t Stage::particle_system_count() const { return particle_system_manager_->size(); }

LightPtr Stage::new_light_as_directional(const Vec3& direction, const smlt::Colour& colour) {
    auto light = light_manager_->make(this);
    auto light_id = light->id();

    light->set_type(smlt::LIGHT_TYPE_DIRECTIONAL);
    light->set_direction(direction.normalized());
    light->set_diffuse(colour);
    light->set_parent(this);

    /* Whenever the light moves, we need to tell the stage's partitioner */
    light->signal_bounds_updated().connect([this, light_id](const AABB& new_bounds) {
        this->partitioner->update_light(light_id, new_bounds);
    });

    signal_light_created_(light_id);
    return light;
}

LightPtr Stage::new_light_as_point(const Vec3& position, const smlt::Colour& colour) {
    auto light = light_manager_->make(this);
    auto light_id = light->id();

    light->set_type(smlt::LIGHT_TYPE_POINT);
    light->move_to(position);

    light->set_diffuse(colour);
    light->set_parent(this);

    /* Whenever the light moves, we need to tell the stage's partitioner */
    light->signal_bounds_updated().connect([this, light_id](const AABB& new_bounds) {
        this->partitioner->update_light(light_id, new_bounds);
    });

    signal_light_created_(light_id);
    return light;
}

LightPtr Stage::light(LightID light_id) {
    return light_manager_->get(light_id);
}

void Stage::destroy_light(LightID light_id) {
    auto l = light(light_id);
    if(l) {
        l->destroy();
    }
}

std::size_t Stage::light_count() const { return light_manager_->size(); }

void Stage::set_partitioner(AvailablePartitioner partitioner) {
    /*
     * FIXME: Calling this twice will probably break because signals aren't disconnected!
     */

    switch(partitioner) {
        case PARTITIONER_NULL:
            partitioner_ = Partitioner::ptr(new NullPartitioner(this));
        break;
        case PARTITIONER_FRUSTUM:
            partitioner_ = std::make_shared<FrustumPartitioner>(this);
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
    asset_manager_->update(dt);

    if(debug_) {
        debug_->update(dt);
    }
}

Debug* Stage::enable_debug(bool v) {
    if(debug_ && !v) {
        debug_.reset();
    } else if(!debug_ && v) {
        debug_ = Debug::create(*this);
    }

    return debug_.get();
}

void Stage::destroy_object(Actor* object) {
    auto id = object->id();
    if(actor_manager_->destroy(id)) {
        signal_actor_destroyed_(id);
    }
}

void Stage::destroy_object(Light* object) {
    auto id = object->id();
    if(light_manager_->destroy(id)) {
        signal_light_destroyed_(id);
    }
}

void Stage::destroy_object(Camera* object) {
    auto id = object->id();
    if(camera_manager_->destroy(id)) {
        signal_camera_destroyed_(id);
    }
}

void Stage::destroy_object(Geom* object) {
    auto id = object->id();
    if(geom_manager_->destroy(id)) {
        signal_geom_destroyed_(id);
    }
}

void Stage::destroy_object(ParticleSystem* object) {
    auto id = object->id();
    if(particle_system_manager_->destroy(id)) {
        signal_particle_system_destroyed_(id);
    }
}

void Stage::destroy_object_immediately(Actor* object) {
    // Only send the signal if we didn't already
    auto id = object->id();
    if(actor_manager_->destroy_immediately(id)) {
        signal_actor_destroyed_(id);
    }
}

void Stage::destroy_object_immediately(Light* object) {
    auto id = object->id();
    if(light_manager_->destroy_immediately(id)) {
        signal_light_destroyed_(id);
    }
}

void Stage::destroy_object_immediately(Camera* object) {
    auto id = object->id();
    if(camera_manager_->destroy_immediately(id)) {
        signal_camera_destroyed_(id);
    }
}

void Stage::destroy_object_immediately(Geom* object) {
    auto id = object->id();
    if(geom_manager_->destroy_immediately(id)) {
        signal_geom_destroyed_(id);
    }
}

void Stage::destroy_object_immediately(ParticleSystem* object) {
    auto id = object->id();
    if(particle_system_manager_->destroy_immediately(id)) {
        signal_particle_system_destroyed_(id);
    }
}

void Stage::on_actor_created(ActorID actor_id) {
    _S_UNUSED(actor_id);
}

void Stage::on_actor_destroyed(ActorID actor_id) {
    _S_UNUSED(actor_id);
}

void Stage::clean_up_dead_objects() {
    actor_manager_->clean_up();
    light_manager_->clean_up();
    geom_manager_->clean_up();
    particle_system_manager_->clean_up();
    camera_manager_->clean_up();
}

}
