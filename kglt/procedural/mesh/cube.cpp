#include "../../resource_manager.h"
#include "cube.h"

namespace kglt {
namespace procedural {
namespace mesh {

void box(ProtectedPtr<Mesh> mesh, float width, float height, float depth) {
    mesh->clear();

    float rx = width * 0.5f;
    float ry = height * 0.5f;
    float rz = depth * 0.5f;

    SubMeshIndex sm = mesh->new_submesh(
        mesh->resource_manager().new_material_from_file(mesh->resource_manager().default_material_filename()),
        MESH_ARRANGEMENT_TRIANGLES,
        true
    );

    //front and back
    for(int32_t z: { -1, 1 }) {
        for(int32_t i = 0; i < 2; ++i) {
            uint32_t count = mesh->shared_data().count();

            mesh->shared_data().position(-1 * rx, -1 * ry, z * rz);
            mesh->shared_data().tex_coord0(0, 0);
            mesh->shared_data().tex_coord1(0, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, 0, z);
            mesh->shared_data().move_next();

            mesh->shared_data().position( 1 * rx, -1 * ry, z * rz);
            mesh->shared_data().tex_coord0(1, 0);
            mesh->shared_data().tex_coord1(1, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, 0, z);
            mesh->shared_data().move_next();

            mesh->shared_data().position( 1 * rx,  1 * ry, z * rz);
            mesh->shared_data().tex_coord0(1, 1);
            mesh->shared_data().tex_coord1(1, 1);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, 0, z);
            mesh->shared_data().move_next();

            mesh->shared_data().position(-1 * rx,  1 * ry, z * rz);
            mesh->shared_data().tex_coord0(0, 1);
            mesh->shared_data().tex_coord1(0, 1);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, 0, z);
            mesh->shared_data().move_next();

            if(z > 0) {
                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 1);
                mesh->submesh(sm).index_data().index(count + 2);

                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 2);
                mesh->submesh(sm).index_data().index(count + 3);
            } else {
                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 2);
                mesh->submesh(sm).index_data().index(count + 1);

                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 3);
                mesh->submesh(sm).index_data().index(count + 2);
            }
        }
    }

    //left and right
    for(int32_t x: { -1, 1 }) {
        for(int32_t i = 0; i < 2; ++i) {
            uint32_t count = mesh->shared_data().count();

            mesh->shared_data().position( x * rx, -1 * ry, -1 * rz);
            mesh->shared_data().tex_coord0(0, 0);
            mesh->shared_data().tex_coord1(0, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(x, 0, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( x * rx,  1 * ry, -1 * rz);
            mesh->shared_data().tex_coord0(1, 0);
            mesh->shared_data().tex_coord1(1, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(x, 0, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( x * rx,  1 * ry, 1 * rz);
            mesh->shared_data().tex_coord0(1, 1);
            mesh->shared_data().tex_coord1(1, 1);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(x, 0, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position(x * rx, -1 * ry, 1 * rz);
            mesh->shared_data().tex_coord0(0, 1);
            mesh->shared_data().tex_coord1(0, 1);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(x, 0, 0);
            mesh->shared_data().move_next();

            if(x > 0) {
                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 1);
                mesh->submesh(sm).index_data().index(count + 2);

                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 2);
                mesh->submesh(sm).index_data().index(count + 3);
            } else {
                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 2);
                mesh->submesh(sm).index_data().index(count + 1);

                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 3);
                mesh->submesh(sm).index_data().index(count + 2);
            }
        }
    }

    //top and bottom
    for(int32_t y: { -1, 1 }) {
        for(int32_t i = 0; i < 2; ++i) {
            uint32_t count = mesh->shared_data().count();

            mesh->shared_data().position( 1 * rx, y * ry, -1 * rz);
            mesh->shared_data().tex_coord0(0, 0);
            mesh->shared_data().tex_coord1(0, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, y, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( -1 * rx,  y * ry, -1 * rz);
            mesh->shared_data().tex_coord0(1, 0);
            mesh->shared_data().tex_coord1(1, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, y, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( -1 * rx,  y * ry, 1 * rz);
            mesh->shared_data().tex_coord0(1, 1);
            mesh->shared_data().tex_coord1(1, 1);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, y, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( 1 * rx, y * ry, 1 * rz);
            mesh->shared_data().tex_coord0(0, 1);
            mesh->shared_data().tex_coord1(0, 1);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, y, 0);
            mesh->shared_data().move_next();

            if(y > 0) {
                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 1);
                mesh->submesh(sm).index_data().index(count + 2);

                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 2);
                mesh->submesh(sm).index_data().index(count + 3);
            } else {
                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 2);
                mesh->submesh(sm).index_data().index(count + 1);

                mesh->submesh(sm).index_data().index(count);
                mesh->submesh(sm).index_data().index(count + 3);
                mesh->submesh(sm).index_data().index(count + 2);
            }
        }
    }

    mesh->shared_data().done();
    mesh->submesh(sm).index_data().done();
}

void cube(ProtectedPtr<Mesh> mesh, float width) {
    box(mesh, width, width, width);
}

}
}
}

