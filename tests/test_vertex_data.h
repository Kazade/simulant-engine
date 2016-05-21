#ifndef TEST_VERTEX_DATA_H
#define TEST_VERTEX_DATA_H

#include "kglt/kglt.h"
#include "kaztest/kaztest.h"

#include "global.h"

class VertexDataTest : public KGLTTestCase {
public:
    void test_offsets() {
        kglt::VertexData::ptr data = kglt::VertexData::create();

        assert_equal(0, (int32_t) data->position_offset());
        assert_equal(sizeof(float) * 3, data->normal_offset());
    }
};

#endif // TEST_VERTEX_DATA_H
