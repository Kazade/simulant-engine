#include "interpreter.h"

#include "../kglt.h"

namespace kglt {

void export_lua_api(lua_State* state) {
    luabind::module(state) [
        luabind::class_<SubSceneID>("SubSceneID")
            .def(luabind::constructor<int>())
            .property("value", &SubSceneID::value)
    ];

    luabind::module(state) [
        luabind::class_<MaterialID>("MaterialID")
            .def(luabind::constructor<int>())
            .property("value", &MaterialID::value)
    ];

    luabind::module(state) [
        luabind::class_<EntityID>("EntityID")
            .def(luabind::constructor<int>())
            .property("value", &EntityID::value)
    ];

    luabind::module(state) [
        luabind::class_<MeshID>("MeshID")
            .def(luabind::constructor<int>())
            .property("value", &MeshID::value)
    ];

    luabind::module(state) [
        luabind::class_<kmVec3>("kmVec3")
            .property("x", &Vec3::x)
            .property("y", &Vec3::y)
            .property("z", &Vec3::z),

        luabind::class_<Vec3, kmVec3>("Vec3")
            .def(luabind::constructor<float, float, float>()),

        luabind::class_<kmQuaternion>("kmQuaternion")
            .property("x", &kmQuaternion::x)
            .property("y", &kmQuaternion::y)
            .property("z", &kmQuaternion::z)
            .property("w", &kmQuaternion::w)
    ];

    luabind::module(state) [
        luabind::class_<WindowBase>("WindowBase")
            .def("set_title", &WindowBase::set_title)
            .def("quit", &WindowBase::stop_running)
            .property("scene", &WindowBase::scene)
            .property("width", &WindowBase::width)
            .property("height", &WindowBase::height)
            .property("total_time", &WindowBase::total_time)
            .property("delta_time", &WindowBase::delta_time)
    ];

    luabind::module(state) [
        luabind::class_<ResourceManager>("ResourceManager")
            .def("mesh", (Mesh&(ResourceManager::*)(MeshID))&ResourceManager::mesh)
            .property("mesh_count", &ResourceManager::mesh_count)
    ];

    luabind::module(state) [
        luabind::class_<SubScene, luabind::bases<ResourceManager> >("SubScene")
            .def("new_entity", (EntityID(SubScene::*)())&SubScene::new_entity)
            .def("new_entity_from_mesh", (EntityID(SubScene::*)(MeshID))&SubScene::new_entity)
            .def("entity", &SubScene::entity)
            .def("delete_entity", &SubScene::delete_entity)
            .property("scene", &SubScene::scene)
            .property("entity_count", &SubScene::entity_count)
            .property("light_count", &SubScene::light_count)
            .property("id", &SubScene::id)
    ];

    /*
        kmMat4 absolute_transformation();
    */

    luabind::module(state) [
        luabind::class_<Object>("Object")
            .def("move_to", (void(Object::*)(float, float, float))&Object::move_to)
            .def("move_forward", &Object::move_forward)
            .def("rotate_x", &Object::rotate_x)
            .def("rotate_y", &Object::rotate_y)
            .def("rotate_z", &Object::rotate_z)
            .def("rotate_to", (void(Object::*)(float, float, float, float))&Object::rotate_to)
            .def("lock_rotation", &Object::lock_rotation)
            .def("unlock_rotation", &Object::unlock_rotation)
            .def("lock_position", &Object::lock_position)
            .def("unlock_position", &Object::unlock_position)
            .property("uuid", &Object::uuid)
            .property("position", &Object::position)
            .property("absolute_position", &Object::absolute_position)
            .property("rotation", &Object::rotation)
            .property("absolute_rotation", &Object::absolute_rotation)
    ];

    luabind::module(state) [
        luabind::class_<Entity, Object>("Entity")
            .property("id", &Entity::id)
    ];

    luabind::module(state)[
        luabind::class_<Mesh>("Mesh")
            .property("id", &Mesh::id)
    ];

    luabind::module(state) [
        luabind::class_<Scene, luabind::bases<ResourceManager> >("Scene")
            .def("update", &Scene::update)
            .def("new_subscene", &Scene::new_subscene)
            .def("subscene", (SubScene&(Scene::*)(SubSceneID))&Scene::subscene)
            .def("delete_subscene", &Scene::delete_subscene)
            .property("subscene_count", &Scene::subscene_count)
            .property("default_subscene", (SubScene&(Scene::*)())&Scene::subscene)
            .property("default_material_id", &Scene::default_material_id)
            .property("pipeline", &Scene::pipeline)
    ];

    luabind::module(state) [
        luabind::class_<GeomFactory>("GeomFactory")
            .def("new_line", &GeomFactory::new_line)
    ];
}

}
