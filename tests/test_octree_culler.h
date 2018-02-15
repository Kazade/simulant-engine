#pragma once

#include "global.h"

#include "../simulant/nodes/geoms/octree_culler.h"

namespace {

using namespace smlt;

class OctreeCullerTests : public SimulantTestCase {
public:
    void test_basic_visibility() {
        auto stage = window->new_stage();
        auto camera = stage->new_camera(); // Looking down -Z

        // Guarantee 2 renderables by using different materials
        auto mat1 = stage->assets->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY);
        auto mat2 = stage->assets->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY);

        auto mesh = stage->assets->new_mesh(VertexSpecification::DEFAULT).fetch();
        mesh->new_submesh_as_box("visible", mat1, 1.0, 1.0, 1.0, Vec3(0, 0, -10.0));
        mesh->new_submesh_as_box("not visible", mat2, 1.0, 1.0, 1.0, Vec3(0, 0, 10.0));

        OctreeCuller culler(nullptr, mesh);
        culler.compile();

        auto result = culler.renderables_visible(camera->frustum());

        // Only one renderable should come back
        assert_equal(1u, result.size());

        auto ret1 = result[0];

        camera->look_at(0, 0, 1); // Looking up +Z

        result = culler.renderables_visible(camera->frustum());
        assert_equal(1u, result.size());

        auto ret2 = result[0];

        // Should be different renderables that came back
        assert_not_equal(ret1.get(), ret2.get());
    }
};

}
