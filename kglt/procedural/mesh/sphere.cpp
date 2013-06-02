#include <kazmath/kazmath.h>
#include "sphere.h"
#include "../../resource_manager.h"
#include "../../scene.h"
#include "../../mesh.h"

namespace kglt {
namespace procedural {
namespace mesh {

float sa_acos(float fac) {
    if(fac <= -1.0f) {
        return (float)kmPI;
    } else if(fac >= 1.0f) {
        return 0.0f;
    }

    return (float)acos(fac);
}

void generate_uv(const kmVec3& pos, float& u, float& v) {
    float len = kmVec3Length(&pos);

    if(len > 0.0) {
        if(pos.x == 0.0 && pos.y == 0.0) {
            u = 0.0;
        } else {
            u = (1.0 - atan2(pos.x, pos.y) / kmPI) / 2.0;
        }

        float z = pos.z / len;
        v = 1.0 - sa_acos(z) / kmPI;
    }
}

void sphere(kglt::Mesh& mesh, float diameter, int32_t slices, int32_t stacks) {
    float theta, phi;
    float u, v;
    kmVec3 pos, n;

    const float radius = diameter / 2.0;

    for(int32_t current_stack = 1; current_stack < stacks - 1; ++current_stack) {
        for(int32_t current_slice = 0; current_slice < slices; ++current_slice) {
            theta = float(current_stack) / (stacks - 1) * kmPI;
            phi = float(current_slice) / (slices - 1) * kmPI * 2.0;

            kmVec3Fill(&pos,
               sinf(theta) * cosf(phi) * radius,
               cosf(theta) * radius,
               -sinf(theta) * sinf(phi) * radius
            );
            kmVec3Normalize(&n, &pos);

            generate_uv(pos, u, v);

            mesh.shared_data().position(pos);
            mesh.shared_data().tex_coord0(u, v);
            mesh.shared_data().tex_coord1(u, v);
            mesh.shared_data().normal(n);
            mesh.shared_data().diffuse(kglt::Colour::white);
            mesh.shared_data().move_next();
        }
    }
    kmVec3Fill(&pos, 0, 1 * radius, 0);
    kmVec3Normalize(&n, &pos);
    generate_uv(pos, u, v);

    mesh.shared_data().position(pos);
    mesh.shared_data().tex_coord0(u, v);
    mesh.shared_data().tex_coord1(u, v);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().move_next();

    kmVec3Fill(&pos, 0, -1 * radius, 0);
    kmVec3Normalize(&n, &pos);
    generate_uv(pos, u, v);

    mesh.shared_data().position(pos);
    mesh.shared_data().tex_coord0(u, v);
    mesh.shared_data().tex_coord1(u, v);
    mesh.shared_data().diffuse(kglt::Colour::white);
    mesh.shared_data().move_next();

    mesh.shared_data().done();

    SubMeshIndex sm = mesh.new_submesh(
        mesh.scene().clone_default_material(),
        MESH_ARRANGEMENT_TRIANGLES,
        true
    );

    for(int32_t current_stack = 0; current_stack < stacks - 3; current_stack++) {
        for(int32_t current_slice = 0; current_slice < slices - 1; current_slice++) {
            mesh.submesh(sm).index_data().index(current_stack * slices + current_slice);
            mesh.submesh(sm).index_data().index((current_stack + 1) * slices + current_slice + 1);
            mesh.submesh(sm).index_data().index(current_stack * slices + current_slice + 1);

            mesh.submesh(sm).index_data().index(current_stack * slices + current_slice);
            mesh.submesh(sm).index_data().index((current_stack + 1) * slices + current_slice);
            mesh.submesh(sm).index_data().index((current_stack + 1) * slices + current_slice + 1);
        }
    }

    for(int32_t i = 0; i < slices - 1; ++i) {
        mesh.submesh(sm).index_data().index((stacks - 2) * slices);
        mesh.submesh(sm).index_data().index(i);
        mesh.submesh(sm).index_data().index(i + 1);

        mesh.submesh(sm).index_data().index((stacks - 2) * slices + 1);
        mesh.submesh(sm).index_data().index((stacks - 3) * slices + i + 1);
        mesh.submesh(sm).index_data().index((stacks - 3) * slices + i);
    }

    mesh.submesh(sm).index_data().done();
}

}
}
}
