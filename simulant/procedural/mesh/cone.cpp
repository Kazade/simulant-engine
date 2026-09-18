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

#include "cone.h"
#include "../../types.h"
#include "../../meshes/mesh.h"
#include "../../meshes/submesh.h"

namespace smlt {
namespace procedural {
namespace mesh {

void cone(SubMeshPtr submesh, float diameter, float length, int32_t segments) {
    float radius = diameter * 0.5f;
    float half_length = length * 0.5f;
    float delta_angle = (PI * 2.0f) / (float) segments;

    auto mesh = submesh->mesh.get();
    auto& buffer = submesh;

    // Side surface: one base-ring vertex + one duplicated apex vertex per
    // segment, so each side face gets its own outward-facing normal (same
    // "duplicate the shared point" approach cylinder() uses for its caps).
    for(auto j = 0; j < segments; ++j) {
        float theta = delta_angle * j;
        float x0 = radius * cosf(theta);
        float z0 = radius * sinf(theta);

        // The lateral surface normal of a right circular cone has a
        // constant "up" component of radius/length at every point on the
        // slant, regardless of how far up the slant you are.
        smlt::Vec3 side_normal =
            smlt::Vec3(cosf(theta), radius / length, sinf(theta))
                .normalized();

        mesh->vertex_data->position(smlt::Vec3(x0, -half_length, z0));
        mesh->vertex_data->color(smlt::Color::white());
        mesh->vertex_data->normal(side_normal);
        mesh->vertex_data->tex_coord0(
            smlt::Vec2(j / (float) segments, 0.0f));
        mesh->vertex_data->move_next();

        mesh->vertex_data->position(smlt::Vec3(0, half_length, 0));
        mesh->vertex_data->color(smlt::Color::white());
        mesh->vertex_data->normal(side_normal);
        mesh->vertex_data->tex_coord0(
            smlt::Vec2(j / (float) segments, 1.0f));
        mesh->vertex_data->move_next();
    }

    for(auto j = 0; j < segments; ++j) {
        int base_idx = j * 2;
        int apex_idx = base_idx + 1;
        int next_base_idx = ((j + 1) % segments) * 2;

        buffer->index_data->index(base_idx);
        buffer->index_data->index(next_base_idx);
        buffer->index_data->index(apex_idx);
    }

    // Base cap (facing down), same construction as cylinder()'s base cap.
    auto center_index = mesh->vertex_data->count();
    mesh->vertex_data->position(smlt::Vec3(0, -half_length, 0));
    mesh->vertex_data->color(smlt::Color::white());
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
        mesh->vertex_data->color(smlt::Color::white());
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

    mesh->vertex_data->done();
    buffer->index_data->done();
}

}
}
}
