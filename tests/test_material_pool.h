#pragma once

#include <cstdint>

#include "../simulant/assets/materials/core/material_value_pool.h"
#include <simulant/test.h>

namespace {

using namespace smlt;

class DynamicAlignedAllocatorTest: public test::SimulantTestCase {
public:
    void test_free_blocks() {
        auto allocator = DynamicAlignedAllocator<16>();

        uint8_t *ptr = nullptr, *s = nullptr, *m = nullptr, *l = nullptr,
                *xs1 = nullptr, *xs2 = nullptr;

        struct Updater {
            Updater(uint8_t*& ptr, uint8_t*& s, uint8_t*& m, uint8_t*& l,
                    uint8_t*& xs1, uint8_t*& xs2) :
                ptr_(ptr), s_(s), m_(m), l_(l), xs1_(xs1), xs2_(xs2) {}

            static void update(uint8_t* old_base, uint8_t* new_base,
                               void* _this) {
                auto self = reinterpret_cast<Updater*>(_this);
                self->ptr_ = new_base + (self->ptr_ - old_base);
                self->s_ = new_base + (self->s_ - old_base);
                self->m_ = new_base + (self->m_ - old_base);
                self->l_ = new_base + (self->l_ - old_base);
                self->xs1_ = new_base + (self->xs1_ - old_base);
                self->xs2_ = new_base + (self->xs2_ - old_base);
            }

            uint8_t*& ptr_;
            uint8_t*& s_;
            uint8_t*& m_;
            uint8_t*& l_;
            uint8_t*& xs1_;
            uint8_t*& xs2_;
        };

        Updater updater(ptr, s, m, l, xs1, xs2);

        allocator.set_realloc_callback(Updater::update, &updater);

        ptr = allocator.allocate(16);
        assert_true(ptr);
        assert_true(uintptr_t(ptr) % 16 == 0);

        assert_equal(allocator.capacity(), 16u);
        assert_equal(allocator.used(), 16u);
        assert_equal(allocator._block_count(), 1u);

        allocator.deallocate(ptr);

        assert_equal(allocator.capacity(), 16u);
        assert_equal(allocator.used(), 0u);
        assert_equal(allocator._block_count(), 1u);

        s = allocator.allocate(16);
        m = allocator.allocate(24);
        l = allocator.allocate(32);

        assert_true(s);
        assert_true(m);
        assert_true(l);

        assert_true(uintptr_t(s) % 16 == 0);
        assert_true(uintptr_t(m) % 16 == 0);
        assert_true(uintptr_t(l) % 16 == 0);

        assert_equal(allocator.capacity(), 80u);
        assert_equal(allocator.used(), 72u);

        assert_equal(allocator._block_count(), 3u);

        allocator.deallocate(s);

        // Should go in the first block
        xs1 = allocator.allocate(8);

        // No more blocks than before
        assert_equal(allocator._block_count(), 3u);

        assert_true(xs1);
        assert_equal(allocator.capacity(), 80u);
        assert_equal(allocator.used(), 64u);

        // Requires a new block to stay aligned
        xs2 = allocator.allocate(8);

        assert_true(xs2);
        assert_equal(allocator.capacity(), 96u);
        assert_equal(allocator.used(), 64 + 8u);

        allocator.deallocate(xs1);
        allocator.deallocate(m);
        allocator.deallocate(l);
        allocator.deallocate(xs2);

        assert_equal(allocator.capacity(), 96u);
        assert_equal(allocator.used(), 0u);
    }
};

class MaterialValuePoolTest: public test::TestCase {
public:
    void test_basic_usage() {
        auto pool = MaterialValuePool();
        auto vec3 = pool.get_or_create_value(Vec3(1, 2, 3));

        auto color = pool.get_or_create_value(Vec4(0.33, 0.33, 0.33, 0.33));
        auto color2 = pool.get_or_create_value(Vec4(0.33, 0.33, 0.33, 0.33));

        assert_equal(color.get<Vec4>(), color2.get<Vec4>());

        assert_equal(color.refcount(), 2u);
        color.reset();
        assert_equal(color2.refcount(), 1u);
        color2.reset();

        vec3.reset(); // Make sure all pointers are cleaned up
    }

    void test_refcounting() {
        auto pool = MaterialValuePool();
        auto vec3 = pool.get_or_create_value(Vec3(1, 2, 3));
        assert_equal(vec3.refcount(), 1u);
        auto p2 = vec3;
        assert_equal(vec3.refcount(), 2u);
        assert_equal(p2.refcount(), 2u);

        vec3.reset();
        assert_false(vec3);
        assert_true(p2);

        assert_equal(p2.refcount(), 1u);
    }

    void test_double_clear() {
        auto pool = MaterialValuePool();
        auto a = pool.get_or_create_value(Vec3(1, 2, 3));
        auto b = pool.get_or_create_value(Vec3(2, 2, 3));
        auto c = pool.get_or_create_value(Vec3(3, 2, 3));

        pool.clear();
        pool.clear();

        assert_false(a);
        assert_false(b);
        assert_false(c);
    }
};

} // namespace
