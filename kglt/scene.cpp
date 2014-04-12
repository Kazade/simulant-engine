#include "utils/glcompat.h"
#include "utils/ownable.h"
#include "background.h"

#include "render_sequence.h"
#include "scene.h"
#include "renderer.h"
#include "loader.h"
#include "stage.h"
#include "ui_stage.h"
#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"
#include "physics/physics_engine.h"
#include "shaders/default_shaders.h"
#include "window_base.h"
#include "camera.h"

namespace kglt {

SceneImpl::SceneImpl(WindowBase* window) {

}

SceneImpl::~SceneImpl() {

}

void SceneImpl::enable_physics(std::shared_ptr<PhysicsEngine> engine) {
    physics_engine_ = engine;
}

PhysicsEnginePtr SceneImpl::physics() {
    if(!physics_engine_) {
        throw std::logic_error("Tried to access the physics engine when one has not been enabled");
    }
    return physics_engine_;
}

const bool SceneImpl::has_physics_engine() const {
    return bool(physics_engine_);
}

void SceneImpl::initialize_defaults() {

}


bool SceneImpl::init() {
    return true;
}

void SceneImpl::update(double dt) {
    if(has_physics_engine()) {
        physics()->step(dt);
    }
}

}
