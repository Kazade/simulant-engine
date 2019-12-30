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


#include "cylinder.h"
#include "../../types.h"
#include "../../meshes/mesh.h"
#include "../../asset_manager.h"

namespace smlt {
namespace procedural {
namespace mesh {

void cylinder(MeshPtr mesh, float diameter, float length, int32_t segments, int32_t stacks) {
    float radius = diameter * 0.5f;
    float half_length = length * 0.5f;
    float delta_angle = (PI * 2.0f) / (float) segments;
    float delta_height = length / (float) stacks;
    int offset = 0;

    // 5 6 7 8 9
    // 0 1 2 3 4

    auto buffer = mesh->new_submesh("cylinder");
    for(auto i = 0; i <= stacks; ++i) {
        auto segment_offset = offset;
        for(auto j = 0; j < segments; ++j) {
            float x0 = radius * cosf(delta_angle * j);
            float z0 = radius * sinf(delta_angle * j);

            smlt::Vec3 new_point(x0, (delta_height * i) - half_length, z0);
            smlt::Vec3 new_normal = smlt::Vec3(x0, 0, z0).normalized();
            smlt::Vec2 new_uv = smlt::Vec2(j / (float) segments, i / (float) stacks);

            mesh->vertex_data->position(new_point);
            mesh->vertex_data->diffuse(smlt::Colour::WHITE);
            mesh->vertex_data->normal(new_normal);
            mesh->vertex_data->tex_coord0(new_uv);
            mesh->vertex_data->move_next();

            if(i > 0 && j > 0) {
                buffer->index_data->index(offset - 1);
                buffer->index_data->index(offset);
                buffer->index_data->index(offset - segments - 1);
                buffer->index_data->index(offset - segments - 1);
                buffer->index_data->index(offset);
                buffer->index_data->index(offset - segments);

                if(j == segments - 1) {
                    buffer->index_data->index(offset);
                    buffer->index_data->index(segment_offset);
                    buffer->index_data->index(offset - segments);
                    buffer->index_data->index(offset - segments);
                    buffer->index_data->index(segment_offset);
                    buffer->index_data->index(segment_offset - segments);
                }
            }
            ++offset;
        }
    }

    // Now cap the cylinder
    auto center_index = offset;

    // Add a central point at the base
    mesh->vertex_data->position(smlt::Vec3(0, -half_length, 0));
    mesh->vertex_data->normal(smlt::Vec3(0, -1, 0));
    mesh->vertex_data->tex_coord0(smlt::Vec2());
    mesh->vertex_data->move_next();

    for(auto j = 1; j <= segments; ++j) {
        float x0 = cosf(j * delta_angle);
        float z0 = sinf(j * delta_angle);

        smlt::Vec3 new_point(x0 * radius, -half_length, z0 * radius);
        smlt::Vec3 new_normal(0, -1, 0);
        smlt::Vec2 new_uv(x0, z0);

        mesh->vertex_data->position(new_point);
        mesh->vertex_data->normal(new_normal);
        mesh->vertex_data->tex_coord0(new_uv);
        mesh->vertex_data->move_next();

        if(j > 1) {
            buffer->index_data->index(center_index);
            buffer->index_data->index(center_index + j - 1);
            buffer->index_data->index(center_index + j);
        }
    }

    buffer->index_data->index(center_index);
    buffer->index_data->index(center_index + segments);
    buffer->index_data->index(center_index + 1);

    center_index = mesh->vertex_data->count();
    // Add a central point at the top
    mesh->vertex_data->position(smlt::Vec3(0, half_length, 0));
    mesh->vertex_data->normal(smlt::Vec3(0, 1, 0));
    mesh->vertex_data->tex_coord0(smlt::Vec2());
    mesh->vertex_data->move_next();

    for(auto j = 1; j <= segments; ++j) {
        float x0 = cosf(j * delta_angle);
        float z0 = sinf(j * delta_angle);

        smlt::Vec3 new_point(x0 * radius, half_length, z0 * radius);
        smlt::Vec3 new_normal(0, 1, 0);
        smlt::Vec2 new_uv(x0, z0);

        mesh->vertex_data->position(new_point);
        mesh->vertex_data->normal(new_normal);
        mesh->vertex_data->tex_coord0(new_uv);
        mesh->vertex_data->move_next();
        ++offset;

        if(j > 1) {
            buffer->index_data->index(center_index);
            buffer->index_data->index(center_index + j);
            buffer->index_data->index(center_index + j - 1);
        }
    }

    // Final closing triangle in the cap
    buffer->index_data->index(center_index);
    buffer->index_data->index(center_index + 1);
    buffer->index_data->index(center_index + segments);

    mesh->vertex_data->done();
    buffer->index_data->done();
}

}
}
}
