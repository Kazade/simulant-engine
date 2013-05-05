#include <GLee.h>

#include "scene.h"
#include "renderer.h"
#include "camera.h"
#include "pipeline.h"
#include "loader.h"
#include "subscene.h"
#include "partitioners/null_partitioner.h"
#include "partitioners/octree_partitioner.h"

#include "shaders/default_shaders.h"
#include "window_base.h"

namespace kglt {

void Scene::do_lua_export(lua_State &state) {
    luabind::module(&state) [
        luabind::class_<Scene>("Scene")
            .def("update", &Scene::update)
            .def("new_subscene", &Scene::new_subscene)
            .def("subscene", (SubScene&(Scene::*)(SubSceneID))&Scene::subscene)
            .def("default_subscene", (SubScene&(Scene::*)())&Scene::subscene)
            .def("delete_subscene", &Scene::delete_subscene)
            .property("default_material_id", &Scene::default_material_id)
            .property("pipeline", &Scene::pipeline)
    ];
}

Scene::Scene(WindowBase* window):
    SceneBase(window, nullptr),
    default_texture_(0),
    default_material_(0),
    pipeline_(new Pipeline(*this)) {

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
    Texture& tex = texture(default_texture_);
    tex.resize(1, 1);
    tex.set_bpp(32);

    tex.data()[0] = 255;
    tex.data()[1] = 255;
    tex.data()[2] = 255;
    tex.data()[3] = 255;
    tex.upload();

    default_material_ = new_material();
    Material& mat = material(default_material_);
    this->window().loader_for("kglt/materials/multitexture_and_lighting.kglm")->into(mat);

    //Set the default material's first texture to the default (white) texture
    mat.technique().pass(0).set_texture_unit(0, tex.id());
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
    update_materials(dt);

    //Update the subscenes
    SubSceneManager::apply_func_to_objects(std::bind(&Object::update, std::tr1::placeholders::_1, dt));
}

void Scene::render() {
    pipeline_->run();
}

}
