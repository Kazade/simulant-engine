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

SceneImpl::SceneImpl(WindowBase* window):
    ResourceManagerImpl(window),
    default_texture_(0),
    default_material_(0) {

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

MaterialID SceneImpl::default_material_id() const {
    return default_material_->id();
}

TextureID SceneImpl::default_texture_id() const {
    return default_texture_->id();
}

unicode SceneImpl::default_material_filename() const {
    return window().resource_locator().locate_file("kglt/materials/multitexture_and_lighting.kglm");
}

void SceneImpl::initialize_defaults() {
    window().stage()->host_camera(window().default_camera_id());


    //FIXME: Should lock the default texture and material during construction!

    //Create the default blank texture
    default_texture_ = texture(new_texture()).__object;
    default_texture_->resize(1, 1);
    default_texture_->set_bpp(32);

    default_texture_->data()[0] = 255;
    default_texture_->data()[1] = 255;
    default_texture_->data()[2] = 255;
    default_texture_->data()[3] = 255;
    default_texture_->upload();

    //Maintain ref-count
    default_material_ = material(new_material_from_file(default_material_filename())).__object;

    //Set the default material's first texture to the default (white) texture
    default_material_->technique().pass(0).set_texture_unit(0, default_texture_->id());
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
