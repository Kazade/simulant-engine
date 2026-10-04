
#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"


namespace {

using namespace smlt;

/* Reference PVR twiddle: interleave v (even bits) and u (odd bits) up to the
 * smaller dimension, then append the remaining bits of the larger one. Built
 * bit by bit, independently of Texture's XOR-based flip, so the flip tests
 * check against the layout rather than against themselves. */
static uint32_t ref_twiddle(uint32_t u, uint32_t v, uint32_t w, uint32_t h) {
    uint32_t mn = std::min(w, h);
    uint32_t bits = 0;
    while((1u << bits) < mn) ++bits;

    uint32_t ret = 0;
    for(uint32_t i = 0; i < bits; ++i) {
        ret |= ((v >> i) & 1) << (2 * i);
        ret |= ((u >> i) & 1) << (2 * i + 1);
    }

    uint32_t rest = (w > h) ? (u >> bits) : (v >> bits);
    return ret | (rest << (2 * bits));
}

/* A texel value unique to each (level, x, y) */
static uint16_t ref_texel(uint32_t level, uint32_t x, uint32_t y) {
    return uint16_t((level << 12) | (y << 6) | x);
}

/* Writes a w x h level of VQ data where every block gets its own codebook
 * entry (first_entry onwards), holding that block's reference texels. */
static void write_vq_level(uint8_t* codebook, uint8_t* indices, uint32_t level,
                           uint32_t w, uint32_t h, uint32_t first_entry) {
    uint32_t bw = w / 2, bh = h / 2;
    for(uint32_t by = 0; by < bh; ++by) {
        for(uint32_t bx = 0; bx < bw; ++bx) {
            uint32_t entry = first_entry + by * bw + bx;
            indices[ref_twiddle(bx, by, bw, bh)] = uint8_t(entry);

            /* Entries are stored (u, v) = (0,0), (0,1), (1,0), (1,1) */
            uint16_t* e = (uint16_t*) (codebook + entry * 8);
            e[0] = ref_texel(level, bx * 2, by * 2);
            e[1] = ref_texel(level, bx * 2, by * 2 + 1);
            e[2] = ref_texel(level, bx * 2 + 1, by * 2);
            e[3] = ref_texel(level, bx * 2 + 1, by * 2 + 1);
        }
    }
}

static uint16_t read_vq_texel(const uint8_t* codebook, const uint8_t* indices,
                              uint32_t w, uint32_t h, uint32_t x, uint32_t y) {
    uint8_t entry = indices[ref_twiddle(x / 2, y / 2, w / 2, h / 2)];
    const uint16_t* e = (const uint16_t*) (codebook + entry * 8);
    return e[(x % 2) * 2 + (y % 2)];
}

class TextureTests : public smlt::test::SimulantTestCase {
public:
    void test_flush() {
        auto tex = application->shared_assets->create_texture(
            8, 8,
            TEXTURE_FORMAT_RGB_3UB_888
        );

        std::vector<uint8_t> data(8 * 8 * 3, 255);
        tex->set_data(data);

        assert_true(tex->has_data());
        tex->flush();
        application->run_frame();

        assert_false(tex->has_data());
    }

    void test_blur() {
        TexturePtr tex = application->shared_assets->create_texture(3, 3, smlt::TEXTURE_FORMAT_R_1UB_8);
        tex->set_auto_upload(false);

        auto data = tex->data_copy();
        data[4] = 255;

        tex->set_data(data);
        tex->blur(BLUR_TYPE_SIMPLE, 1);  // Simple box blur with a radius of 1

        assert_equal(+tex->data()[0], 28);
        assert_equal(+tex->data()[1], 28);
        assert_equal(+tex->data()[2], 28);
        assert_equal(+tex->data()[3], 28);
        assert_equal(+tex->data()[4], 28);
        assert_equal(+tex->data()[5], 28);
        assert_equal(+tex->data()[6], 28);
        assert_equal(+tex->data()[7], 28);
        assert_equal(+tex->data()[8], 28);
    }

    void test_transaction_api() {
        TexturePtr tex = application->shared_assets->create_texture(8, 8);

        application->run_frame();
        assert_false(tex->_data_dirty());
        assert_false(tex->_params_dirty());

        tex->resize(64, 64);

        assert_true(tex->_data_dirty());
        assert_false(tex->_params_dirty());
    }

    void test_conversion_from_r8_to_rgba4444() {
        auto tex = application->shared_assets->create_texture(8, 8, TEXTURE_FORMAT_R_1UB_8);

        auto data = tex->data_copy();
        data[0] = 255;
        data[1] = 128;
        data[2] = 0;
        data[3] = 255;

        assert_equal(64u, data.size());

        tex->set_data(data);

        // Should convert each pixel to: {1, 0, 0, v}
        tex->convert(
            TEXTURE_FORMAT_RGBA_1US_4444,
            {{TEXTURE_CHANNEL_ONE, TEXTURE_CHANNEL_ZERO, TEXTURE_CHANNEL_GREEN, TEXTURE_CHANNEL_RED}}
        );

        assert_equal(128u, tex->data_size());

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
        skip_if(get_platform()->name() == "dreamcast", "Dreamcast doesn't support tex width < 8");

        auto tex = application->shared_assets->create_texture(2, 2, TEXTURE_FORMAT_RGB565_PALETTED4);

        assert_true(tex->is_paletted_format());
        assert_equal(tex->data_size(), ((tex->width() * tex->height()) / 2) + tex->palette_size());

        uint8_t data [] = {
            // Palette (2 bytes per color, 16 colors)
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

    void test_flip_vertically_linear_16bpp() {
        /* Rows of a packed 16-bit format are 2 bytes per texel, not one per
         * channel */
        const uint32_t w = 8, h = 3;
        auto tex = application->shared_assets->create_texture(w, h, TEXTURE_FORMAT_RGB_1US_565);
        tex->set_auto_upload(false);

        std::vector<uint8_t> data(w * h * 2);
        uint16_t* texels = (uint16_t*) &data[0];
        for(uint32_t y = 0; y < h; ++y) {
            for(uint32_t x = 0; x < w; ++x) {
                texels[y * w + x] = ref_texel(0, x, y);
            }
        }
        tex->set_data(data);
        tex->flip_vertically();

        const uint16_t* out = (const uint16_t*) tex->data();
        for(uint32_t y = 0; y < h; ++y) {
            for(uint32_t x = 0; x < w; ++x) {
                assert_equal(out[y * w + x], ref_texel(0, x, h - 1 - y));
            }
        }
    }

    void check_flip_twiddled(uint32_t w, uint32_t h) {
        auto tex = application->shared_assets->create_texture(w, h, TEXTURE_FORMAT_RGB_1US_565_TWID);
        tex->set_auto_upload(false);

        std::vector<uint8_t> data(w * h * 2);
        uint16_t* texels = (uint16_t*) &data[0];
        for(uint32_t y = 0; y < h; ++y) {
            for(uint32_t x = 0; x < w; ++x) {
                texels[ref_twiddle(x, y, w, h)] = ref_texel(0, x, y);
            }
        }
        tex->set_data(data);
        tex->flip_vertically();

        const uint16_t* out = (const uint16_t*) tex->data();
        for(uint32_t y = 0; y < h; ++y) {
            for(uint32_t x = 0; x < w; ++x) {
                assert_equal(out[ref_twiddle(x, y, w, h)], ref_texel(0, x, h - 1 - y));
            }
        }
    }

    void test_flip_vertically_twiddled() {
        check_flip_twiddled(16, 16);
        check_flip_twiddled(32, 8);  /* wide */
        check_flip_twiddled(8, 32);  /* tall */
    }

    void check_flip_vq(uint32_t w, uint32_t h) {
        auto tex = application->shared_assets->create_texture(w, h, TEXTURE_FORMAT_RGB_1US_565_VQ_TWID);
        tex->set_auto_upload(false);

        std::vector<uint8_t> data(tex->data_size(), 0);
        write_vq_level(&data[0], &data[2048], 0, w, h, 0);
        tex->set_data(data);
        tex->flip_vertically();

        for(uint32_t y = 0; y < h; ++y) {
            for(uint32_t x = 0; x < w; ++x) {
                assert_equal(
                    read_vq_texel(tex->data(), tex->data() + 2048, w, h, x, y),
                    ref_texel(0, x, h - 1 - y)
                );
            }
        }
    }

    void test_flip_vertically_vq() {
        check_flip_vq(16, 16);
        check_flip_vq(32, 8);  /* wide */
        check_flip_vq(8, 32);  /* tall */
    }

    void test_flip_vertically_vq_mipmapped() {
        /* Levels are stored smallest first after the codebook: one byte for
         * 1x1, then the indices for 2x2, 4x4, ... up to the full size */
        const uint32_t size = 16;
        auto tex = application->shared_assets->create_texture(size, size, TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP);
        tex->set_auto_upload(false);

        std::vector<uint8_t> data(tex->data_size(), 0);
        uint32_t offset = 2048 + 1, entry = 0;
        for(uint32_t s = 2, level = 1; s <= size; s *= 2, ++level) {
            write_vq_level(&data[0], &data[offset], level, s, s, entry);
            entry += (s / 2) * (s / 2);
            offset += (s / 2) * (s / 2);
        }
        assert_equal(offset, (uint32_t) data.size());

        tex->set_data(data);
        tex->flip_vertically();

        offset = 2048 + 1;
        for(uint32_t s = 2, level = 1; s <= size; s *= 2, ++level) {
            for(uint32_t y = 0; y < s; ++y) {
                for(uint32_t x = 0; x < s; ++x) {
                    assert_equal(
                        read_vq_texel(tex->data(), tex->data() + offset, s, s, x, y),
                        ref_texel(level, x, s - 1 - y)
                    );
                }
            }
            offset += (s / 2) * (s / 2);
        }
    }
};


}
