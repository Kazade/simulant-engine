#include <unittest++/UnitTest++.h>

#include "kglt/kglt.h"

TEST(test_user_data_works) {
	kglt::Window window;
	
	kglt::MeshID mid = window.scene().new_mesh();
	kglt::Mesh& mesh = window.scene().mesh(mid);
	
    CHECK(mesh.id() != 0); //Make sure we set an id for the mesh
    CHECK(mesh.uuid() != 0); //Make sure we set a unique ID for the object
	CHECK(!mesh.has_user_data());
	mesh.set_user_data((int)0xDEADBEEF);
	CHECK(mesh.has_user_data());
	CHECK_EQUAL(0xDEADBEEF, mesh.user_data<int>());

    window.scene().delete_mesh(mid);

    CHECK(!window.scene().has_mesh(mid));
}

TEST(test_deleting_meshes_deletes_children) {
    kglt::Window window;
    kglt::Scene& scene = window.scene();

    kglt::MeshID mid = scene.new_mesh(); //Create the root mesh
    kglt::MeshID cid1 = scene.new_mesh(&scene.mesh(mid)); //Create a child
    kglt::MeshID cid2 = scene.new_mesh(&scene.mesh(cid1)); //Crete a child of the child

    CHECK_EQUAL(1, scene.mesh(mid).child_count());
    CHECK_EQUAL(1, scene.mesh(cid1).child_count());
    CHECK_EQUAL(0, scene.mesh(cid2).child_count());

    scene.delete_mesh(mid);
    CHECK(!scene.has_mesh(mid));
    CHECK(!scene.has_mesh(cid1));
    CHECK(!scene.has_mesh(cid2));
}

TEST(test_procedural_rectangle_outline) {
	kglt::Window window;
	
	kglt::MeshID mid = window.scene().new_mesh();
	kglt::Mesh& mesh = window.scene().mesh(mid);
	
	CHECK_EQUAL(0, mesh.vertices().size());
	kglt::procedural::mesh::rectangle_outline(mesh, 1.0, 1.0);
	
	CHECK_EQUAL(kglt::MESH_ARRANGEMENT_LINE_STRIP, mesh.arrangement());
	CHECK_EQUAL(5, mesh.vertices().size());
}
