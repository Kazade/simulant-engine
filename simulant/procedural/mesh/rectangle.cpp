//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../../asset_manager.h"
#include "../../window.h"
#include "rectangle.h"

namespace smlt {
namespace procedural {
namespace mesh {

SubMesh* new_rectangle_submesh(
    MeshPtr mesh, smlt::MaterialPtr mat, float width, float height,
    float x_offset, float y_offset, float z_offset) {

    //Create a submesh
    SubMesh* submesh = mesh->new_submesh(
        "rectangle",
        mat,
        INDEX_TYPE_16_BIT,
        MESH_ARRANGEMENT_TRIANGLES
    );

    auto offset = mesh->vertex_data->count();
    mesh->vertex_data->move_to_end();

    //Build some shared vertex data
    mesh->vertex_data->position(x_offset + (-width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 0.0f);
    mesh->vertex_data->tex_coord1(0.0, 0.0f);
    mesh->vertex_data->tex_coord2(0.0, 0.0f);
    mesh->vertex_data->tex_coord3(0.0, 0.0f);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 0.0f);
    mesh->vertex_data->tex_coord1(1.0, 0.0f);
    mesh->vertex_data->tex_coord2(1.0, 0.0f);
    mesh->vertex_data->tex_coord3(1.0, 0.0f);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0f),  y_offset + (height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 1.0f);
    mesh->vertex_data->tex_coord1(1.0, 1.0f);
    mesh->vertex_data->tex_coord2(1.0, 1.0f);
    mesh->vertex_data->tex_coord3(1.0, 1.0f);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (-width / 2.0f),  y_offset + (height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 1.0f);
    mesh->vertex_data->tex_coord1(0.0, 1.0f);
    mesh->vertex_data->tex_coord2(0.0, 1.0f);
    mesh->vertex_data->tex_coord3(0.0, 1.0f);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();
    mesh->vertex_data->done();

    submesh->index_data->index(offset + 0);
    submesh->index_data->index(offset + 1);
    submesh->index_data->index(offset + 2);

    submesh->index_data->index(offset + 0);
    submesh->index_data->index(offset + 2);
    submesh->index_data->index(offset + 3);
    submesh->index_data->done();

    return submesh;
}

SubMesh *rectangle(
        MeshPtr mesh,
        MaterialPtr material,
        float width, float height,
        float x_offset, float y_offset, float z_offset,
        bool clear) {

    if(clear) {
        mesh->reset(mesh->vertex_data->vertex_specification());
    }

    uint16_t offset = mesh->vertex_data->count();

    mesh->vertex_data->move_to_end();

    //Build some shared vertex data
    mesh->vertex_data->position(x_offset + (-width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 0.0f);
    mesh->vertex_data->tex_coord1(0.0, 0.0f);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 0.0f);
    mesh->vertex_data->tex_coord1(1.0, 0.0f);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0f),  y_offset + (height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 1.0f);
    mesh->vertex_data->tex_coord1(1.0, 1.0f);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (-width / 2.0f),  y_offset + (height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 1.0f);
    mesh->vertex_data->tex_coord1(0.0, 1.0f);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();
    mesh->vertex_data->done();

    //Create a submesh that uses the shared data
    SubMesh* submesh = mesh->new_submesh(
        "rectangle",
        material,
        INDEX_TYPE_16_BIT,
        MESH_ARRANGEMENT_TRIANGLES
    );
    submesh->index_data->index(offset + 0);
    submesh->index_data->index(offset + 1);
    submesh->index_data->index(offset + 2);

    submesh->index_data->index(offset + 0);
    submesh->index_data->index(offset + 2);
    submesh->index_data->index(offset + 3);
    submesh->index_data->done();

    return submesh;
}

SubMesh* rectangle_outline(
    MeshPtr mesh,
    MaterialPtr material,
    float width, float height,
    float x_offset, float y_offset, float z_offset,
    bool clear) {

    if(clear) {
        mesh->reset(mesh->vertex_data->vertex_specification());
    }

    uint16_t offset = mesh->vertex_data->count();

    mesh->vertex_data->position(x_offset + (-width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 0.0f);
    mesh->vertex_data->tex_coord1(0.0, 0.0f);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0f), y_offset + (-height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 0.0f);
    mesh->vertex_data->tex_coord1(1.0, 0.0f);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0f), y_offset + (height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 1.0f);
    mesh->vertex_data->tex_coord1(1.0, 1.0f);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (-width / 2.0f), y_offset + (height / 2.0f), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 1.0f);
    mesh->vertex_data->tex_coord1(0.0, 1.0f);
    mesh->vertex_data->move_next();
    mesh->vertex_data->done();

    SubMesh* submesh = mesh->new_submesh(
        "rectangle_outline",
        material,
        INDEX_TYPE_16_BIT,
        MESH_ARRANGEMENT_LINE_STRIP
    );

    for(uint8_t i = 0; i < 4; ++i) {
        submesh->index_data->index(offset + i);
    }

    //Add the original point
    submesh->index_data->index(offset);
    submesh->index_data->done();

    return submesh;
}

}
}
}
