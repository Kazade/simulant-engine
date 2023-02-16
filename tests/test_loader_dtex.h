#pragma once

#include <simulant/test.h>

namespace {

/* The .dtex file format supports numerous image formats. We currently
 * support:
 *
 *  - VQ compressed ARGB 1555, 4444, 565 (twiddled)
 *  - non-compressed ARGB 1555, 4444, 565 (twiddled)
 */
class DTEXLoaderTests : public smlt::test::SimulantTestCase {
public:

    void test_1555_vq() {
        auto tex = application->shared_assets->new_texture_from_file(
            "textures/1555_vq.dtex"
        );

        tex->set_free_data_mode(smlt::TEXTURE_FREE_DATA_NEVER);

        assert_equal(tex->width(), 8);
        assert_equal(tex->height(), 8);

        tex->flush();

#ifndef __DREAMCAST__
        /* Only the Dreamcast supports VQ compression and twiddling */
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_1555);
        assert_equal(tex->data_size(), 8 * 8 * 2u);
        uint16_t texel = *((uint16_t*) &tex->data()[0]);
        assert_equal(texel, 0xFC1F);
#else
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID);
#endif
    }

    void test_4444_vq() {
        auto tex = application->shared_assets->new_texture_from_file(
            "textures/4444_vq.dtex"
        );

        tex->set_free_data_mode(smlt::TEXTURE_FREE_DATA_NEVER);

        assert_equal(tex->width(), 8);
        assert_equal(tex->height(), 8);

        tex->flush();

#ifndef __DREAMCAST__
        /* Only the Dreamcast supports VQ compression and twiddling */
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_4444);
        assert_equal(tex->data_size(), 8 * 8 * 2u);
        uint16_t texel = *((uint16_t*) &tex->data()[0]);
        assert_equal(texel, 0xFF0F);
#else
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID);
#endif
    }

    void test_565_vq() {
        auto tex = application->shared_assets->new_texture_from_file(
            "textures/565_vq.dtex"
        );

        tex->set_free_data_mode(smlt::TEXTURE_FREE_DATA_NEVER);

        assert_equal(tex->width(), 8);
        assert_equal(tex->height(), 8);

        tex->flush();

#ifndef __DREAMCAST__
        /* Only the Dreamcast supports VQ compression and twiddling */
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_RGB_1US_565);
        assert_equal(tex->data_size(), 8 * 8 * 2u);
        uint16_t texel = *((uint16_t*) &tex->data()[0]);
        assert_equal(texel, 0xF81F);
#else
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_RGB_1US_565_VQ_TWID);
#endif
    }

    void test_1555() {
        auto tex = application->shared_assets->new_texture_from_file(
            "textures/1555.dtex"
        );

        tex->set_free_data_mode(smlt::TEXTURE_FREE_DATA_NEVER);

        assert_equal(tex->width(), 8);
        assert_equal(tex->height(), 8);

        tex->flush();

#ifndef __DREAMCAST__
        /* Only the Dreamcast supports VQ compression and twiddling */
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_1555);
        assert_equal(tex->data_size(), 8 * 8 * 2u);
#else
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_1555_TWID);
#endif
        uint16_t texel = *((uint16_t*) &tex->data()[0]);
        assert_equal(texel, 0xFC1F);
    }

    void test_4444() {
        auto tex = application->shared_assets->new_texture_from_file(
            "textures/4444.dtex"
        );

        tex->set_free_data_mode(smlt::TEXTURE_FREE_DATA_NEVER);

        assert_equal(tex->width(), 8);
        assert_equal(tex->height(), 8);

        tex->flush();

#ifndef __DREAMCAST__
        /* Only the Dreamcast supports VQ compression and twiddling */
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_4444);
        assert_equal(tex->data_size(), 8 * 8 * 2u);
#else
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_4444_TWID);
#endif

        uint16_t texel = *((uint16_t*) &tex->data()[0]);
        assert_equal(texel, 0xFF0F);
    }

    void test_565() {
        auto tex = application->shared_assets->new_texture_from_file(
            "textures/565.dtex"
        );

        tex->set_free_data_mode(smlt::TEXTURE_FREE_DATA_NEVER);

        assert_equal(tex->width(), 8);
        assert_equal(tex->height(), 8);

        tex->flush();

#ifndef __DREAMCAST__
        /* Only the Dreamcast supports VQ compression and twiddling */
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_RGB_1US_565);
        assert_equal(tex->data_size(), 8 * 8 * 2u);
#else
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_RGB_1US_565_TWID);
#endif
        uint16_t texel = *((uint16_t*) &tex->data()[0]);
        assert_equal(texel, 0xF81F);
    }
};

}
