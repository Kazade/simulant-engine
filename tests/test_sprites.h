#pragma once

#include "global.h"

namespace {

class SpriteTests : public SimulantTestCase {
public:
    void test_set_alpha() {
        auto stage = window->new_stage();
        auto sprite = stage->sprites->new_sprite();

        sprite->set_alpha(0.5f);

        assert_equal(sprite->alpha(), 0.5f);
    }
};

}
