#ifndef TEST_VERTEX_DATA_H
#define TEST_VERTEX_DATA_H

#include "kglt/kglt.h"
#include <kaztest/kaztest.h>

#include "global.h"

class VertexDataTest : public TestCase {
public:
    void set_up() {
        if(!window) {
            window = kglt::Window::create();
            window->set_logging_level(kglt::LOG_LEVEL_NONE);
        }

        //window->reset();
    }

    void test_offsets() {
        kglt::VertexData::ptr data = kglt::VertexData::create(window->scene());

        assert_equal(0, (int32_t) data->position_offset());
        assert_equal(sizeof(float) * 3, data->normal_offset());
    }
};

#endif // TEST_VERTEX_DATA_H
