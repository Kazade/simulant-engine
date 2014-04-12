#include "circle.h"
#include "../../scene.h"
#include "../../resource_manager.h"

namespace kglt {
namespace procedural {
namespace mesh {

SubMeshIndex circle(kglt::Mesh& mesh, float diameter, int32_t point_count, float x_offset, float y_offset, float z_offset) {
    float radius = diameter * 0.5f;

    SubMeshIndex smi = mesh.new_submesh(
        mesh.resource_manager().new_material_from_file(mesh.resource_manager().default_material_filename()),
        MESH_ARRANGEMENT_TRIANGLE_FAN,
        false
    );

    kglt::SubMesh& submesh = mesh.submesh(smi);

    kglt::VertexData& vdata = submesh.vertex_data();
    kglt::IndexData& idata = submesh.index_data();

    for(uint16_t i = 0; i < point_count; ++i) {
        //Build some shared vertex data

        float rads = 2 * kmPI * i / point_count;

        float x = radius * cos(rads);
        float y = radius * sin(rads);

        float u = cos(rads) * 0.5 + 0.5f;
        float v = sin(rads) * 0.5 + 0.5f;

        vdata.position(x_offset + x, y_offset + y, z_offset);
        vdata.diffuse(kglt::Colour::WHITE);
        vdata.tex_coord0(u, v);
        vdata.tex_coord1(u, v);
        vdata.tex_coord2(u, v);
        vdata.tex_coord3(u, v);
        vdata.normal(0, 0, 1);
        vdata.move_next();
    }

    vdata.done();

    for(uint16_t i = 0; i < point_count; ++i) {
        idata.index(i);
    }
    idata.done();

    return smi;
}

SubMeshIndex circle_outline(kglt::Mesh& mesh, float diameter, int32_t point_count, float x_offset, float y_offset, float z_offset) {
    float radius = diameter * 0.5f;

    SubMeshIndex smi = mesh.new_submesh(MaterialID(), MESH_ARRANGEMENT_LINE_STRIP, false);
    kglt::SubMesh& submesh = mesh.submesh(smi);

    kglt::VertexData& vdata = submesh.vertex_data();
    kglt::IndexData& idata = submesh.index_data();

    for(uint16_t i = 0; i < point_count; ++i) {
        //Build some shared vertex data

        float rads = 2 * kmPI * i / point_count;

        float x = radius * cos(rads);
        float y = radius * sin(rads);

        float u = cos(rads) * 0.5 + 0.5f;
        float v = sin(rads) * 0.5 + 0.5f;

        vdata.position(x_offset + x, y_offset + y, z_offset);
        vdata.diffuse(kglt::Colour::WHITE);
        vdata.tex_coord0(u, v);
        vdata.tex_coord1(u, v);
        vdata.tex_coord2(u, v);
        vdata.tex_coord3(u, v);
        vdata.normal(0, 0, 1);
        vdata.move_next();
    }

    vdata.done();

    for(uint16_t i = 0; i < point_count; ++i) {
        idata.index(i);
    }
    idata.index(0);
    idata.done();

    return smi;
}

}
}
}
