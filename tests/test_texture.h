#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class TextureTests : public smlt::test::SimulantTestCase {
public:

    void test_locking() {
        auto tex = window->shared_assets->new_texture(8, 8);
        {
            auto lock = tex->lock();
            assert_false(tex->try_lock());
        }

        assert_true(tex->try_lock());
    }

    void test_conversion_from_r8_to_rgba4444() {
        auto tex = window->shared_assets->new_texture(2, 2, TEXTURE_FORMAT_R8);

        auto& data = tex->data();
        data[0] = 255;
        data[1] = 128;
        data[2] = 0;
        data[3] = 255;

        assert_equal(4u, data.size());

        // Should convert each pixel to: {1, 0, 0, v}
        tex->convert(
            TEXTURE_FORMAT_RGBA4444,
            {{TEXTURE_CHANNEL_ONE, TEXTURE_CHANNEL_ZERO, TEXTURE_CHANNEL_GREEN, TEXTURE_CHANNEL_RED}}
        );

        assert_equal(8u, data.size());

        auto expected1 = 0b1111000000001111;
        uint16_t* first_pixel = (uint16_t*) &data[0];
        assert_equal(*first_pixel, expected1);

        auto expected2 = 0b1111000000000111;
        uint16_t* second_pixel = (uint16_t*) &data[2];
        assert_equal(*second_pixel, expected2);

        auto expected3 = 0b1111000000000000;
        uint16_t* third_pixel = (uint16_t*) &data[4];
        assert_equal(*third_pixel, expected3);
    }
};


}
