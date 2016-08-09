#include "../../resource_manager.h"
#include "cube.h"

namespace kglt {
namespace procedural {
namespace mesh {

void box(ProtectedPtr<Mesh> mesh, float width, float height, float depth, MeshStyle style) {
    mesh->clear();

    float rx = width * 0.5f;
    float ry = height * 0.5f;
    float rz = depth * 0.5f;

    std::vector<SubMesh*> submeshes;
    submeshes.resize(6);

    if(style == MESH_STYLE_SUBMESH_PER_FACE) {
        for(uint8_t i = 0; i < 6; ++i) {
            SubMeshID sm = mesh->new_submesh(MESH_ARRANGEMENT_TRIANGLES);
            submeshes[i] = mesh->submesh(sm);
        }
    } else {
        SubMeshID sm = mesh->new_submesh(MESH_ARRANGEMENT_TRIANGLES);

        for(uint8_t i = 0; i < 6; ++i) {
            submeshes[i] = mesh->submesh(sm);
        }
    }

    //front and back
    for(int32_t z: { -1, 1 }) {
        uint32_t count = mesh->shared_data->count();

        mesh->shared_data->position(-1 * rx, -1 * ry, z * rz);
        mesh->shared_data->tex_coord0(0, 0);
        mesh->shared_data->tex_coord1(0, 0);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(0, 0, z);
        mesh->shared_data->move_next();

        mesh->shared_data->position( 1 * rx, -1 * ry, z * rz);
        mesh->shared_data->tex_coord0(1, 0);
        mesh->shared_data->tex_coord1(1, 0);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(0, 0, z);
        mesh->shared_data->move_next();

        mesh->shared_data->position( 1 * rx,  1 * ry, z * rz);
        mesh->shared_data->tex_coord0(1, 1);
        mesh->shared_data->tex_coord1(1, 1);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(0, 0, z);
        mesh->shared_data->move_next();

        mesh->shared_data->position(-1 * rx,  1 * ry, z * rz);
        mesh->shared_data->tex_coord0(0, 1);
        mesh->shared_data->tex_coord1(0, 1);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(0, 0, z);
        mesh->shared_data->move_next();

        if(z > 0) {
            SubMesh* sm = submeshes[0];
            sm->index_data->index(count);
            sm->index_data->index(count + 1);
            sm->index_data->index(count + 2);

            sm->index_data->index(count);
            sm->index_data->index(count + 2);
            sm->index_data->index(count + 3);
        } else {
            SubMesh* sm = submeshes[1];
            sm->index_data->index(count);
            sm->index_data->index(count + 2);
            sm->index_data->index(count + 1);

            sm->index_data->index(count);
            sm->index_data->index(count + 3);
            sm->index_data->index(count + 2);
        }

    }

    //left and right
    for(int32_t x: { -1, 1 }) {
        uint32_t count = mesh->shared_data->count();

        mesh->shared_data->position( x * rx, -1 * ry, -1 * rz);
        mesh->shared_data->tex_coord0(0, 0);
        mesh->shared_data->tex_coord1(0, 0);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(x, 0, 0);
        mesh->shared_data->move_next();

        mesh->shared_data->position( x * rx,  1 * ry, -1 * rz);
        mesh->shared_data->tex_coord0(1, 0);
        mesh->shared_data->tex_coord1(1, 0);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(x, 0, 0);
        mesh->shared_data->move_next();

        mesh->shared_data->position( x * rx,  1 * ry, 1 * rz);
        mesh->shared_data->tex_coord0(1, 1);
        mesh->shared_data->tex_coord1(1, 1);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(x, 0, 0);
        mesh->shared_data->move_next();

        mesh->shared_data->position(x * rx, -1 * ry, 1 * rz);
        mesh->shared_data->tex_coord0(0, 1);
        mesh->shared_data->tex_coord1(0, 1);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(x, 0, 0);
        mesh->shared_data->move_next();

        if(x > 0) {
            SubMesh* sm = submeshes[2];
            sm->index_data->index(count);
            sm->index_data->index(count + 1);
            sm->index_data->index(count + 2);

            sm->index_data->index(count);
            sm->index_data->index(count + 2);
            sm->index_data->index(count + 3);
        } else {
            SubMesh* sm = submeshes[3];
            sm->index_data->index(count);
            sm->index_data->index(count + 2);
            sm->index_data->index(count + 1);

            sm->index_data->index(count);
            sm->index_data->index(count + 3);
            sm->index_data->index(count + 2);
        }

    }

    //top and bottom
    for(int32_t y: { -1, 1 }) {
        uint32_t count = mesh->shared_data->count();

        mesh->shared_data->position( 1 * rx, y * ry, -1 * rz);
        mesh->shared_data->tex_coord0(0, 0);
        mesh->shared_data->tex_coord1(0, 0);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(0, y, 0);
        mesh->shared_data->move_next();

        mesh->shared_data->position( -1 * rx,  y * ry, -1 * rz);
        mesh->shared_data->tex_coord0(1, 0);
        mesh->shared_data->tex_coord1(1, 0);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(0, y, 0);
        mesh->shared_data->move_next();

        mesh->shared_data->position( -1 * rx,  y * ry, 1 * rz);
        mesh->shared_data->tex_coord0(1, 1);
        mesh->shared_data->tex_coord1(1, 1);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(0, y, 0);
        mesh->shared_data->move_next();

        mesh->shared_data->position( 1 * rx, y * ry, 1 * rz);
        mesh->shared_data->tex_coord0(0, 1);
        mesh->shared_data->tex_coord1(0, 1);
        mesh->shared_data->diffuse(kglt::Colour::WHITE);
        mesh->shared_data->normal(0, y, 0);
        mesh->shared_data->move_next();

        if(y > 0) {
            SubMesh* sm = submeshes[4];
            sm->index_data->index(count);
            sm->index_data->index(count + 1);
            sm->index_data->index(count + 2);

            sm->index_data->index(count);
            sm->index_data->index(count + 2);
            sm->index_data->index(count + 3);
        } else {
            SubMesh* sm = submeshes[5];
            sm->index_data->index(count);
            sm->index_data->index(count + 2);
            sm->index_data->index(count + 1);

            sm->index_data->index(count);
            sm->index_data->index(count + 3);
            sm->index_data->index(count + 2);
        }

    }

    mesh->shared_data->done();
    for(auto sm: submeshes) {
        sm->index_data->done();
    }
}

void cube(ProtectedPtr<Mesh> mesh, float width, procedural::MeshStyle style) {
    box(mesh, width, width, width, style);
}

}
}
}

