#include <unittest++/UnitTest++.h>

#include "kglt/kglt.h"

SUITE(test_mesh) {

TEST(basic_usage) {
    using namespace newmesh;

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

    CHECK(2, mesh.submesh_count());


}

}
