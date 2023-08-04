#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace  {

using namespace smlt;

class BinaryTests : public smlt::test::SimulantTestCase {
public:
    void test_loading_binaries() {
        auto bin = application->shared_assets->new_binary_from_file("textures/4444_vq.dtex");
        assert_true(bin);
        assert_true(bin->data_size_in_bytes());

        bin->set_name("bin");

        auto other = application->shared_assets->find_binary("bin");
        assert_true(other);
        assert_equal(bin, other);

        assert_false(application->shared_assets->find_binary("???"));

        assert_equal(application->shared_assets->binary_count(), 1u);

        application->shared_assets->destroy_binary(bin->id());
        bin.reset();
        other.reset();
        application->shared_assets->run_garbage_collection();

        assert_equal(application->shared_assets->binary_count(), 0u);
    }
};

}
