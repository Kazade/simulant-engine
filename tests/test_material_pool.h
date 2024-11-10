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

        assert_equal(allocator.capacity(), 16);
        assert_equal(allocator.used(), 16);

        allocator.deallocate(ptr);

        assert_equal(allocator.capacity(), 16);
        assert_equal(allocator.used(), 0);

        auto s = allocator.allocate(16);
        auto m = allocator.allocate(24);
        auto l = allocator.allocate(32);

        assert_true(s);
        assert_true(m);
        assert_true(l);

        assert_true(uintptr_t(s) % 16 == 0);
        assert_true(uintptr_t(m) % 16 == 0);
        assert_true(uintptr_t(l) % 16 == 0);

        assert_equal(allocator.capacity(), 64);
        assert_equal(allocator.used(), 64);
    }
};

} // namespace
