#pragma once

#include "simulant/simulant.h"

namespace {

using namespace smlt;

class SpriteTests : public smlt::test::SimulantTestCase {
public:
    void test_set_alpha() {
        auto sprite = scene->create_child<Sprite>();

        sprite->set_alpha(0.5f);

        assert_equal(sprite->alpha(), 0.5f);
    }
};

}
