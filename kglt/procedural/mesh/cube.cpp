#include "../../resource_manager.h"
#include "../../scene.h"

#include "cube.h"

namespace kglt {
namespace procedural {
namespace mesh {

void cube(ProtectedPtr<Mesh> mesh, float width) {
    mesh->clear();

    float r = width * 0.5f;

    SubMeshIndex sm = mesh->new_submesh(
        mesh->scene().clone_default_material(),
        MESH_ARRANGEMENT_TRIANGLES,
        true
    );

    //front and back
    for(int32_t z: { -1, 1 }) {
        for(int32_t i = 0; i < 2; ++i) {
            uint32_t count = mesh->shared_data().count();

            mesh->shared_data().position(-1 * r, -1 * r, z * r);
            mesh->shared_data().tex_coord0(0, 0);
            mesh->shared_data().tex_coord1(0, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, 0, z);
            mesh->shared_data().move_next();

            mesh->shared_data().position( 1 * r, -1 * r, z * r);
            mesh->shared_data().tex_coord0(1, 0);
            mesh->shared_data().tex_coord1(1, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, 0, z);
            mesh->shared_data().move_next();

            mesh->shared_data().position( 1 * r,  1 * r, z * r);
            mesh->shared_data().tex_coord0(1, 1);
            mesh->shared_data().tex_coord1(1, 1);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, 0, z);
            mesh->shared_data().move_next();

            mesh->shared_data().position(-1 * r,  1 * r, z * r);
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

            mesh->shared_data().position( x * r, -1 * r, -1 * r);
            mesh->shared_data().tex_coord0(0, 0);
            mesh->shared_data().tex_coord1(0, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(x, 0, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( x * r,  1 * r, -1 * r);
            mesh->shared_data().tex_coord0(1, 0);
            mesh->shared_data().tex_coord1(1, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(x, 0, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( x * r,  1 * r, 1 * r);
            mesh->shared_data().tex_coord0(1, 1);
            mesh->shared_data().tex_coord1(1, 1);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(x, 0, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position(x * r, -1 * r, 1 * r);
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

            mesh->shared_data().position( 1 * r, y * r, -1 * r);
            mesh->shared_data().tex_coord0(0, 0);
            mesh->shared_data().tex_coord1(0, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, y, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( -1 * r,  y * r, -1 * r);
            mesh->shared_data().tex_coord0(1, 0);
            mesh->shared_data().tex_coord1(1, 0);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, y, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( -1 * r,  y * r, 1 * r);
            mesh->shared_data().tex_coord0(1, 1);
            mesh->shared_data().tex_coord1(1, 1);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(0, y, 0);
            mesh->shared_data().move_next();

            mesh->shared_data().position( 1 * r, y * r, 1 * r);
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

}
}
}

