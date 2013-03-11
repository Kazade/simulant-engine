
#include "capsule.h"

namespace kglt {
namespace procedural {
namespace mesh {

SubMeshIndex capsule(kglt::Mesh& mesh, float diameter, float height,
    uint32_t segment_count, uint32_t vertical_segment_count, uint32_t ring_count, const kglt::Vec3& pos_offset) {

    float radius = diameter * 0.5f;

    SubMeshIndex smi = mesh.new_submesh(MaterialID(), MESH_ARRANGEMENT_TRIANGLES, false);
    kglt::SubMesh& submesh = mesh.submesh(smi);

    kglt::VertexData& vdata = submesh.vertex_data();
    kglt::IndexData& idata = submesh.index_data();

    float delta_ring_angle = ((kmPI / 2.0) / ring_count);
    float delta_seg_angle = ((kmPI * 2.0) / segment_count);

    float sphere_ratio = radius / (2 * radius + height);
    float cylinder_ratio = height / (2 * radius + height);

    int32_t offset = 0;

    for(uint32_t ring = 0; ring <= ring_count; ++ring) {
        float r0 = radius * sinf(ring * delta_ring_angle);
        float y0 = radius * cosf(ring * delta_ring_angle);

        // Generate the group of segments for the current ring
        for(uint32_t seg = 0; seg <= segment_count; ++seg) {
            float x0 = r0 * cosf(seg * delta_seg_angle);
            float z0 = r0 * sinf(seg * delta_seg_angle);

            kglt::Vec3 new_point;
            kmVec3Fill(&new_point, x0, 0.5f * height + y0, z0);

            kglt::Vec3 new_normal;
            kmVec3Fill(&new_normal, x0, y0, z0);
            kmVec3Normalize(&new_normal, &new_normal);

            kglt::Vec2 new_tex;
            kmVec2Fill(&new_tex,
                (float) seg / (float) segment_count,
                (float) ring / (float) ring_count * sphere_ratio
            );

            // Add one vertex to the strip which makes up the sphere
            vdata.position(new_point);
            vdata.tex_coord0(new_tex);
            vdata.tex_coord1(new_tex);
            vdata.tex_coord2(new_tex);
            vdata.tex_coord3(new_tex);
            vdata.normal(new_normal);
            vdata.diffuse(kglt::Colour::white);
            vdata.move_next();

            // each vertex (except the last) has six indices pointing to it
            idata.index(offset + segment_count + 1);
            idata.index(offset + segment_count);
            idata.index(offset);
            idata.index(offset + segment_count + 1);
            idata.index(offset);
            idata.index(offset + 1);

            offset ++;
        } // end for seg
    }

    // Cylinder part
    float delta_angle = ((kmPI * 2.0) / segment_count);
    float delta_height = height / (float) vertical_segment_count;

    for(uint16_t i = 1; i < vertical_segment_count; i++) {
        for (uint16_t j = 0; j <= segment_count; j++) {
            float x0 = radius * cosf(j * delta_angle);
            float z0 = radius * sinf(j * delta_angle);

            kglt::Vec3 new_point;
            kmVec3Fill(&new_point,
                x0,
                0.5f * height - i * delta_height,
                z0
            );

            kglt::Vec3 new_normal;
            kmVec3Fill(&new_normal, x0, 0, z0);
            kmVec3Normalize(&new_normal, &new_normal);

            kglt::Vec2 new_tex;
            kmVec2Fill(&new_tex,
                j / (float)segment_count,
                i / (float)vertical_segment_count * cylinder_ratio + sphere_ratio
            );

            vdata.position(new_point);
            vdata.tex_coord0(new_tex);
            vdata.tex_coord1(new_tex);
            vdata.tex_coord2(new_tex);
            vdata.tex_coord3(new_tex);
            vdata.normal(new_normal);
            vdata.diffuse(kglt::Colour::white);
            vdata.move_next();

            idata.index(offset + segment_count + 1);
            idata.index(offset + segment_count);
            idata.index(offset);
            idata.index(offset + segment_count + 1);
            idata.index(offset);
            idata.index(offset + 1);

            offset ++;
        }
    }

    // Generate the group of rings for the sphere
    for(uint32_t ring = 0; ring <= ring_count; ring++) {
        float r0 = radius * sinf((kmPI / 2.0) + ring * delta_ring_angle);
        float y0 = radius * cosf((kmPI / 2.0) + ring * delta_ring_angle);

        // Generate the group of segments for the current ring
        for(uint32_t seg = 0; seg <= segment_count; seg++) {
            float x0 = r0 * cosf(seg * delta_seg_angle);
            float z0 = r0 * sinf(seg * delta_seg_angle);

            kglt::Vec3 new_point;
            kmVec3Fill(&new_point,
                x0,
                -0.5f * height + y0,
                z0
            );

            kglt::Vec3 new_normal;
            kmVec3Fill(&new_normal, x0, y0, z0);
            kmVec3Normalize(&new_normal, &new_normal);

            kglt::Vec2 new_tex;
            kmVec2Fill(&new_tex,
               (float) seg / (float) segment_count,
               (float) ring / (float) ring_count * sphere_ratio + cylinder_ratio + sphere_ratio
            );

            vdata.position(new_point);
            vdata.tex_coord0(new_tex);
            vdata.tex_coord1(new_tex);
            vdata.tex_coord2(new_tex);
            vdata.tex_coord3(new_tex);
            vdata.normal(new_normal);
            vdata.diffuse(kglt::Colour::white);
            vdata.move_next();

            if (ring != ring_count) {
                // each vertex (except the last) has six indices pointing to it
                idata.index(offset + segment_count + 1);
                idata.index(offset + segment_count);
                idata.index(offset);
                idata.index(offset + segment_count + 1);
                idata.index(offset);
                idata.index(offset + 1);
            }
            offset++;
        } // end for seg
    } // end for ring

    idata.done();
    vdata.done();

    return smi;
}



}
}
}
