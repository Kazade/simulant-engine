//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../../asset_manager.h"
#include "cube.h"

namespace smlt {
namespace procedural {
namespace mesh {

void box(MeshPtr mesh, float width, float height, float depth, MeshStyle style) {
    mesh->reset(VertexSpecification::DEFAULT);

    static const std::map<uint8_t, std::string> SUBMESH_NAMES = {
        {0, "front"},
        {1, "back"},
        {2, "left"},
        {3, "right"},
        {4, "top"},
        {5, "bottom"},
    };

    float rx = width * 0.5f;
    float ry = height * 0.5f;
    float rz = depth * 0.5f;

    std::vector<SubMesh*> submeshes;
    submeshes.resize(6);

    if(style == MESH_STYLE_SUBMESH_PER_FACE) {
        for(uint8_t i = 0; i < 6; ++i) {
            submeshes[i] = mesh->new_submesh(SUBMESH_NAMES.at(i), MESH_ARRANGEMENT_TRIANGLES);
        }
    } else {
        SubMesh* sm = mesh->new_submesh("box", MESH_ARRANGEMENT_TRIANGLES);

        for(uint8_t i = 0; i < 6; ++i) {
            submeshes[i] = sm;
        }
    }

    //front and back
    for(int32_t z: { -1, 1 }) {
        uint32_t count = mesh->vertex_data->count();

        mesh->vertex_data->position(-1 * rx, -1 * ry, z * rz);
        mesh->vertex_data->tex_coord0((z > 0) ? 0 : 1, 0);
        mesh->vertex_data->tex_coord1((z > 0) ? 0 : 1, 0);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(0, 0, z);
        mesh->vertex_data->move_next();

        mesh->vertex_data->position( 1 * rx, -1 * ry, z * rz);
        mesh->vertex_data->tex_coord0((z > 0) ? 1 : 0, 0);
        mesh->vertex_data->tex_coord1((z > 0) ? 1 : 0, 0);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(0, 0, z);
        mesh->vertex_data->move_next();

        mesh->vertex_data->position( 1 * rx,  1 * ry, z * rz);
        mesh->vertex_data->tex_coord0((z > 0) ? 1 : 0, 1);
        mesh->vertex_data->tex_coord1((z > 0) ? 1 : 0, 1);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(0, 0, z);
        mesh->vertex_data->move_next();

        mesh->vertex_data->position(-1 * rx,  1 * ry, z * rz);
        mesh->vertex_data->tex_coord0((z > 0) ? 0 : 1, 1);
        mesh->vertex_data->tex_coord1((z > 0) ? 0 : 1, 1);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(0, 0, z);
        mesh->vertex_data->move_next();

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
        uint32_t count = mesh->vertex_data->count();

        mesh->vertex_data->position( x * rx, -1 * ry, -1 * rz);
        mesh->vertex_data->tex_coord0((x < 0) ? 0 : 1, 0);
        mesh->vertex_data->tex_coord1((x < 0) ? 0 : 1, 0);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(x, 0, 0);
        mesh->vertex_data->move_next();

        mesh->vertex_data->position( x * rx,  1 * ry, -1 * rz);
        mesh->vertex_data->tex_coord0((x < 0) ? 0 : 1, 1);
        mesh->vertex_data->tex_coord1((x < 0) ? 0 : 1, 1);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(x, 0, 0);
        mesh->vertex_data->move_next();

        mesh->vertex_data->position( x * rx,  1 * ry, 1 * rz);
        mesh->vertex_data->tex_coord0((x < 0) ? 1 : 0, 1);
        mesh->vertex_data->tex_coord1((x < 0) ? 1 : 0, 1);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(x, 0, 0);
        mesh->vertex_data->move_next();

        mesh->vertex_data->position(x * rx, -1 * ry, 1 * rz);
        mesh->vertex_data->tex_coord0((x < 0) ? 1 : 0, 0);
        mesh->vertex_data->tex_coord1((x < 0) ? 1 : 0, 0);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(x, 0, 0);
        mesh->vertex_data->move_next();

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
        uint32_t count = mesh->vertex_data->count();

        mesh->vertex_data->position( 1 * rx, y * ry, -1 * rz);
        mesh->vertex_data->tex_coord0((y > 0) ? 1 : 0, 1);
        mesh->vertex_data->tex_coord1((y > 0) ? 1 : 0, 1);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(0, y, 0);
        mesh->vertex_data->move_next();

        mesh->vertex_data->position( -1 * rx,  y * ry, -1 * rz);
        mesh->vertex_data->tex_coord0((y > 0) ? 0 : 1, 1);
        mesh->vertex_data->tex_coord1((y > 0) ? 0 : 1, 1);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(0, y, 0);
        mesh->vertex_data->move_next();

        mesh->vertex_data->position( -1 * rx,  y * ry, 1 * rz);
        mesh->vertex_data->tex_coord0((y > 0) ? 0 : 1, 0);
        mesh->vertex_data->tex_coord1((y > 0) ? 0 : 1, 0);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(0, y, 0);
        mesh->vertex_data->move_next();

        mesh->vertex_data->position( 1 * rx, y * ry, 1 * rz);
        mesh->vertex_data->tex_coord0((y > 0) ? 1 : 0, 0);
        mesh->vertex_data->tex_coord1((y > 0) ? 1 : 0, 0);
        mesh->vertex_data->diffuse(smlt::Colour::WHITE);
        mesh->vertex_data->normal(0, y, 0);
        mesh->vertex_data->move_next();

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

    mesh->vertex_data->done();
    for(auto sm: submeshes) {
        sm->index_data->done();
    }
}

void cube(MeshPtr mesh, float width, procedural::MeshStyle style) {
    box(mesh, width, width, width, style);
}

}
}
}

