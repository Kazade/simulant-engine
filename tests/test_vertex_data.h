#ifndef TEST_VERTEX_DATA_H
#define TEST_VERTEX_DATA_H

#include "kglt/kglt.h"
#include "kaztest/kaztest.h"

#include "global.h"

class VertexDataTest : public KGLTTestCase {
public:
    void test_offsets() {
        kglt::VertexData::ptr data = kglt::VertexData::create(
            kglt::VERTEX_ATTRIBUTE_POSITION_3F | kglt::VERTEX_ATTRIBUTE_NORMAL_3F | kglt::VERTEX_ATTRIBUTE_TEXCOORD0_2F
        );

        assert_equal(0, (int32_t) data->position_offset());
        assert_equal(sizeof(float) * 3, data->normal_offset());
        assert_equal(sizeof(float) * 6, data->texcoord0_offset());
    }

    void test_basic_usage() {
        kglt::VertexData data(
            kglt::VERTEX_ATTRIBUTE_POSITION_3F |
            kglt::VERTEX_ATTRIBUTE_TEXCOORD0_2F |
            kglt::VERTEX_ATTRIBUTE_DIFFUSE_4F
        );

        assert_equal(0, data.data_size());

        data.position(0, 0, 0);
        data.tex_coord0(1, 1);

        assert_equal(sizeof(float) * 9, data.data_size());
        data.move_next();
        data.position(0, 0, 0);
        data.tex_coord0(2, 2);
        data.done();

        assert_equal(sizeof(float) * 18, data.data_size());
    }
};

#endif // TEST_VERTEX_DATA_H
