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
#include "capsule.h"

namespace smlt {
namespace procedural {
namespace mesh {

SubMesh* capsule(MeshPtr mesh, float diameter, float height,
    uint32_t segment_count, uint32_t vertical_segment_count, uint32_t ring_count, const smlt::Vec3& pos_offset) {

    float radius = diameter * 0.5f;

    SubMesh* submesh = mesh->new_submesh(
        "capsule",
        MESH_ARRANGEMENT_TRIANGLES
    );

    auto& vdata = submesh->vertex_data;
    auto& idata = submesh->index_data;

    float delta_ring_angle = ((PI / 2.0) / ring_count);
    float delta_seg_angle = ((PI * 2.0) / segment_count);

    float sphere_ratio = radius / (2 * radius + height);
    float cylinder_ratio = height / (2 * radius + height);

    auto offset = mesh->shared_data->count();

    for(uint32_t ring = 0; ring <= ring_count; ++ring) {
        float r0 = radius * sinf(ring * delta_ring_angle);
        float y0 = radius * cosf(ring * delta_ring_angle);

        // Generate the group of segments for the current ring
        for(uint32_t seg = 0; seg <= segment_count; ++seg) {
            float x0 = r0 * cosf(seg * delta_seg_angle);
            float z0 = r0 * sinf(seg * delta_seg_angle);

            smlt::Vec3 new_point(x0, 0.5f * height + y0, z0);

            smlt::Vec3 new_normal(x0, y0, z0);
            new_normal.normalize();

            smlt::Vec2 new_tex(
                (float) seg / (float) segment_count,
                (float) ring / (float) ring_count * sphere_ratio
            );

            // Add one vertex to the strip which makes up the sphere
            vdata->position(new_point);
            vdata->tex_coord0(new_tex);
            vdata->tex_coord1(new_tex);
            vdata->normal(new_normal);
            vdata->diffuse(smlt::Colour::WHITE);
            vdata->move_next();

            // each vertex (except the last) has six indices pointing to it
            idata->index(offset + segment_count + 1);
            idata->index(offset + segment_count);
            idata->index(offset);
            idata->index(offset + segment_count + 1);
            idata->index(offset);
            idata->index(offset + 1);

            offset ++;
        } // end for seg
    }

    // Cylinder part
    float delta_angle = ((PI * 2.0) / segment_count);
    float delta_height = height / (float) vertical_segment_count;

    for(uint16_t i = 1; i < vertical_segment_count; i++) {
        for (uint16_t j = 0; j <= segment_count; j++) {
            float x0 = radius * cosf(j * delta_angle);
            float z0 = radius * sinf(j * delta_angle);

            Vec3 new_point(
                x0,
                0.5f * height - i * delta_height,
                z0
            );

            Vec3 new_normal(x0, 0, z0);
            new_normal.normalize();

            Vec2 new_tex(
                j / (float)segment_count,
                i / (float)vertical_segment_count * cylinder_ratio + sphere_ratio
            );

            vdata->position(new_point);
            vdata->tex_coord0(new_tex);
            vdata->tex_coord1(new_tex);
            vdata->normal(new_normal);
            vdata->diffuse(smlt::Colour::WHITE);
            vdata->move_next();

            idata->index(offset + segment_count + 1);
            idata->index(offset + segment_count);
            idata->index(offset);
            idata->index(offset + segment_count + 1);
            idata->index(offset);
            idata->index(offset + 1);

            offset ++;
        }
    }

    // Generate the group of rings for the sphere
    for(uint32_t ring = 0; ring <= ring_count; ring++) {
        float r0 = radius * sinf((PI / 2.0) + ring * delta_ring_angle);
        float y0 = radius * cosf((PI / 2.0) + ring * delta_ring_angle);

        // Generate the group of segments for the current ring
        for(uint32_t seg = 0; seg <= segment_count; seg++) {
            float x0 = r0 * cosf(seg * delta_seg_angle);
            float z0 = r0 * sinf(seg * delta_seg_angle);

            Vec3 new_point(
                x0,
                -0.5f * height + y0,
                z0
            );

            Vec3 new_normal(x0, y0, z0);
            new_normal.normalize();

            Vec2 new_tex(
               (float) seg / (float) segment_count,
               (float) ring / (float) ring_count * sphere_ratio + cylinder_ratio + sphere_ratio
            );

            vdata->position(new_point);
            vdata->tex_coord0(new_tex);
            vdata->tex_coord1(new_tex);
            vdata->normal(new_normal);
            vdata->diffuse(smlt::Colour::WHITE);
            vdata->move_next();

            if (ring != ring_count) {
                // each vertex (except the last) has six indices pointing to it
                idata->index(offset + segment_count + 1);
                idata->index(offset + segment_count);
                idata->index(offset);
                idata->index(offset + segment_count + 1);
                idata->index(offset);
                idata->index(offset + 1);
            }
            offset++;
        } // end for seg
    } // end for ring

    idata->done();
    vdata->done();

    return submesh;
}



}
}
}
