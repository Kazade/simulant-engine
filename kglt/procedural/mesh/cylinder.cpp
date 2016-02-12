#include <kazmath/kazmath.h>
#include "cylinder.h"
#include "../../mesh.h"
#include "../../resource_manager.h"

namespace kglt {
namespace procedural {
namespace mesh {

void cylinder(ProtectedPtr<Mesh> mesh, float diameter, float length, int32_t segments, int32_t stacks) {
    float radius = diameter * 0.5;

    float delta_angle = (kmPI * 2.0) / (float) segments;
    float delta_height = length / (float) stacks;
    int offset = 0;

    auto smi = mesh->new_submesh();
    auto* buffer = mesh->submesh(smi);

    for(auto i = 0; i <= stacks; ++i) {
        for(auto j = 0; j <= segments; ++j) {
            float x0 = radius * cosf(delta_angle * j);
            float z0 = radius * sinf(delta_angle * j);

            kglt::Vec3 new_point(x0, delta_height * i, z0);
            kglt::Vec3 new_normal = kglt::Vec3(x0, 0, z0).normalized();
            kglt::Vec2 new_uv = kglt::Vec2(j / (float) segments, i / (float) stacks);

            mesh->shared_data().position(new_point);
            mesh->shared_data().diffuse(kglt::Colour::WHITE);
            mesh->shared_data().normal(new_normal);
            mesh->shared_data().tex_coord0(new_uv);
            mesh->shared_data().move_next();

            if(i != stacks) {
                buffer->index_data().index(offset + segments + 1);
                buffer->index_data().index(offset);
                buffer->index_data().index(offset + segments);
                buffer->index_data().index(offset + segments + 1);
                buffer->index_data().index(offset + 1);
                buffer->index_data().index(offset);
            }
            ++offset;
        }
    }

    // Now cap the cylinder
    auto center_index = offset;

    // Add a central point at the base
    mesh->shared_data().position(kglt::Vec3());
    mesh->shared_data().normal(kglt::Vec3(0, -1, 0));
    mesh->shared_data().tex_coord0(kglt::Vec2());
    mesh->shared_data().move_next();
    ++offset;

    for(auto j = 0; j <= segments; ++j) {
        float x0 = cosf(j * delta_angle);
        float z0 = sinf(j * delta_angle);

        kglt::Vec3 new_point(x0 * radius, 0, z0 * radius);
        kglt::Vec3 new_normal(0, -1, 0);
        kglt::Vec2 new_uv(x0, z0);

        mesh->shared_data().position(new_point);
        mesh->shared_data().normal(new_normal);
        mesh->shared_data().tex_coord0(new_uv);
        mesh->shared_data().move_next();

        if(j != segments) {
            buffer->index_data().index(center_index);
            buffer->index_data().index(offset);
            buffer->index_data().index(offset + 1);
        }
    }

    center_index = offset;
    // Add a central point at the top
    mesh->shared_data().position(kglt::Vec3(0, length, 0));
    mesh->shared_data().normal(kglt::Vec3(0, 1, 0));
    mesh->shared_data().tex_coord0(kglt::Vec2());
    mesh->shared_data().move_next();
    ++offset;

    for(auto j = 0; j <= segments; ++j) {
        float x0 = cosf(j * delta_angle);
        float z0 = sinf(j * delta_angle);

        kglt::Vec3 new_point(x0 * radius, length, z0 * radius);
        kglt::Vec3 new_normal(0, 1, 0);
        kglt::Vec2 new_uv(x0, z0);

        mesh->shared_data().position(new_point);
        mesh->shared_data().normal(new_normal);
        mesh->shared_data().tex_coord0(new_uv);
        mesh->shared_data().move_next();

        if(j != segments) {
            buffer->index_data().index(center_index);
            buffer->index_data().index(offset + 1);
            buffer->index_data().index(offset);
        }
    }

    mesh->shared_data().done();
    buffer->index_data().done();
}

}
}
}
