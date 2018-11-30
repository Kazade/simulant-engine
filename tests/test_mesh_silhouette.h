#pragma once


#include "../simulant/shadows.h"

namespace {

using namespace smlt;

class MeshSilhouetteTests : public smlt::test::SimulantTestCase {
public:
    void test_directional_silhouette_generation() {
        auto stage = window->new_stage();
        auto mesh = stage->assets->new_mesh_as_rectangle(1.0f, 1.0f).fetch();
        auto light = stage->new_light_as_directional();
        light->move_to(0, 0, -10);

        MeshSilhouette silhouette(mesh, Mat4(), light);
        assert_equal(4u, silhouette.edge_list().size());
    }

    void test_point_silhouette_generation() {
        auto stage = window->new_stage();
        auto mesh = stage->assets->new_mesh_as_rectangle(1.0f, 1.0f).fetch();
        auto light = stage->new_light_as_point();
        light->move_to(0, 0, -10);

        MeshSilhouette silhouette(mesh, Mat4(), light);
        assert_equal(4u, silhouette.edge_list().size());
    }

    void test_out_of_range_generates_none() {
        auto stage = window->new_stage();
        auto mesh = stage->assets->new_mesh_as_rectangle(1.0f, 1.0f).fetch();
        auto light = stage->new_light_as_point();
        light->move_to(0, 0, -10);
        light->set_attenuation_from_range(5.0);

        // Out of range of light, no silhouette edges
        MeshSilhouette silhouette(mesh, Mat4(), light);
        assert_equal(0u, silhouette.edge_list().size());
    }
};

}
