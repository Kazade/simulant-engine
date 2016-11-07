#include "circle.h"
#include "../../resource_manager.h"

namespace smlt {
namespace procedural {
namespace mesh {

SubMesh* circle(smlt::Mesh& mesh, float diameter, int32_t point_count, float x_offset, float y_offset, float z_offset) {
    float radius = diameter * 0.5f;

    SubMesh* submesh = mesh.new_submesh(
        "circle",
        MESH_ARRANGEMENT_TRIANGLE_FAN,
        VERTEX_SHARING_MODE_INDEPENDENT
    );

    auto& vdata = submesh->vertex_data;
    auto& idata = submesh->index_data;

    for(uint16_t i = 0; i < point_count; ++i) {
        //Build some shared vertex data

        float rads = 2 * kmPI * i / point_count;

        float x = radius * cos(rads);
        float y = radius * sin(rads);

        float u = cos(rads) * 0.5 + 0.5f;
        float v = sin(rads) * 0.5 + 0.5f;

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
        idata->index(i);
    }
    idata->done();

    return submesh;
}

SubMesh* circle_outline(smlt::Mesh& mesh, float diameter, int32_t point_count, float x_offset, float y_offset, float z_offset) {
    float radius = diameter * 0.5f;

    SubMesh* submesh = mesh.new_submesh("circle_outline", MESH_ARRANGEMENT_LINE_STRIP, VERTEX_SHARING_MODE_INDEPENDENT);

    auto& vdata = submesh->vertex_data;
    auto& idata = submesh->index_data;

    for(uint16_t i = 0; i < point_count; ++i) {
        //Build some shared vertex data

        float rads = 2 * kmPI * i / point_count;

        float x = radius * cos(rads);
        float y = radius * sin(rads);

        float u = cos(rads) * 0.5 + 0.5f;
        float v = sin(rads) * 0.5 + 0.5f;

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
        idata->index(i);
    }
    idata->index(0);
    idata->done();

    return submesh;
}

}
}
}
