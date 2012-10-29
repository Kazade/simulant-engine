#include <unittest++/UnitTest++.h>

#include "kglt/kglt.h"

using namespace newmesh;

Mesh::ptr generate_test_mesh() {
    Mesh::ptr mesh(new Mesh());

    VertexData& data = mesh->shared_data();

    data.position(-1.0, -1.0, 0.0);
    data.move_next();

    data.position( 1.0, -1.0, 0.0);
    data.move_next();

    data.position( 1.0, 1.0, 0.0);
    data.move_next();

    data.position(-1.0, 1.0, 0.0);
    data.move_next();

    data.done();

    SubMesh& submesh = mesh->submesh(mesh->new_submesh(0));

    submesh.index_data().index(0);
    submesh.index_data().index(1);
    submesh.index_data().index(2);

    submesh.index_data().index(0);
    submesh.index_data().index(2);
    submesh.index_data().index(3);

    //Draw a line between the first two vertices
    SubMesh& submesh = mesh->submesh(mesh->new_submesh(0, MESH_ARRANGEMENT_LINES));
    submesh.index_data().index(0);
    submesh.index_data().index(1);
}

SUITE(test_mesh) {

TEST(basic_usage) {    
    Mesh::ptr mesh = generate_test_mesh();

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

    CHECK(2, mesh.submesh_count());
}

TEST(entity_from_mesh) {
    Mesh::ptr mesh = generate_test_mesh();

    Entity::ptr entity(new Entity());
    entity->set_mesh(mesh);

    //The entity's MeshID should match the mesh we set
    CHECK_EQUAL(mesh.id, entity->mesh());

    //The entity should report the same data as the mesh, the same subentity count
    //as well as the same shared vertex data
    CHECK_EQUAL(mesh->submesh_count(), entity->subentity_count());
    CHECK_EQUAL(mesh->shared_data(), entity->shared_data());

    //Likewise for subentities, they should just proxy to the submesh
    CHECK_EQUAL(mesh->submesh(0).material(), entity->subentity(0).material());
    CHECK_EQUAL(mesh->submesh(0).index_data(), entity->subentity(0).index_data());
    CHECK_EQUAL(mesh->submesh(0).vertex_data(), entity->subentity(0).vertex_data());

    //We should be able to override the material on a subentity though
    entity->subentity(0).override_material(1);

    CHECK_EQUAL(1, entity->subentity(0).material());
}

TEST(scene_methods) {
    kglt::Window::ptr window = kglt::Window::create();
    kglt::Scene& scene = window->scene();

    Mesh& mesh = scene().mesh(scene().new_mesh()); //Create a mesh
    Entity& entity = scene().entity(scene().new_entity(mesh.id));

    CHECK_EQUAL(mesh.id, entity.mesh());
}

}
