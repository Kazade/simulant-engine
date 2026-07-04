#pragma once

#include <fstream>
#include <sstream>

#include <simulant/test.h>
#include <simulant/loaders/dtex_loader.h>

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
        auto tex = application->shared_assets->load_texture(
            "assets/samples/1555_vq.dtex"
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
        auto tex = application->shared_assets->load_texture(
            "assets/samples/4444_vq.dtex"
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
        auto tex = application->shared_assets->load_texture(
            "assets/samples/565_vq.dtex"
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
        auto tex = application->shared_assets->load_texture(
            "assets/samples/1555.dtex"
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
        auto tex = application->shared_assets->load_texture(
            "assets/samples/4444.dtex"
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
        auto tex = application->shared_assets->load_texture(
            "assets/samples/565.dtex"
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

    /* Embedded (data-uri) .dtex textures, as produced by the SMLT_dtex_texture
     * gltf extension, are loaded from an in-memory stream rather than a file.
     * This exercises DTEXLoader's memory do_load() overload directly. */
    void test_load_from_memory() {
        std::ifstream file("assets/samples/4444.dtex", std::ios::binary);
        std::string bytes((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        assert_true(!bytes.empty());

        auto stream = std::make_shared<std::istringstream>(bytes, std::ios::binary);
        smlt::loaders::DTEXLoader loader(smlt::Path("memory.dtex"), stream);

        auto tex = application->shared_assets->create_texture(8, 8);
        assert_true(loader.into(*tex));

        tex->set_free_data_mode(smlt::TEXTURE_FREE_DATA_NEVER);
        tex->flush();

        assert_equal(tex->width(), 8);
        assert_equal(tex->height(), 8);

#ifndef __DREAMCAST__
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_4444);
#else
        assert_equal(tex->format(), smlt::TEXTURE_FORMAT_ARGB_1US_4444_TWID);
#endif
        uint16_t texel = *((uint16_t*) &tex->data()[0]);
        assert_equal(texel, 0xFF0F);
    }
};

}
