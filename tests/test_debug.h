#pragma once

#include "simulant/simulant.h"


namespace {

using namespace smlt;

class DebugTests : public test::SimulantTestCase {
public:
    void test_debug_can_be_created() {
        auto debug = scene->create_child<Debug>();
        assert_is_not_null(debug);
        assert_equal(scene->count_nodes_by_type<Debug>(), 1u);
        debug->destroy();
        assert_equal(scene->count_nodes_by_type<Debug>(), 0u);
    }
};

}
