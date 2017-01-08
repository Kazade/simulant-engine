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


#include "cylinder.h"
#include "../../types.h"
#include "../../mesh.h"
#include "../../resource_manager.h"

namespace smlt {
namespace procedural {
namespace mesh {

void cylinder(MeshPtr mesh, float diameter, float length, int32_t segments, int32_t stacks) {
    float radius = diameter * 0.5;

    float delta_angle = (kmPI * 2.0) / (float) segments;
    float delta_height = length / (float) stacks;
    int offset = 0;

    auto buffer = mesh->new_submesh("cylinder");
    for(auto i = 0; i <= stacks; ++i) {
        for(auto j = 0; j <= segments; ++j) {
            float x0 = radius * cosf(delta_angle * j);
            float z0 = radius * sinf(delta_angle * j);

            smlt::Vec3 new_point(x0, delta_height * i, z0);
            smlt::Vec3 new_normal = smlt::Vec3(x0, 0, z0).normalized();
            smlt::Vec2 new_uv = smlt::Vec2(j / (float) segments, i / (float) stacks);

            mesh->shared_data->position(new_point);
            mesh->shared_data->diffuse(smlt::Colour::WHITE);
            mesh->shared_data->normal(new_normal);
            mesh->shared_data->tex_coord0(new_uv);
            mesh->shared_data->move_next();

            if(i != stacks) {
                buffer->index_data->index(offset + segments + 1);
                buffer->index_data->index(offset);
                buffer->index_data->index(offset + segments);
                buffer->index_data->index(offset + segments + 1);
                buffer->index_data->index(offset + 1);
                buffer->index_data->index(offset);
            }
            ++offset;
        }
    }

    // Now cap the cylinder
    auto center_index = offset;

    // Add a central point at the base
    mesh->shared_data->position(smlt::Vec3());
    mesh->shared_data->normal(smlt::Vec3(0, -1, 0));
    mesh->shared_data->tex_coord0(smlt::Vec2());
    mesh->shared_data->move_next();
    ++offset;

    for(auto j = 0; j <= segments; ++j) {
        float x0 = cosf(j * delta_angle);
        float z0 = sinf(j * delta_angle);

        smlt::Vec3 new_point(x0 * radius, 0, z0 * radius);
        smlt::Vec3 new_normal(0, -1, 0);
        smlt::Vec2 new_uv(x0, z0);

        mesh->shared_data->position(new_point);
        mesh->shared_data->normal(new_normal);
        mesh->shared_data->tex_coord0(new_uv);
        mesh->shared_data->move_next();

        if(j != segments) {
            buffer->index_data->index(center_index);
            buffer->index_data->index(offset);
            buffer->index_data->index(offset + 1);
        }
    }

    center_index = offset;
    // Add a central point at the top
    mesh->shared_data->position(smlt::Vec3(0, length, 0));
    mesh->shared_data->normal(smlt::Vec3(0, 1, 0));
    mesh->shared_data->tex_coord0(smlt::Vec2());
    mesh->shared_data->move_next();
    ++offset;

    for(auto j = 0; j <= segments; ++j) {
        float x0 = cosf(j * delta_angle);
        float z0 = sinf(j * delta_angle);

        smlt::Vec3 new_point(x0 * radius, length, z0 * radius);
        smlt::Vec3 new_normal(0, 1, 0);
        smlt::Vec2 new_uv(x0, z0);

        mesh->shared_data->position(new_point);
        mesh->shared_data->normal(new_normal);
        mesh->shared_data->tex_coord0(new_uv);
        mesh->shared_data->move_next();

        if(j != segments) {
            buffer->index_data->index(center_index);
            buffer->index_data->index(offset + 1);
            buffer->index_data->index(offset);
        }
    }

    mesh->shared_data->done();
    buffer->index_data->done();
}

}
}
}
