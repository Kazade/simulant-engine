#include <UnitTest++.h>

#include "kglt/kglt.h"

TEST(test_user_data_works) {
    kglt::Window::ptr window = kglt::Window::create();
    kglt::SubScene& scene = window->scene().subscene();
	
    kglt::EntityID mid = scene.new_entity();
    kglt::Entity& entity = scene.entity(mid);
	
    CHECK(entity.id() != 0); //Make sure we set an id for the mesh
    CHECK(entity.uuid() != 0); //Make sure we set a unique ID for the object
    CHECK(!entity.exists("data"));
    entity.stash((int)0xDEADBEEF, "data");
    CHECK(entity.exists("data"));
    CHECK_EQUAL(0xDEADBEEF, entity.get<int>("data"));

    scene.delete_entity(mid);

    CHECK(!scene.has_entity(mid));
}

TEST(test_deleting_entities_deletes_children) {
    kglt::Window::ptr window = kglt::Window::create();
    kglt::SubScene& scene = window->scene().subscene();

    kglt::EntityID mid = scene.new_entity(); //Create the root mesh
    kglt::EntityID cid1 = scene.new_entity(scene.entity(mid)); //Create a child
    kglt::EntityID cid2 = scene.new_entity(scene.entity(cid1)); //Create a child of the child

    CHECK_EQUAL(1, scene.entity(mid).child_count());
    CHECK_EQUAL(1, scene.entity(cid1).child_count());
    CHECK_EQUAL(0, scene.entity(cid2).child_count());

    scene.delete_entity(mid);
    CHECK(!scene.has_entity(mid));
    CHECK(!scene.has_entity(cid1));
    CHECK(!scene.has_entity(cid2));
}

TEST(test_procedural_rectangle_outline) {
    kglt::Window::ptr window = kglt::Window::create();
    kglt::SubScene& scene = window->scene().subscene();
	
    kglt::MeshID mid = scene.new_mesh();
    kglt::Mesh& mesh = scene.mesh(mid);
	
    CHECK_EQUAL(0, mesh.shared_data().count());
    kglt::SubMeshIndex idx = kglt::procedural::mesh::rectangle_outline(mesh, 1.0, 1.0);
	
    CHECK_EQUAL(kglt::MESH_ARRANGEMENT_LINE_STRIP, mesh.submesh(idx).arrangement());
    CHECK_EQUAL(4, mesh.shared_data().count());
    CHECK_EQUAL(5, mesh.submesh(idx).index_data().count());
}
