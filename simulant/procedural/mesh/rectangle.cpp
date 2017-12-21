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

#include "../../resource_manager.h"
#include "../../window.h"
#include "rectangle.h"

namespace smlt {
namespace procedural {
namespace mesh {

SubMesh* new_rectangle_submesh(
    MeshPtr& mesh, float width, float height,
    float x_offset, float y_offset, float z_offset, MaterialID material_id) {

    //Create a submesh
    SubMesh* submesh = mesh->new_submesh_with_material(
        "rectangle",
        (material_id) ? material_id : mesh->resource_manager().clone_default_material(),
        MESH_ARRANGEMENT_TRIANGLES
    );

    auto offset = mesh->vertex_data->count();
    submesh->vertex_data->move_to_end();

    //Build some shared vertex data
    submesh->vertex_data->position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    submesh->vertex_data->diffuse(smlt::Colour::WHITE);
    submesh->vertex_data->tex_coord0(0.0, 0.0);
    submesh->vertex_data->tex_coord1(0.0, 0.0);
    submesh->vertex_data->tex_coord2(0.0, 0.0);
    submesh->vertex_data->tex_coord3(0.0, 0.0);
    submesh->vertex_data->normal(0, 0, 1);
    submesh->vertex_data->move_next();

    submesh->vertex_data->position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    submesh->vertex_data->diffuse(smlt::Colour::WHITE);
    submesh->vertex_data->tex_coord0(1.0, 0.0);
    submesh->vertex_data->tex_coord1(1.0, 0.0);
    submesh->vertex_data->tex_coord2(1.0, 0.0);
    submesh->vertex_data->tex_coord3(1.0, 0.0);
    submesh->vertex_data->normal(0, 0, 1);
    submesh->vertex_data->move_next();

    submesh->vertex_data->position(x_offset + (width / 2.0),  y_offset + (height / 2.0), z_offset);
    submesh->vertex_data->diffuse(smlt::Colour::WHITE);
    submesh->vertex_data->tex_coord0(1.0, 1.0);
    submesh->vertex_data->tex_coord1(1.0, 1.0);
    submesh->vertex_data->tex_coord2(1.0, 1.0);
    submesh->vertex_data->tex_coord3(1.0, 1.0);
    submesh->vertex_data->normal(0, 0, 1);
    submesh->vertex_data->move_next();

    submesh->vertex_data->position(x_offset + (-width / 2.0),  y_offset + (height / 2.0), z_offset);
    submesh->vertex_data->diffuse(smlt::Colour::WHITE);
    submesh->vertex_data->tex_coord0(0.0, 1.0);
    submesh->vertex_data->tex_coord1(0.0, 1.0);
    submesh->vertex_data->tex_coord2(0.0, 1.0);
    submesh->vertex_data->tex_coord3(0.0, 1.0);
    submesh->vertex_data->normal(0, 0, 1);
    submesh->vertex_data->move_next();
    submesh->vertex_data->done();

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
        float width, float height,
        float x_offset, float y_offset, float z_offset,
        bool clear, smlt::MaterialID material) {

    if(clear) {
        mesh->clear();
    }

    uint16_t offset = mesh->vertex_data->count();

    mesh->vertex_data->move_to_end();

    //Build some shared vertex data
    mesh->vertex_data->position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 0.0);
    mesh->vertex_data->tex_coord1(0.0, 0.0);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 0.0);
    mesh->vertex_data->tex_coord1(1.0, 0.0);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0),  y_offset + (height / 2.0), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 1.0);
    mesh->vertex_data->tex_coord1(1.0, 1.0);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (-width / 2.0),  y_offset + (height / 2.0), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 1.0);
    mesh->vertex_data->tex_coord1(0.0, 1.0);
    mesh->vertex_data->normal(0, 0, 1);
    mesh->vertex_data->move_next();
    mesh->vertex_data->done();

    if(!material) {
        material = mesh->resource_manager().clone_default_material();
    }

    //Create a submesh that uses the shared data
    SubMesh* submesh = mesh->new_submesh_with_material(
        "rectangle",
        material,
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
        float width, float height,
        float x_offset, float y_offset, float z_offset,
        bool clear, smlt::MaterialID material) {
    if(clear) {
        mesh->clear();
    }

    uint16_t offset = mesh->vertex_data->count();

    mesh->vertex_data->position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 0.0);
    mesh->vertex_data->tex_coord1(0.0, 0.0);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 0.0);
    mesh->vertex_data->tex_coord1(1.0, 0.0);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (width / 2.0), y_offset + (height / 2.0), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(1.0, 1.0);
    mesh->vertex_data->tex_coord1(1.0, 1.0);
    mesh->vertex_data->move_next();

    mesh->vertex_data->position(x_offset + (-width / 2.0), y_offset + (height / 2.0), z_offset);
    mesh->vertex_data->diffuse(smlt::Colour::WHITE);
    mesh->vertex_data->tex_coord0(0.0, 1.0);
    mesh->vertex_data->tex_coord1(0.0, 1.0);
    mesh->vertex_data->move_next();
    mesh->vertex_data->done();
    
    if(!material) {
        material = mesh->resource_manager().clone_default_material();
    }
    SubMesh* submesh = mesh->new_submesh_with_material(
        "rectangle_outline",
        material,
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
