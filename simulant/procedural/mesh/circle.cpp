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

#include "circle.h"
#include "../../asset_manager.h"

namespace smlt {
namespace procedural {
namespace mesh {

SubMesh* circle(smlt::MeshPtr mesh, smlt::MaterialPtr mat, float diameter, int32_t point_count, float x_offset, float y_offset, float z_offset) {
    float radius = diameter * 0.5f;

    SubMesh* submesh = mesh->new_submesh(
        "circle",
        mat,
        MESH_ARRANGEMENT_TRIANGLE_FAN
    );

    auto offset = mesh->vertex_data->count();

    auto& vdata = mesh->vertex_data;
    auto& idata = submesh->index_data;

    vdata->move_to_end();

    for(uint16_t i = 0; i < point_count; ++i) {
        //Build some shared vertex data

        float rads = 2 * PI * i / point_count;

        float x = radius * std::cos(rads);
        float y = radius * std::sin(rads);

        float u = std::cos(rads) * 0.5f + 0.5f;
        float v = std::sin(rads) * 0.5f + 0.5f;

        vdata->position(x_offset + x, y_offset + y, z_offset);
        vdata->diffuse(smlt::Colour::WHITE);
        vdata->tex_coord0(u, v);
        vdata->tex_coord1(u, v);
        vdata->tex_coord2(u, v);
        vdata->tex_coord3(u, v);
        vdata->normal(0, 0, 1);
        vdata->move_next();
    }

    vdata->done();

    for(uint16_t i = 0; i < point_count; ++i) {
        idata->index(offset + i);
    }
    idata->done();

    return submesh;
}

SubMesh* circle_outline(smlt::MeshPtr mesh, smlt::MaterialPtr mat, float diameter, int32_t point_count, float x_offset, float y_offset, float z_offset) {
    float radius = diameter * 0.5f;

    SubMesh* submesh = mesh->new_submesh("circle_outline", mat, MESH_ARRANGEMENT_LINE_STRIP);

    auto offset = mesh->vertex_data->count();

    auto& vdata = mesh->vertex_data;
    auto& idata = submesh->index_data;

    vdata->move_to_end();

    for(uint16_t i = 0; i < point_count; ++i) {
        //Build some shared vertex data

        float rads = 2 * PI * i / point_count;

        float x = radius * std::cos(rads);
        float y = radius * std::sin(rads);

        float u = std::cos(rads) * 0.5f + 0.5f;
        float v = std::sin(rads) * 0.5f + 0.5f;

        vdata->position(x_offset + x, y_offset + y, z_offset);
        vdata->diffuse(smlt::Colour::WHITE);
        vdata->tex_coord0(u, v);
        vdata->tex_coord1(u, v);
        vdata->tex_coord2(u, v);
        vdata->tex_coord3(u, v);
        vdata->normal(0, 0, 1);
        vdata->move_next();
    }

    vdata->done();

    for(uint16_t i = 0; i < point_count; ++i) {
        idata->index(offset + i);
    }
    idata->index(0);
    idata->done();

    return submesh;
}

}
}
}
