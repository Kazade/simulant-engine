#pragma once

#include "simulant/generic/containers/contiguous_map.h"
#include "simulant/renderers/batching/render_queue.h"
#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/utils/float.h"

namespace {

using namespace smlt;

class RenderQueueTests : public test::SimulantTestCase {
public:
    void set_up() {
        SimulantTestCase::set_up();
        stage_ = scene->create_child<smlt::Stage>();
    }

    void tear_down() {
        stage_->destroy();
    }

    void test_render_group_insertion() {
        typedef ContiguousMultiMap<batcher::RenderGroup, std::size_t> SortedRenderables;

        batcher::RenderGroup key1;
        batcher::RenderGroup key2;
        batcher::RenderGroup key3;

        key1.sort_key.s.pass = 0;
        key1.sort_key.s.is_blended = true;
        key1.sort_key.s.distance_to_camera = float10_from_float(739.956055)->i;

        key2.sort_key.s.pass = 0;
        key2.sort_key.s.is_blended = true;
        key2.sort_key.s.distance_to_camera = float10_from_float(1523.241211)->i;

        key3.sort_key.s.pass = 0;
        key3.sort_key.s.is_blended = true;
        key3.sort_key.s.distance_to_camera = float10_from_float(1518.375244)->i;

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
        batcher::RenderGroup pass0_unblended_100_tex1 = {
            batcher::generate_render_group_key(RENDER_PRIORITY_MAIN, 0, false,
                                               100.0f, 0, 0)};

        batcher::RenderGroup pass0_unblended_10_tex1 = {
            batcher::generate_render_group_key(RENDER_PRIORITY_MAIN, 0, false,
                                               10.0f, 0, 0)};

        batcher::RenderGroup pass0_blended_100_tex1 = {
            batcher::generate_render_group_key(RENDER_PRIORITY_MAIN, 0, true,
                                               100.0f, 0, 0)};

        batcher::RenderGroup pass0_blended_10_tex1 = {
            batcher::generate_render_group_key(RENDER_PRIORITY_MAIN, 0, true,
                                               10.0f, 0, 0)};

        batcher::RenderGroup pass1_blended_10_tex1 = {
            batcher::generate_render_group_key(RENDER_PRIORITY_MAIN, 1, true,
                                               10.0f, 0, 0)};

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
