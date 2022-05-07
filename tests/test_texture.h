
#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

class TextureTests : public smlt::test::SimulantTestCase {
public:
    void test_flush() {
        auto tex = application->shared_assets->new_texture(
            8, 8,
            TEXTURE_FORMAT_R_1UB_8
        );

        std::vector<uint8_t> data(8 * 8, 255);
        tex->set_data(data);

        assert_true(tex->has_data());
        tex->flush();
        application->idle->execute();

        assert_false(tex->has_data());
    }

    void test_transaction_api() {
        TexturePtr tex = application->shared_assets->new_texture(0, 0);

        application->run_frame();
        assert_false(tex->_data_dirty());
        assert_false(tex->_params_dirty());

        tex->resize(64, 64);

        assert_true(tex->_data_dirty());
        assert_false(tex->_params_dirty());
    }

    void test_conversion_from_r8_to_rgba4444() {
        auto tex = application->shared_assets->new_texture(2, 2, TEXTURE_FORMAT_R_1UB_8);

        auto data = tex->data_copy();
        data[0] = 255;
        data[1] = 128;
        data[2] = 0;
        data[3] = 255;

        assert_equal(4u, data.size());

        tex->set_data(data);

        // Should convert each pixel to: {1, 0, 0, v}
        tex->convert(
            TEXTURE_FORMAT_RGBA_1US_4444,
            {{TEXTURE_CHANNEL_ONE, TEXTURE_CHANNEL_ZERO, TEXTURE_CHANNEL_GREEN, TEXTURE_CHANNEL_RED}}
        );

        assert_equal(8u, tex->data_size());

        auto expected1 = 0b1111000000001111;
        uint16_t* first_pixel = (uint16_t*) &tex->data()[0];
        assert_equal(*first_pixel, expected1);

        auto expected2 = 0b1111000000000111;
        uint16_t* second_pixel = (uint16_t*) &tex->data()[2];
        assert_equal(*second_pixel, expected2);

        auto expected3 = 0b1111000000000000;
        uint16_t* third_pixel = (uint16_t*) &tex->data()[4];
        assert_equal(*third_pixel, expected3);
    }

    void test_paletted_textures() {
        auto tex = application->shared_assets->new_texture(2, 2, TEXTURE_FORMAT_RGB565_PALETTED4);

        assert_true(tex->is_paletted_format());
        assert_equal(tex->data_size(), ((tex->width() * tex->height()) / 2) + tex->palette_size());

        uint8_t data [] = {
            // Palette (2 bytes per colour, 16 colours)
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 0,
            0, 1, 2, 3  /* Indexes */
        };

        tex->set_data(data, tex->data_size());
        tex->flush();

        assert_false(tex->has_data());

        uint8_t new_palette [] = {
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
            1, 0,
        };

        assert_equal(sizeof(new_palette), tex->palette_size());

        // FIXME: tex->update_palette(new_palette);
    }
};


}
