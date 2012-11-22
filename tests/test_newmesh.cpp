#include <unittest++/UnitTest++.h>

#include "kglt/kglt.h"
#include "kglt/entity.h"
#include "kglt/vertex_data.h"

using namespace kglt;

MeshID generate_test_mesh(kglt::Scene& scene) {
    kglt::MeshID mid = scene.new_mesh();
    kglt::Mesh& mesh = scene.mesh(mid);

    kglt::VertexData& data = mesh.shared_data();

    data.position(-1.0, -1.0, 0.0);
    data.move_next();

    data.position( 1.0, -1.0, 0.0);
    data.move_next();

    data.position( 1.0, 1.0, 0.0);
    data.move_next();

    data.position(-1.0, 1.0, 0.0);
    data.move_next();

    data.done();

    SubMesh& submesh = mesh.submesh(mesh.new_submesh(MaterialID()));

    submesh.index_data().index(0);
    submesh.index_data().index(1);
    submesh.index_data().index(2);

    submesh.index_data().index(0);
    submesh.index_data().index(2);
    submesh.index_data().index(3);
    submesh.index_data().done();

    //Draw a line between the first two vertices
    SubMesh& sm = mesh.submesh(mesh.new_submesh(MaterialID(), kglt::MESH_ARRANGEMENT_LINES));
    sm.index_data().index(0);
    sm.index_data().index(1);
    sm.index_data().done();

    kmVec3 expected_min, expected_max;
    kmVec3Fill(&expected_min, -1.0, -1.0, 0.0);
    kmVec3Fill(&expected_max, 1.0, -1.0, 0.0);

    CHECK(kmVec3AreEqual(&sm.bounds().min, &expected_min));
    CHECK(kmVec3AreEqual(&sm.bounds().max, &expected_max));

    return mid;
}

SUITE(test_mesh) {

TEST(basic_usage) {    
    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();
    Mesh& mesh = scene.mesh(generate_test_mesh(scene));

    kglt::VertexData& data = mesh.shared_data();

    CHECK(data.has_positions());
    CHECK(!data.has_normals());
    CHECK(!data.has_texcoord0());
    CHECK(!data.has_texcoord1());
    CHECK(!data.has_texcoord2());
    CHECK(!data.has_texcoord3());
    CHECK(!data.has_texcoord4());
    CHECK(!data.has_diffuse());
    CHECK(!data.has_specular());
    CHECK_EQUAL(4, data.count());

    CHECK_EQUAL(2, mesh.submesh_count());
}

TEST(entity_from_mesh) {
    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();

    Mesh& mesh = scene.mesh(generate_test_mesh(scene));

    kglt::Entity& entity = scene.entity(scene.new_entity());

    CHECK(!entity.has_mesh());

    entity.set_mesh(mesh.id());

    CHECK(entity.has_mesh());

    //The entity's MeshID should match the mesh we set
    CHECK(mesh.id() == entity.mesh());

    //The entity should report the same data as the mesh, the same subentity count
    //as well as the same shared vertex data
    CHECK_EQUAL(mesh.submesh_count(), entity.subentity_count());
    CHECK(mesh.shared_data().count() == entity.shared_data().count());

    //Likewise for subentities, they should just proxy to the submesh
    CHECK_EQUAL(mesh.submesh(0).material(), entity.subentity(0).material());
    CHECK(mesh.submesh(0).index_data() == entity.subentity(0).index_data());
    CHECK(mesh.submesh(0).vertex_data() == entity.subentity(0).vertex_data());

    //We should be able to override the material on a subentity though
    entity.subentity(0).override_material(MaterialID(1));

    CHECK_EQUAL(MaterialID(1), entity.subentity(0).material());
}

TEST(scene_methods) {
    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();

    Mesh& mesh = scene.mesh(scene.new_mesh()); //Create a mesh
    kglt::Entity& entity = scene.entity(scene.new_entity(mesh.id()));

    CHECK(mesh.id() == entity.mesh());
}

}
