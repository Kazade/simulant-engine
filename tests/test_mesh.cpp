#include <unittest++/UnitTest++.h>

#include "kglt/kglt.h"

TEST(test_user_data_works) {
	kglt::Window window;
	
	kglt::MeshID mid = window.scene().new_mesh();
	kglt::Mesh& mesh = window.scene().mesh(mid);
	
	
	CHECK(!mesh.has_user_data());
	mesh.set_user_data((int)0xDEADBEEF);
	CHECK(mesh.has_user_data());
	CHECK_EQUAL(0xDEADBEEF, mesh.user_data<int>());
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
