#pragma once

#include <simulant/simulant.h>
#include <simulant/test.h>

namespace {

using namespace smlt;

class SphereApproximationTests : public test::SimulantTestCase {
public:

    void test_basic_usage() {
        auto mesh = scene->assets->create_mesh(VertexSpecification::DEFAULT);

        auto approx1 = mesh->generate_sphere_approximation();
        assert_equal(approx1.sphere_count(), 0u);

        mesh->create_submesh_as_sphere("Sphere", scene->assets->clone_default_material(), 1.0f, 5, 5);

        auto approx2 = mesh->generate_sphere_approximation();
        assert_equal(approx2.sphere_count(), 1u);
    }
};

}
