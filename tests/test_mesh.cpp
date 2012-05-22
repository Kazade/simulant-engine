#include <unittest++/UnitTest++.h>

#include "kglt/kglt.h"

TEST(test_user_data_works) {
	kglt::Window window;
	
	kglt::MeshID mid = window.scene().new_mesh();
	kglt::Mesh& mesh = window.scene().mesh(mid);
	
	CHECK_EQUAL((void*)NULL, mesh.user_data());
	mesh.set_user_data((int*)0xDEADBEEF);
	CHECK_EQUAL((void*)0xDEADBEEF, mesh.user_data());
}

TEST(test_procedural_rectangle_outline) {
	kglt::Window window;
	
	kglt::MeshID mid = window.scene().new_mesh();
	kglt::Mesh& mesh = window.scene().mesh(mid);
	
	CHECK_EQUAL(0, mesh.vertices().size());
	kglt::procedural::mesh::rectangle_outline(mesh, 1.0, 1.0);
	
	CHECK_EQUAL(kglt::MESH_ARRANGEMENT_LINE_STRIP, mesh.arrangement());
	CHECK_EQUAL(4, mesh.vertices().size());
}
