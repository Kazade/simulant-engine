#pragma once

#include "global.h"

namespace {

class TextureTests : public SimulantTestCase {
public:

    void test_locking() {
        auto tex = window->shared_assets->new_texture().fetch();
        {
            auto lock = tex->lock();
            assert_false(tex->try_lock());
        }

        assert_true(tex->try_lock());
    }

};


}
