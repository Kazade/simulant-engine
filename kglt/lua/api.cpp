#include "interpreter.h"

#include "../kglt.h"

namespace kglt {

void export_lua_api(lua_State* state) {
    luabind::module(state) [
        luabind::class_<StageID>("StageID")
            .def(luabind::constructor<int>())
            .property("value", &StageID::value)
    ];

    luabind::module(state) [
        luabind::class_<MaterialID>("MaterialID")
            .def(luabind::constructor<int>())
            .property("value", &MaterialID::value)
    ];

    luabind::module(state) [
        luabind::class_<ActorID>("ActorID")
            .def(luabind::constructor<int>())
            .property("value", &ActorID::value)
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
            .property("scene", (Scene&(WindowBase::*)())&WindowBase::scene)
            .property("width", &WindowBase::width)
            .property("height", &WindowBase::height)
            .property("total_time", &WindowBase::total_time)
            .property("delta_time", &WindowBase::delta_time)
    ];

    luabind::module(state) [
        luabind::class_<ResourceManager>("ResourceManager")
//            .def("mesh", (Mesh&(ResourceManager::*)(MeshID))&ResourceManager::mesh)
            .property("mesh_count", &ResourceManager::mesh_count)
    ];

    luabind::module(state) [
        luabind::class_<Stage, luabind::bases<ResourceManager> >("Stage")
            .def("new_entity", (ActorID(Stage::*)())&Stage::new_entity)
            .def("new_entity_from_mesh", (ActorID(Stage::*)(MeshID))&Stage::new_entity)
            .def("entity", &Stage::entity)
            .def("delete_entity", &Stage::delete_entity)
            .property("scene", (Scene&(Stage::*)())&Stage::scene)
            .property("entity_count", &Stage::entity_count)
            .property("light_count", &Stage::light_count)
            .property("id", &Stage::id)
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
        luabind::class_<Actor, Object>("Actor")
            .property("id", &Actor::id)
    ];

    luabind::module(state)[
        luabind::class_<Mesh>("Mesh")
            .property("id", &Mesh::id)
    ];

    luabind::module(state) [
        luabind::class_<Scene, luabind::bases<ResourceManager> >("Scene")
            .def("update", &Scene::update)
            .def("new_stage", &Scene::new_stage)
            .def("stage", (Stage&(Scene::*)(StageID))&Scene::stage)
            .def("delete_stage", &Scene::delete_stage)
            .property("stage_count", &Scene::stage_count)
            .property("default_stage", (Stage&(Scene::*)())&Scene::stage)
            .property("default_material_id", &Scene::default_material_id)
            .property("pipeline", &Scene::pipeline)
    ];

    luabind::module(state) [
        luabind::class_<GeomFactory>("GeomFactory")
            .def("new_line", &GeomFactory::new_line)
    ];
}

}
