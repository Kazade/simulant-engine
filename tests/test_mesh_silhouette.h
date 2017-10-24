#pragma once

#include "global.h"
#include "../simulant/shadows.h"

namespace {

using namespace smlt;

class MeshSilhouetteTests : public SimulantTestCase {
public:
    void test_directional_silhouette_generation() {
        auto stage = window->new_stage().fetch();
        auto mesh = stage->assets->new_mesh_as_rectangle(1.0f, 1.0f).fetch();
        auto light = stage->new_light_as_directional().fetch();
        light->move_to(0, 0, -10);

        MeshSilhouette silhouette(mesh, Mat4(), light);
        assert_equal(4u, silhouette.edge_list().size());
    }

    void test_point_silhouette_generation() {
        auto stage = window->new_stage().fetch();
        auto mesh = stage->assets->new_mesh_as_rectangle(1.0f, 1.0f).fetch();
        auto light = stage->new_light_as_point().fetch();
        light->move_to(0, 0, -10);

        MeshSilhouette silhouette(mesh, Mat4(), light);
        assert_equal(4u, silhouette.edge_list().size());
    }
};

}
