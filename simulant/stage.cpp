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
#include "window.h"
#include "partitioner.h"
#include "debug.h"
#include "viewport.h"
#include "application.h"

#include "nodes/actor.h"
#include "nodes/light.h"
#include "nodes/camera.h"
#include "nodes/sprite.h"
#include "nodes/particle_system.h"
#include "nodes/geom.h"
#include "nodes/camera.h"
#include "nodes/mesh_instancer.h"

#include "nodes/ui/ui_manager.h"

#include "loader.h"

#include "partitioners/null_partitioner.h"
#include "partitioners/spatial_hash.h"
#include "partitioners/frustum_partitioner.h"

namespace smlt {

// Apparently this is the colour of a high noon sun (colour temp 5400 - 255, 255, 251)
const Colour DEFAULT_LIGHT_COLOUR = Colour(1.0, 1.0, 251.0 / 255.0, 1.0);

static inline SoundDriver* sound_driver() {
    return get_app()->sound_driver.get();
}

Stage::Stage(StageManager *parent, StageNodePool *node_pool, AvailablePartitioner partitioner):
    ContainerNode(this, STAGE_NODE_TYPE_STAGE),
    node_pool_(node_pool),
    asset_manager_(LocalAssetManager::create(get_app()->shared_assets.get())),
    ui_(new ui::UIManager(this, node_pool_)),
    geom_manager_(new GeomManager(node_pool_)),
    sky_manager_(new SkyManager(this, node_pool_)),
    sprite_manager_(new SpriteManager(this, node_pool_)),
    mesh_instancer_manager_(new MeshInstancerManager(node_pool_)),
    actor_manager_(new ActorManager(node_pool_)),
    particle_system_manager_(new ParticleSystemManager(node_pool_)),
    light_manager_(new LightManager(node_pool_)),
    camera_manager_(new CameraManager(node_pool_)) {

    set_partitioner(partitioner);
}

Stage::~Stage() {
    // Composite things first
    sprite_manager_.reset();
    sky_manager_.reset();
    ui_.reset();

    // Then core objects
    geom_manager_.reset();
    camera_manager_.reset();
    light_manager_.reset();
    particle_system_manager_.reset();
    actor_manager_.reset();

    // Finally assets
    asset_manager_.reset();
}

bool Stage::init() {
    S_DEBUG("Initializing stage {0}", id());
    return true;
}

void Stage::clean_up() {
    ui_.reset();
    debug_.reset();

    //Recurse through the tree, destroying all children
    auto pair = each_descendent();

    for(auto it = pair.begin(); it != pair.end(); ++it) {
        auto& stage_node = *it;
        stage_node.destroy();
    }

    light_manager_->clear();
    actor_manager_->clear();
    camera_manager_->clear();
    geom_manager_->clear();

    asset_manager_->destroy_all();
    asset_manager_.reset();

    S_DEBUG("Stage {0} destroyed", id());
}

ActorPtr Stage::new_actor() {
    S_DEBUG("Creating actor");

    using namespace std::placeholders;

    auto a = actor_manager_->make(this, get_app()->sound_driver);

    a->set_parent(this);

    /* Whenever the actor moves, we need to tell the stage's partitioner */
    a->signal_bounds_updated().connect([this, a](const AABB& new_bounds) {
        this->partitioner->update_stage_node(a, new_bounds);
    });

    //Tell everyone about the new actor
    signal_stage_node_created_(a, STAGE_NODE_TYPE_ACTOR);
    a->signal_destroyed().connect([=]() {
        signal_stage_node_destroyed_(a, STAGE_NODE_TYPE_ACTOR);
    });

    S_DEBUG("Actor created: {0}", a->id());
    return a;
}

ActorPtr Stage::new_actor_with_name(const std::string& name) {
    auto a = new_actor();
    a->set_name(name);
    return a;
}

ActorPtr Stage::new_actor_with_mesh(MeshPtr mesh) {
    using namespace std::placeholders;

    auto a = actor_manager_->make(this, sound_driver());
    a->set_parent(this);

    /* Whenever the actor moves, we need to tell the stage's partitioner */
    a->signal_bounds_updated().connect([this, a](const AABB& new_bounds) {
        this->partitioner->update_stage_node(a, new_bounds);
    });

    // Tell everyone about the new actor
    // It's important this happens *before* we set the mesh that will
    // trigger a bounds update.
    signal_stage_node_created_(a, STAGE_NODE_TYPE_ACTOR);

    a->signal_destroyed().connect([=]() {
        signal_stage_node_destroyed_(a, STAGE_NODE_TYPE_ACTOR);
    });

    //If a mesh was specified, set it
    if(mesh) {
        a->set_mesh(mesh);
    }

    return a;
}

ActorPtr Stage::new_actor_with_name_and_mesh(const std::string& name, MeshPtr mesh) {
    auto a = new_actor_with_mesh(mesh);
    a->set_name(name);
    return a;
}

ActorPtr Stage::new_actor_with_parent(StageNodePtr parent) {
    auto a = new_actor();
    a->set_parent(parent);
    return a;
}

ActorPtr Stage::new_actor_with_parent_and_mesh(StageNodePtr parent, const MeshPtr& mesh) {
    auto a = new_actor_with_mesh(mesh);
    a->set_parent(parent);
    return a;
}

bool Stage::has_actor(StageNodeID m) const {
    return actor_manager_->contains(m);
}

ActorPtr Stage::actor(StageNodeID e) {
    return actor_manager_->get(e);
}

ActorPtr Stage::actor(StageNodeID e) const {
    return actor_manager_->get(e);
}

void Stage::destroy_actor(StageNodeID e) {
    auto a = actor(e);
    if(a) {
        a->destroy();
    }
}

std::size_t Stage::actor_count() const {
    return actor_manager_->size();
}


MeshInstancerPtr Stage::new_mesh_instancer(MeshPtr mesh) {
    auto instance = mesh_instancer_manager_->make(
        this, sound_driver(), mesh
    );

    instance->set_parent(this);

    instance->signal_bounds_updated().connect([this, instance](const AABB& new_bounds) {
        this->partitioner->update_stage_node(instance, new_bounds);
    });

    signal_stage_node_created_(instance, STAGE_NODE_TYPE_MESH_INSTANCER);
    instance->signal_destroyed().connect([=]() {
        signal_stage_node_destroyed_(instance, STAGE_NODE_TYPE_MESH_INSTANCER);
    });

    return instance;
}

bool Stage::destroy_mesh_instancer(StageNodeID mid) {
    auto instance = mesh_instancer(mid);
    if(instance) {
        instance->destroy();
        return true;
    }

    return false;
}

MeshInstancerPtr Stage::mesh_instancer(StageNodeID mid) {
    return mesh_instancer_manager_->get(mid);
}

std::size_t Stage::mesh_instancer_count() const {
    return mesh_instancer_manager_->size();
}

bool Stage::has_mesh_instancer(StageNodeID mid) const {
    return mesh_instancer_manager_->contains(mid);
}

CameraPtr Stage::new_camera() {
    auto new_camera = camera_manager_->make(this, sound_driver());
    new_camera->set_parent(this);
    signal_stage_node_created_(new_camera, STAGE_NODE_TYPE_CAMERA);
    new_camera->signal_destroyed().connect([=]() {
        signal_stage_node_destroyed_(new_camera, STAGE_NODE_TYPE_CAMERA);
    });

    return new_camera;
}

CameraPtr Stage::new_camera_with_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    /*
     *  Instantiates a camera with an orthographic projection. If both left and right are zero then they default to 0 and window.width()
     *  respectively. If top and bottom are zero, then they default to window.height() and 0 respectively. So top left is 0,0
     */
    auto new_cam = new_camera();

    if(!left && !right) {
        right = get_app()->window->width();
    }

    if(!bottom && !top) {
        top = get_app()->window->height();
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
        0, get_app()->window->width(),
        0, get_app()->window->height(), -1, 1
    );
}

CameraPtr Stage::camera(StageNodeID c) {
    return camera_manager_->get(c);
}

void Stage::destroy_camera(StageNodeID cid) {
    auto c = camera(cid);
    if(c) {
        c->destroy();
    }
}

uint32_t Stage::camera_count() const {
    return camera_manager_->size();
}

bool Stage::has_camera(StageNodeID id) const {
    return camera_manager_->contains(id);
}

void Stage::destroy_all_cameras() {
    camera_manager_->destroy_all();
}

//=============== GEOMS =====================

GeomPtr Stage::new_geom_with_mesh(MeshPtr mesh, const GeomCullerOptions& culler_options) {
    return new_geom_with_mesh_at_position(mesh, smlt::Vec3(), smlt::Quaternion(), smlt::Vec3(1, 1, 1), culler_options);
}

GeomPtr Stage::geom(const StageNodeID gid) const {
    return geom_manager_->get(gid);
}

GeomPtr Stage::new_geom_with_mesh_at_position(MeshPtr mid, const Vec3& position, const Quaternion& rotation, const Vec3& scale, const GeomCullerOptions& culler_options) {
    auto gid = geom_manager_->make(
        this, sound_driver(),
        mid, position, rotation, scale,
        culler_options
    );
    gid->set_parent(this);

    signal_stage_node_created_(gid, STAGE_NODE_TYPE_GEOM);
    gid->signal_destroyed().connect([=]() {
        signal_stage_node_destroyed_(gid, STAGE_NODE_TYPE_GEOM);
    });

    return gid;
}

bool Stage::has_geom(StageNodeID geom_id) const {
    return geom_manager_->contains(geom_id);
}

void Stage::destroy_geom(StageNodeID geom_id) {
    auto g = geom(geom_id);
    if(g) {
        g->destroy();
    }
}

std::size_t Stage::geom_count() const {
    return geom_manager_->size();
}

//=============== PARTICLES =================

ParticleSystemPtr Stage::new_particle_system(ParticleScriptPtr particle_script) {
    auto p = particle_system_manager_->make(
        this,
        sound_driver(),
        particle_script
    );

    /* Whenever the particle system moves, we need to tell the stage's partitioner */
    p->signal_bounds_updated().connect([this, p](const AABB& new_bounds) {
        this->partitioner->update_stage_node(p, new_bounds);
    });

    p->set_parent(this);

    signal_stage_node_created_(p, STAGE_NODE_TYPE_PARTICLE_SYSTEM);
    p->signal_destroyed().connect([=]() {
        signal_stage_node_destroyed_(p, STAGE_NODE_TYPE_PARTICLE_SYSTEM);
    });

    return p;
}


ParticleSystemPtr Stage::new_particle_system_with_parent(ParticleScriptPtr particle_script, StageNode* parent) {
    auto p = new_particle_system(particle_script);
    p->set_parent(parent);
    return p;
}

ParticleSystemPtr Stage::particle_system(StageNodeID pid) {
    return particle_system_manager_->get(pid);
}

bool Stage::has_particle_system(StageNodeID pid) const {
    return particle_system_manager_->contains(pid);
}

void Stage::destroy_particle_system(StageNodeID pid) {
    auto p = particle_system(pid);
    if(p) {
        p->destroy();
    }
}

std::size_t Stage::particle_system_count() const { return particle_system_manager_->size(); }

LightPtr Stage::new_light_as_directional(const Vec3& direction, const smlt::Colour& colour) {
    auto light = light_manager_->make(this);

    light->set_type(smlt::LIGHT_TYPE_DIRECTIONAL);
    light->set_direction(direction.normalized());
    light->set_diffuse(colour);
    light->set_parent(this);

    /* Whenever the light moves, we need to tell the stage's partitioner */
    light->signal_bounds_updated().connect([this, light](const AABB& new_bounds) {
        this->partitioner->update_stage_node(light, new_bounds);
    });

    signal_stage_node_created_(light, STAGE_NODE_TYPE_LIGHT);
    light->signal_destroyed().connect([=]() {
        signal_stage_node_destroyed_(light, STAGE_NODE_TYPE_LIGHT);
    });
    return light;
}

LightPtr Stage::new_light_as_point(const Vec3& position, const smlt::Colour& colour) {
    auto light = light_manager_->make(this);

    light->set_type(smlt::LIGHT_TYPE_POINT);
    light->move_to(position);

    light->set_diffuse(colour);
    light->set_parent(this);

    /* Whenever the light moves, we need to tell the stage's partitioner */
    light->signal_bounds_updated().connect([this, light](const AABB& new_bounds) {
        this->partitioner->update_stage_node(light, new_bounds);
    });

    signal_stage_node_created_(light, STAGE_NODE_TYPE_LIGHT);
    light->signal_destroyed().connect([=]() {
        signal_stage_node_destroyed_(light, STAGE_NODE_TYPE_LIGHT);
    });

    return light;
}

LightPtr Stage::light(StageNodeID light_id) {
    return light_manager_->get(light_id);
}

void Stage::destroy_light(StageNodeID light_id) {
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
    signal_stage_node_created().connect(
        std::bind(&Partitioner::add_stage_node, partitioner_.get(),
        std::placeholders::_1
    ));

    signal_stage_node_destroyed().connect(
                [=](StageNode* node, StageNodeType) {
        partitioner_->remove_stage_node(node);
    });
}

void Stage::update(float dt) {
    asset_manager_->update(dt);

    if(debug_) {
        debug_->update(dt);
    }

    /* Regularly trim the node pool size */
    node_pool_->shrink_to_fit();
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
    actor_manager_->destroy(id);
}

void Stage::destroy_object(Light* object) {
    auto id = object->id();
    light_manager_->destroy(id);
}

void Stage::destroy_object(Camera* object) {
    auto id = object->id();
    camera_manager_->destroy(id);
}

void Stage::destroy_object(Geom* object) {
    auto id = object->id();
    geom_manager_->destroy(id);
}

void Stage::destroy_object(ParticleSystem* object) {
    auto id = object->id();
    particle_system_manager_->destroy(id);
}

void Stage::destroy_object(MeshInstancer* object) {
    auto id = object->id();
    mesh_instancer_manager_->destroy(id);
}

void Stage::destroy_object_immediately(Actor* object) {
    // Only send the signal if we didn't already
    auto id = object->id();
    actor_manager_->destroy_immediately(id);
}

void Stage::destroy_object_immediately(Light* object) {
    auto id = object->id();
    light_manager_->destroy_immediately(id);
}

void Stage::destroy_object_immediately(Camera* object) {
    auto id = object->id();
    camera_manager_->destroy_immediately(id);
}

void Stage::destroy_object_immediately(Geom* object) {
    auto id = object->id();
    geom_manager_->destroy_immediately(id);
}

void Stage::destroy_object_immediately(ParticleSystem* object) {
    auto id = object->id();
    particle_system_manager_->destroy_immediately(id);
}

void Stage::destroy_object_immediately(MeshInstancer *object) {
    auto id = object->id();
    mesh_instancer_manager_->destroy_immediately(id);
}

void Stage::on_actor_created(StageNodeID actor_id) {
    _S_UNUSED(actor_id);
}

void Stage::on_actor_destroyed(StageNodeID actor_id) {
    _S_UNUSED(actor_id);
}

void Stage::clean_up_dead_objects() {
    // We need to tell the UI manager to
    // clean itself up, as it's not updateable
    // and so can't easily do it itself.
    ui_->manager_->clean_up();

    actor_manager_->clean_up();
    light_manager_->clean_up();
    geom_manager_->clean_up();
    particle_system_manager_->clean_up();
    camera_manager_->clean_up();
    mesh_instancer_manager_->clean_up();
}

}
