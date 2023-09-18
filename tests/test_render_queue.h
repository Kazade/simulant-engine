#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/renderers/batching/render_queue.h"
#include "simulant/generic/containers/contiguous_map.h"

namespace {

using namespace smlt;

class RenderQueueTests : public smlt::test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = scene->create_node<smlt::Stage>();
    }

    void tear_down() {
        scene->destroy_stage(stage_->id());
    }

    void test_render_group_insertion() {
        typedef ContiguousMultiMap<batcher::RenderGroup, std::size_t> SortedRenderables;

        batcher::RenderGroup key1;
        batcher::RenderGroup key2;
        batcher::RenderGroup key3;

        key1.sort_key.pass = 0;
        key1.sort_key.is_blended = true;
        key1.sort_key.distance_to_camera = 739.956055;

        key2.sort_key.pass = 0;
        key2.sort_key.is_blended = true;
        key2.sort_key.distance_to_camera = 1523.241211;

        key3.sort_key.pass = 0;
        key3.sort_key.is_blended = true;
        key3.sort_key.distance_to_camera = 1518.375244;

        SortedRenderables queue;

        for(int i = 0; i < 16; ++i) {
            queue.insert(key1, 0);
        }

        queue.insert(key2, 1);
        queue.insert(key2, 1);
        queue.insert(key2, 1);

        queue.insert(key3, 1);

        assert_equal(queue.size(), 20u);
        auto count = 0;

        for(auto& p: queue) {
            _S_UNUSED(p);
            count++;
        }

        assert_equal(count, 20);
    }

    void test_render_group_key_generation() {
        batcher::RenderGroup pass0_unblended_100_tex1 = {batcher::generate_render_group_key(
            0, false, 100.0f
        )};

        batcher::RenderGroup  pass0_unblended_10_tex1 = {batcher::generate_render_group_key(
            0, false, 10.0f
        )};

        batcher::RenderGroup  pass0_blended_100_tex1 = {batcher::generate_render_group_key(
            0, true, 100.0f
        )};

        batcher::RenderGroup  pass0_blended_10_tex1 = {batcher::generate_render_group_key(
            0, true, 10.0f
        )};

        batcher::RenderGroup  pass1_blended_10_tex1 = {batcher::generate_render_group_key(
            1, true, 10.0f
        )};

        assert_true(pass0_unblended_100_tex1 < pass0_blended_100_tex1);
        assert_true(pass0_unblended_100_tex1 < pass0_blended_10_tex1);

        // Unblended are rendered front-to-back
        assert_true(pass0_unblended_10_tex1 < pass0_unblended_100_tex1);

        // Blended are renderered back to front
        assert_true(pass0_blended_100_tex1 < pass0_blended_10_tex1);

        // Pass always takes precedence
        assert_true(pass0_blended_10_tex1 < pass1_blended_10_tex1);
        assert_true(pass0_blended_100_tex1 < pass1_blended_10_tex1);
    }

private:
    StagePtr stage_;

};

}
