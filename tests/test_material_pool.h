#pragma once

#include "../simulant/assets/materials/core/material_value_pool.h"
#include <simulant/test.h>

namespace {

using namespace smlt;

class DynamicAlignedAllocatorTest: public test::SimulantTestCase {
public:
    void test_free_blocks() {
        auto allocator = _mat_pool::DynamicAlignedAllocator<16>();

        auto ptr = allocator.allocate(16);
        assert_true(ptr);
        assert_true(uintptr_t(ptr) % 16 == 0);

        assert_equal(allocator.capacity(), 16u);
        assert_equal(allocator.used(), 16u);
        assert_equal(allocator._block_count(), 1u);

        allocator.deallocate(ptr);

        assert_equal(allocator.capacity(), 16);
        assert_equal(allocator.used(), 0);
        assert_equal(allocator._block_count(), 1u);

        auto s = allocator.allocate(16);
        auto m = allocator.allocate(24);
        auto l = allocator.allocate(32);

        assert_true(s);
        assert_true(m);
        assert_true(l);

        assert_true(uintptr_t(s) % 16 == 0);
        assert_true(uintptr_t(m) % 16 == 0);
        assert_true(uintptr_t(l) % 16 == 0);

        assert_equal(allocator.capacity(), 80);
        assert_equal(allocator.used(), 72);

        assert_equal(allocator._block_count(), 3u);

        allocator.deallocate(s);
        auto xs1 = allocator.allocate(8); // Should go in the first block

        assert_equal(allocator._block_count(),
                     3u); // No more blocks than before

        assert_true(xs1);
        assert_equal(allocator.capacity(), 96);
        assert_equal(allocator.used(), 64);

        auto xs2 =
            allocator.allocate(8); // Requires a new block to stay aligned

        assert_true(xs2);
        assert_equal(allocator.capacity(), 96 + 16);
        assert_equal(allocator.used(), 64 + 8);
    }
};

} // namespace
