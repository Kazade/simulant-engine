#pragma once

#include "global.h"
#include "../simulant/nodes/geom.h"


namespace {

using namespace smlt;

class GeomTests : public SimulantTestCase {
public:
    void test_renderables_added_to_queue() {
        auto stage = window->new_stage();

        assert_equal(stage->render_queue->pass_count(), 0u);

        auto mesh = stage->assets->new_mesh_as_cube(1.0);
        auto mat = mesh.fetch()->first_submesh()->material_id().fetch();

        stage->new_geom_with_mesh(mesh);

        assert_equal(stage->render_queue->pass_count(), mat->pass_count());
        assert_equal(stage->render_queue->group_count(0), 1u);
    }

    void test_renderables_removed_from_queue() {
        auto stage = window->new_stage();

        assert_equal(stage->render_queue->pass_count(), 0u);

        auto mesh = stage->assets->new_mesh_as_cube(1.0);
        auto mat = mesh.fetch()->first_submesh()->material_id().fetch();

        auto geom = stage->new_geom_with_mesh(mesh);

        assert_equal(stage->render_queue->pass_count(), mat->pass_count());
        assert_equal(stage->render_queue->group_count(0), 1u);

        stage->delete_geom(geom->id());

        assert_equal(stage->render_queue->pass_count(), 0u);
    }
};

}
