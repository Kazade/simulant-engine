#include <GLee.h>

#include "scene.h"
#include "renderer.h"
#include "camera.h"
#include "pipeline.h"
#include "procedural/geom_factory.h"
#include "loader.h"
#include "subscene.h"
#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"

#include "shaders/default_shaders.h"
#include "window_base.h"

namespace kglt {

Scene::Scene(WindowBase* window):
    SceneBase(window, nullptr),
    default_texture_(0),
    default_material_(0),
    pipeline_(new Pipeline(*this)),
    geom_factory_(new GeomFactory(*this)){

}

Scene::~Scene() {
    //TODO: Log the unfreed resources (textures, meshes, materials etc.)
}

void Scene::initialize_defaults() {
    default_subscene_ = new_subscene(kglt::PARTITIONER_NULL);

    //Create a default stage for the default subscene with it's default camera
    pipeline_->add_stage(default_subscene_, subscene().camera().id());

    //Create the default blank texture
    default_texture_ = new_texture();
    TexturePtr tex = texture(default_texture_).lock();
    tex->resize(1, 1);
    tex->set_bpp(32);

    tex->data()[0] = 255;
    tex->data()[1] = 255;
    tex->data()[2] = 255;
    tex->data()[3] = 255;
    tex->upload();

    default_material_ = new_material_from_file("kglt/materials/multitexture_and_lighting.kglm");

    //Set the default material's first texture to the default (white) texture
    material(default_material_).lock()->technique().pass(0).set_texture_unit(0, default_texture_);
}

SubSceneID Scene::new_subscene(AvailablePartitioner partitioner) {
    SubScene& ss = subscene(SubSceneManager::manager_new());

    switch(partitioner) {
        case PARTITIONER_NULL:
        ss.set_partitioner(Partitioner::ptr(new NullPartitioner(ss)));
        break;
        case PARTITIONER_OCTREE:
        ss.set_partitioner(Partitioner::ptr(new OctreePartitioner(ss)));
        break;
        default: {
            delete_subscene(ss.id());
            throw std::logic_error("Invalid partitioner type specified");
        }
    }

    //All subscenes should have at least one camera
    ss.new_camera();

    return ss.id();
}

uint32_t Scene::subscene_count() const {
    return SubSceneManager::manager_count();
}

SubScene& Scene::subscene(SubSceneID s) {
    if(s == DefaultSubSceneID) {
        return SubSceneManager::manager_get(default_subscene_);
    }

    return SubSceneManager::manager_get(s);
}

void Scene::delete_subscene(SubSceneID s) {
    SubScene& ss = subscene(s);
    ss.destroy_children();
    SubSceneManager::manager_delete(s);
}

bool Scene::init() {
    assert(glGetError() == GL_NO_ERROR);

    initialize_defaults();

    return true;
}

void Scene::update(double dt) {
    //Update the subscenes
    SubSceneManager::apply_func_to_objects(std::bind(&Object::update, std::tr1::placeholders::_1, dt));
}

void Scene::render() {
    pipeline_->run();
}

}
