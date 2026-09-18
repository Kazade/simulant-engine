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

#include "torus.h"
#include "../../types.h"
#include "../../meshes/mesh.h"
#include "../../meshes/submesh.h"

namespace smlt {
namespace procedural {
namespace mesh {

void torus(SubMeshPtr submesh, float major_radius, float minor_radius,
          int32_t major_segments, int32_t minor_segments) {
    auto mesh = submesh->mesh.get();
    auto& buffer = submesh;

    float major_delta = (PI * 2.0f) / (float) major_segments;
    float minor_delta = (PI * 2.0f) / (float) minor_segments;

    for(auto i = 0; i < major_segments; ++i) {
        float u = i * major_delta;
        float cu = cosf(u);
        float su = sinf(u);

        for(auto j = 0; j < minor_segments; ++j) {
            float v = j * minor_delta;
            float cv = cosf(v);
            float sv = sinf(v);

            float px = (major_radius + minor_radius * cv) * cu;
            float py = minor_radius * sv;
            float pz = (major_radius + minor_radius * cv) * su;

            smlt::Vec3 normal = smlt::Vec3(cv * cu, sv, cv * su).normalized();

            mesh->vertex_data->position(smlt::Vec3(px, py, pz));
            mesh->vertex_data->color(smlt::Color::white());
            mesh->vertex_data->normal(normal);
            mesh->vertex_data->tex_coord0(
                smlt::Vec2(i / (float) major_segments,
                          j / (float) minor_segments));
            mesh->vertex_data->move_next();
        }
    }

    auto idx = [=](int32_t i, int32_t j) -> int32_t {
        i = ((i % major_segments) + major_segments) % major_segments;
        j = ((j % minor_segments) + minor_segments) % minor_segments;
        return i * minor_segments + j;
    };

    for(auto i = 0; i < major_segments; ++i) {
        for(auto j = 0; j < minor_segments; ++j) {
            auto a = idx(i, j);
            auto b = idx(i + 1, j);
            auto c = idx(i + 1, j + 1);
            auto d = idx(i, j + 1);

            buffer->index_data->index(a);
            buffer->index_data->index(b);
            buffer->index_data->index(c);

            buffer->index_data->index(a);
            buffer->index_data->index(c);
            buffer->index_data->index(d);
        }
    }

    mesh->vertex_data->done();
    buffer->index_data->done();
}

}
}
}
