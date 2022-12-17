#pragma once

#include "simulant/simulant.h"


namespace {

class VFSTests : public smlt::test::SimulantTestCase {
public:
    void test_vfs_caching() {
        auto vfs = application->vfs.get();
        vfs->clear_location_cache();
        assert_equal(vfs->location_cache_size(), 0u);

        auto f0 = vfs->locate_file("simulant/textures/simulant-icon.png");
        assert_equal(vfs->location_cache_size(), 1u);

        auto f1 = vfs->locate_file("simulant/textures/simulant-icon.png");
        assert_equal(vfs->location_cache_size(), 1u);

        assert_true(f0);
        assert_true(f1);
        assert_equal(f0.value(), f1.value());

        // Should purge the cache
        vfs->add_search_path("/bananas/");
        assert_equal(vfs->location_cache_size(), 0u);

        vfs->locate_file("simulant/textures/simulant-icon.png");
        assert_equal(vfs->location_cache_size(), 1u);
    }

    void test_read_blocking() {
        auto vfs = application->vfs.get();
        auto path = vfs->locate_file("simulant/textures/simulant-icon.png");
        assert_true(path);

        assert_false(vfs->read_blocking_enabled());
        vfs->enable_read_blocking();
        assert_true(vfs->read_blocking_enabled());

        path = vfs->locate_file("simulant/textures/simulant-icon.png");
        assert_false(path);

        vfs->disable_read_blocking();
        path = vfs->locate_file("simulant/textures/simulant-icon.png");
        assert_true(path);
    }
};

}
