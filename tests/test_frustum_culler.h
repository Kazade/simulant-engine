#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/nodes/frustum_culler.h"

namespace {

using namespace smlt;

class FrustumCullerTests : public test::SimulantTestCase {
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
        FrustumCuller* partitioner = scene->create_child<FrustumCuller>();

        auto camera = scene->create_child<smlt::Camera3D>();
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
        partitioner->generate_renderables(&queue, camera, &viewport,
                                          DETAIL_LEVEL_NEAREST, nullptr, 0);

        assert_true(queue.renderable_count() > 0);
    }

    void test_nodes_returned_if_never_culled() {
        FrustumCuller* partitioner = scene->create_child<FrustumCuller>();

        auto camera = scene->create_child<smlt::Camera3D>();
        auto a1 = scene->create_child<smlt::Actor>(box_);
        a1->set_parent(partitioner);
        a1->transform->set_translation(Vec3(0, 0, 100));

        batcher::RenderQueue queue;
        queue.reset(partitioner, window->renderer.get(), camera);

        Viewport viewport;
        partitioner->generate_renderables(&queue, camera, &viewport,
                                          DETAIL_LEVEL_NEAREST, nullptr, 0);

        /* Not visible */
        assert_equal(queue.renderable_count(), 0u);

        a1->set_cullable(false);

        partitioner->generate_renderables(&queue, camera, &viewport,
                                          DETAIL_LEVEL_NEAREST, nullptr, 0);

        /* Now visible */
        assert_true(queue.renderable_count() > 0);
    }

    void test_destroyed_nodes_not_returned() {
        FrustumCuller* partitioner = scene->create_child<FrustumCuller>();

        auto camera = scene->create_child<smlt::Camera3D>();

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
        partitioner->generate_renderables(&queue, camera, &viewport,
                                          DETAIL_LEVEL_NEAREST, nullptr, 0);

        auto all_visible_count = queue.renderable_count();

        queue.clear();

        a2->destroy();

        partitioner->generate_renderables(&queue, camera, &viewport,
                                          DETAIL_LEVEL_NEAREST, nullptr, 0);

        assert_true(queue.renderable_count() < all_visible_count);
    }

    void test_compositor_does_not_render_actors_twice() {
        /* Regression test.
         *
         * FrustumCuller (a Partitioner) generates renderables for its whole
         * descendant subtree itself, in do_generate_renderables(), and signals
         * this via generates_renderables_for_descendents() returning true, so
         * that the BFS traversal driving the real frame pipeline
         * (Compositor::run_layer -> traverse_bfs) knows not to *also* visit
         * those descendants directly.
         *
         * A prior bug meant traverse_bfs ignored that signal and unconditionally
         * queued every node's children, so each actor under a FrustumCuller was
         * generated twice per frame: once via the culler's own loop, once via
         * the BFS revisiting it directly (bypassing the frustum test on that
         * second pass too). That made the on-screen and submitted triangle
         * count go *up*, not down, when a FrustumCuller was introduced.
         *
         * This test drives the actual Compositor (not FrustumCuller in
         * isolation, which the tests above already cover) and checks the
         * number of renderables it submits for a frame matches exactly what
         * the culler itself reports -- not double. */
        FrustumCuller* partitioner = scene->create_child<FrustumCuller>();
        auto camera = scene->create_child<smlt::Camera3D>();

        auto a1 = scene->create_child<smlt::Actor>(box_);
        auto a2 = scene->create_child<smlt::Actor>(box_);
        auto a3 = scene->create_child<smlt::Actor>(box_);

        a1->transform->set_translation(Vec3(0, 0, -5));
        a2->transform->set_translation(Vec3(0, 0, -5));
        a3->transform->set_translation(Vec3(0, 0, -5));

        partitioner->adopt_children(a1, a2, a3);

        /* Ground truth: how many renderables should 3 visible actors produce? */
        batcher::RenderQueue queue;
        queue.reset(partitioner, window->renderer.get(), camera);

        Viewport viewport;
        partitioner->generate_renderables(&queue, camera, &viewport,
                                          DETAIL_LEVEL_NEAREST, nullptr, 0);

        auto expected_count = queue.renderable_count();
        assert_true(expected_count > 0);

        /* Snapshot whatever the compositor reports before our own pipeline
         * exists, so the assertion below doesn't depend on any other active
         * pipeline left over from elsewhere. */
        application->run_frame();
        auto baseline = application->stats->subactors_rendered();

        /* Now render the SAME subtree through the real frame pipeline. This is
         * the path that was broken: Compositor::run_layer() calling
         * traverse_bfs() over `partitioner`. */
        auto pipeline = window->compositor->create_layer(partitioner, camera);
        pipeline->activate();

        application->run_frame();

        auto rendered = application->stats->subactors_rendered() - baseline;
        assert_equal(expected_count, (std::size_t)rendered);

        pipeline->destroy();
    }

private:
    StagePtr stage_;
    MeshPtr box_;
};

}
