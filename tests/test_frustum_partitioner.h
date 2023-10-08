#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/partitioners/frustum_partitioner.h"

namespace {

using namespace smlt;

class FrustumPartitionerTests : public test::SimulantTestCase {
public:
    void set_up() override {
        test::SimulantTestCase::set_up();

        stage_ = scene->create_child<smlt::Stage>();
        box_ = application->shared_assets->create_mesh_as_cube_with_submesh_per_face(1.0f);
    }

    void tear_down() override {
        test::SimulantTestCase::tear_down();

        stage_->destroy();
    }

    void test_visibility() {
        FrustumPartitioner* partitioner = scene->create_child<FrustumPartitioner>();

        auto camera = scene->create_child<smlt::Camera>();
        camera->set_parent(partitioner);
        camera->transform->set_translation(Vec3(784, 58, -775));

        auto a1 = scene->create_child<smlt::Actor>(box_);
        a1->set_parent(partitioner);

        assert_true(a1->has_any_mesh());
        assert_close(a1->aabb().max_dimension(), 1.0f, 0.0001f);

        a1->transform->set_translation(Vec3(791, 58, -810));

        assert_false(a1->transformed_aabb().has_zero_area());

        batcher::RenderQueue queue;
        queue.reset(partitioner, window->renderer.get(), camera);

        Viewport viewport;
        partitioner->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        assert_true(queue.renderable_count() > 0);
    }

    void test_nodes_returned_if_never_culled() {
        FrustumPartitioner* partitioner = scene->create_child<FrustumPartitioner>();

        auto camera = scene->create_child<smlt::Camera>();
        auto a1 = scene->create_child<smlt::Actor>(box_);
        a1->set_parent(partitioner);
        a1->transform->set_translation(Vec3(0, 0, 100));

        batcher::RenderQueue queue;
        queue.reset(partitioner, window->renderer.get(), camera);

        Viewport viewport;
        partitioner->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        /* Not visible */
        assert_equal(queue.renderable_count(), 0u);

        a1->set_cullable(false);

        partitioner->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        /* Now visible */
        assert_true(queue.renderable_count() > 0);
    }

    void test_destroyed_nodes_not_returned() {
        FrustumPartitioner* partitioner = scene->create_child<FrustumPartitioner>();

        auto camera = scene->create_child<smlt::Camera>();

        auto a1 = scene->create_child<smlt::Actor>(box_);
        auto a2 = scene->create_child<smlt::Actor>(box_);
        auto a3 = scene->create_child<smlt::Actor>(box_);

        a1->transform->set_translation(Vec3(0, 0, -5));
        a2->transform->set_translation(Vec3(0, 0, -5));
        a3->transform->set_translation(Vec3(0, 0, -5));

        partitioner->adopt_children(a1, a2, a3);

        batcher::RenderQueue queue;
        queue.reset(partitioner, window->renderer.get(), camera);

        Viewport viewport;
        partitioner->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        auto all_visible_count = queue.renderable_count();

        queue.clear();

        a2->destroy();

        partitioner->generate_renderables(&queue, camera, &viewport, DETAIL_LEVEL_NEAREST);

        assert_true(queue.renderable_count() < all_visible_count);
    }

private:
    StagePtr stage_;
    MeshPtr box_;
};

}
