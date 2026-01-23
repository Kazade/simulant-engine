#pragma once

#include "simulant/simulant.h"

#include "../simulant/renderers/utils/vram_alloc.h"

namespace {

class VRAMAllocTests: public smlt::test::SimulantTestCase {
public:
    void test_alloc_count_contiguous() {
        uint8_t buffer[1024 * 512];

        vram_alloc_init(buffer, sizeof(buffer));

        // Size might be less than the buffer if the buffer wasn't aligned
        auto size = vram_alloc_pool_size(buffer);

        assert_equal(vram_alloc_count_free(buffer), size);
        assert_equal(vram_alloc_count_continuous(buffer), size);

        vram_alloc_shutdown(buffer);
    }

    void test_random_allocations() {
        skip_if(get_platform()->name() == "dreamcast",
                "Dreamcast crashes on this test for some reason :(");

        uint8_t buffer[1024 * 768];

        vram_alloc_init(buffer, sizeof(buffer));

        std::vector<std::size_t> allocs = {
            32768, 174762, 174762, 174762, 43690, 2730, 10922,
        };

        std::vector<void*> allocated;

        auto free = vram_alloc_count_free(buffer);

        for(auto v: allocs) {
            auto new_entry = vram_alloc_malloc(buffer, v);
            assert_is_not_null(new_entry);

            allocated.push_back(new_entry);
            auto new_free = vram_alloc_count_free(buffer);
            assert_true(new_free < free);
        }

        vram_alloc_shutdown(buffer);
    }
};
}
