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


#include "sphere.h"
#include "../../types.h"
#include "../../asset_manager.h"
#include "../../meshes/mesh.h"

namespace smlt {
namespace procedural {
namespace mesh {

float sa_acos(float fac) {
    if(fac <= -1.0f) {
        return (float)PI;
    } else if(fac >= 1.0f) {
        return 0.0f;
    }

    return (float)acos(fac);
}

void generate_uv(const Vec3& pos, float& u, float& v) {
    float len = pos.length();

    if(len > 0.0) {
        if(pos.x == 0.0 && pos.y == 0.0) {
            u = 0.0;
        } else {
            u = (1.0 - atan2(pos.x, pos.y) / PI) / 2.0;
        }

        float z = pos.z / len;
        v = 1.0 - sa_acos(z) / PI;
    }
}

void sphere(SubMeshPtr submesh, float diameter, int32_t slices, int32_t stacks) {
    float theta, phi;
    float u, v;
    Vec3 pos, n;

    const float radius = diameter / 2.0;

    for(int32_t current_stack = 1; current_stack < stacks - 1; ++current_stack) {
        for(int32_t current_slice = 0; current_slice < slices; ++current_slice) {
            theta = float(current_stack) / (stacks - 1) * PI;
            phi = float(current_slice) / (slices - 1) * PI * 2.0;

            pos = Vec3(
               sinf(theta) * cosf(phi) * radius,
               cosf(theta) * radius,
               -sinf(theta) * sinf(phi) * radius
            );

            n = pos.normalized();

            generate_uv(pos, u, v);

            submesh->vertex_data->position(pos);
            submesh->vertex_data->tex_coord0(u, v);
            submesh->vertex_data->tex_coord1(u, v);
            submesh->vertex_data->normal(n);
            submesh->vertex_data->diffuse(smlt::Colour::WHITE);
            submesh->vertex_data->move_next();
        }
    }
    pos = Vec3(0, 1 * radius, 0);
    n = pos.normalized();
    generate_uv(pos, u, v);

    submesh->vertex_data->position(pos);
    submesh->vertex_data->tex_coord0(u, v);
    submesh->vertex_data->tex_coord1(u, v);
    submesh->vertex_data->diffuse(smlt::Colour::WHITE);
    submesh->vertex_data->move_next();

    pos = Vec3(0, -1 * radius, 0);
    n = pos.normalized();

    generate_uv(pos, u, v);

    submesh->vertex_data->position(pos);
    submesh->vertex_data->tex_coord0(u, v);
    submesh->vertex_data->tex_coord1(u, v);
    submesh->vertex_data->diffuse(smlt::Colour::WHITE);
    submesh->vertex_data->move_next();

    submesh->vertex_data->done();

    for(int32_t current_stack = 0; current_stack < stacks - 3; current_stack++) {
        for(int32_t current_slice = 0; current_slice < slices - 1; current_slice++) {
            submesh->index_data->index(current_stack * slices + current_slice);
            submesh->index_data->index((current_stack + 1) * slices + current_slice + 1);
            submesh->index_data->index(current_stack * slices + current_slice + 1);

            submesh->index_data->index(current_stack * slices + current_slice);
            submesh->index_data->index((current_stack + 1) * slices + current_slice);
            submesh->index_data->index((current_stack + 1) * slices + current_slice + 1);
        }
    }

    for(int32_t i = 0; i < slices - 1; ++i) {
        submesh->index_data->index((stacks - 2) * slices);
        submesh->index_data->index(i);
        submesh->index_data->index(i + 1);

        submesh->index_data->index((stacks - 2) * slices + 1);
        submesh->index_data->index((stacks - 3) * slices + i + 1);
        submesh->index_data->index((stacks - 3) * slices + i);
    }

    submesh->index_data->done();
}

}
}
}
