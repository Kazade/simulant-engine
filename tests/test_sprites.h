#pragma once

#include "simulant/simulant.h"

namespace {

class SpriteTests : public smlt::test::SimulantTestCase {
public:
    void test_set_alpha() {
        auto stage = scene->create_node<smlt::Stage>();
        auto sprite = stage->sprites->new_sprite();

        sprite->set_alpha(0.5f);

        assert_equal(sprite->alpha(), 0.5f);
    }
};

}
