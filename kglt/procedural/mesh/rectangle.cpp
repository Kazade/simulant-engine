#include "../../resource_manager.h"
#include "../../scene.h"
#include "rectangle.h"

namespace kglt {
namespace procedural {
namespace mesh {

SubMeshIndex new_rectangle_submesh(
    ProtectedPtr<Mesh>& mesh, float width, float height,
    float x_offset, float y_offset, float z_offset, MaterialID material_id) {

    //Create a submesh
    SubMeshIndex sm = mesh->new_submesh(
        (material_id) ? material_id : mesh->scene().clone_default_material(),
        MESH_ARRANGEMENT_TRIANGLES,
        false
    );


    auto& submesh = mesh->submesh(sm);

    //Build some shared vertex data
    submesh.vertex_data().position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    submesh.vertex_data().diffuse(kglt::Colour::WHITE);
    submesh.vertex_data().tex_coord0(0.0, 0.0);
    submesh.vertex_data().tex_coord1(0.0, 0.0);
    submesh.vertex_data().tex_coord2(0.0, 0.0);
    submesh.vertex_data().tex_coord3(0.0, 0.0);
    submesh.vertex_data().normal(0, 0, 1);
    submesh.vertex_data().move_next();

    submesh.vertex_data().position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    submesh.vertex_data().diffuse(kglt::Colour::WHITE);
    submesh.vertex_data().tex_coord0(1.0, 0.0);
    submesh.vertex_data().tex_coord1(1.0, 0.0);
    submesh.vertex_data().tex_coord2(1.0, 0.0);
    submesh.vertex_data().tex_coord3(1.0, 0.0);
    submesh.vertex_data().normal(0, 0, 1);
    submesh.vertex_data().move_next();

    submesh.vertex_data().position(x_offset + (width / 2.0),  y_offset + (height / 2.0), z_offset);
    submesh.vertex_data().diffuse(kglt::Colour::WHITE);
    submesh.vertex_data().tex_coord0(1.0, 1.0);
    submesh.vertex_data().tex_coord1(1.0, 1.0);
    submesh.vertex_data().tex_coord2(1.0, 1.0);
    submesh.vertex_data().tex_coord3(1.0, 1.0);
    submesh.vertex_data().normal(0, 0, 1);
    submesh.vertex_data().move_next();

    submesh.vertex_data().position(x_offset + (-width / 2.0),  y_offset + (height / 2.0), z_offset);
    submesh.vertex_data().diffuse(kglt::Colour::WHITE);
    submesh.vertex_data().tex_coord0(0.0, 1.0);
    submesh.vertex_data().tex_coord1(0.0, 1.0);
    submesh.vertex_data().tex_coord2(0.0, 1.0);
    submesh.vertex_data().tex_coord3(0.0, 1.0);
    submesh.vertex_data().normal(0, 0, 1);
    submesh.vertex_data().move_next();
    submesh.vertex_data().done();

    submesh.index_data().index(0);
    submesh.index_data().index(1);
    submesh.index_data().index(2);

    submesh.index_data().index(0);
    submesh.index_data().index(2);
    submesh.index_data().index(3);
    submesh.index_data().done();

    return sm;
}

SubMeshIndex rectangle(ProtectedPtr<Mesh> mesh, float width, float height, float x_offset, float y_offset, float z_offset, bool clear) {
    if(clear) {
        mesh->clear();
    }

    uint16_t offset = mesh->shared_data().count();

    mesh->shared_data().move_to_end();

    //Build some shared vertex data
    mesh->shared_data().position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh->shared_data().diffuse(kglt::Colour::WHITE);
    mesh->shared_data().tex_coord0(0.0, 0.0);
    mesh->shared_data().tex_coord1(0.0, 0.0);
    mesh->shared_data().tex_coord2(0.0, 0.0);
    mesh->shared_data().tex_coord3(0.0, 0.0);
    mesh->shared_data().normal(0, 0, 1);
    mesh->shared_data().move_next();

    mesh->shared_data().position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh->shared_data().diffuse(kglt::Colour::WHITE);
    mesh->shared_data().tex_coord0(1.0, 0.0);
    mesh->shared_data().tex_coord1(1.0, 0.0);
    mesh->shared_data().tex_coord2(1.0, 0.0);
    mesh->shared_data().tex_coord3(1.0, 0.0);
    mesh->shared_data().normal(0, 0, 1);
    mesh->shared_data().move_next();

    mesh->shared_data().position(x_offset + (width / 2.0),  y_offset + (height / 2.0), z_offset);
    mesh->shared_data().diffuse(kglt::Colour::WHITE);
    mesh->shared_data().tex_coord0(1.0, 1.0);
    mesh->shared_data().tex_coord1(1.0, 1.0);
    mesh->shared_data().tex_coord2(1.0, 1.0);
    mesh->shared_data().tex_coord3(1.0, 1.0);
    mesh->shared_data().normal(0, 0, 1);
    mesh->shared_data().move_next();

    mesh->shared_data().position(x_offset + (-width / 2.0),  y_offset + (height / 2.0), z_offset);
    mesh->shared_data().diffuse(kglt::Colour::WHITE);
    mesh->shared_data().tex_coord0(0.0, 1.0);
    mesh->shared_data().tex_coord1(0.0, 1.0);
    mesh->shared_data().tex_coord2(0.0, 1.0);
    mesh->shared_data().tex_coord3(0.0, 1.0);
    mesh->shared_data().normal(0, 0, 1);
    mesh->shared_data().move_next();
    mesh->shared_data().done();

    //Create a submesh that uses the shared data
    SubMeshIndex sm = mesh->new_submesh(
        mesh->scene().new_material_from_file(mesh->scene().default_material_filename()),
        MESH_ARRANGEMENT_TRIANGLES,
        true
    );
    mesh->submesh(sm).index_data().index(offset + 0);
    mesh->submesh(sm).index_data().index(offset + 1);
    mesh->submesh(sm).index_data().index(offset + 2);

    mesh->submesh(sm).index_data().index(offset + 0);
    mesh->submesh(sm).index_data().index(offset + 2);
    mesh->submesh(sm).index_data().index(offset + 3);
    mesh->submesh(sm).index_data().done();

    return sm;
}

SubMeshIndex rectangle_outline(ProtectedPtr<Mesh> mesh, float width, float height, float x_offset, float y_offset, float z_offset, bool clear) {
    if(clear) {
        mesh->clear();
    }

    uint16_t offset = mesh->shared_data().count();

    mesh->shared_data().position(x_offset + (-width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh->shared_data().diffuse(kglt::Colour::WHITE);
    mesh->shared_data().tex_coord0(0.0, 0.0);
    mesh->shared_data().tex_coord1(0.0, 0.0);
    mesh->shared_data().move_next();

    mesh->shared_data().position(x_offset + (width / 2.0), y_offset + (-height / 2.0), z_offset);
    mesh->shared_data().diffuse(kglt::Colour::WHITE);
    mesh->shared_data().tex_coord0(1.0, 0.0);
    mesh->shared_data().tex_coord1(1.0, 0.0);
    mesh->shared_data().move_next();

    mesh->shared_data().position(x_offset + (width / 2.0), y_offset + (height / 2.0), z_offset);
    mesh->shared_data().diffuse(kglt::Colour::WHITE);
    mesh->shared_data().tex_coord0(1.0, 1.0);
    mesh->shared_data().tex_coord1(1.0, 1.0);
    mesh->shared_data().move_next();

    mesh->shared_data().position(x_offset + (-width / 2.0), y_offset + (height / 2.0), z_offset);
    mesh->shared_data().diffuse(kglt::Colour::WHITE);
    mesh->shared_data().tex_coord0(0.0, 1.0);
    mesh->shared_data().tex_coord1(0.0, 1.0);
    mesh->shared_data().move_next();
    mesh->shared_data().done();
    
    SubMeshIndex sm = mesh->new_submesh(
        mesh->scene().new_material_from_file(mesh->scene().default_material_filename()),
        MESH_ARRANGEMENT_LINE_STRIP,
        true
    );

    for(uint8_t i = 0; i < 4; ++i) {
        mesh->submesh(sm).index_data().index(offset + i);
    }

    //Add the original point
    mesh->submesh(sm).index_data().index(offset);
    mesh->submesh(sm).index_data().done();

    return sm;
}

}
}
}
