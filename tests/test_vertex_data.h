#ifndef TEST_VERTEX_DATA_H
#define TEST_VERTEX_DATA_H

#include "simulant/simulant.h"
#include "simulant/test.h"


class IndexDataTest : public smlt::test::SimulantTestCase {
public:
    void test_clear() {
        smlt::IndexData data(smlt::INDEX_TYPE_16_BIT);
        data.index(0); data.index(1); data.index(2);
        assert_equal(data.count(), 3u);

        data.clear();
        assert_equal(data.count(), 0u);
    }
};

class VertexDataTest : public smlt::test::SimulantTestCase {
public:
    void test_offsets() {
        smlt::VertexSpecification spec = {
            smlt::VERTEX_ATTRIBUTE_3F,
            smlt::VERTEX_ATTRIBUTE_3F,
            smlt::VERTEX_ATTRIBUTE_2F
        };

        smlt::VertexData::ptr data = smlt::VertexData::create(spec);

        assert_equal(0, (int32_t) data->vertex_specification().position_offset());
        assert_equal(sizeof(float) * 3u, data->vertex_specification().texcoord0_offset());
        assert_equal(sizeof(float) * 5, data->vertex_specification().normal_offset());
    }

    void test_moving_cursor() {
        smlt::VertexSpecification spec = {
            smlt::VERTEX_ATTRIBUTE_3F,
            smlt::VERTEX_ATTRIBUTE_3F,
            smlt::VERTEX_ATTRIBUTE_2F
        };

        smlt::VertexData::ptr data = smlt::VertexData::create(spec);
        auto stride = data->stride();

        data->position(0, 0, 0);
        data->move_next();
        data->position(1, 1, 1);
        data->move_next();
        data->position(2, 2, 2);
        data->move_next();

        assert_equal(data->cursor_position(), 3);
        assert_equal(data->cursor_offset(), 3 * (int32_t) stride);

        data->move_to(1);

        assert_equal(data->cursor_position(), 1);
        assert_equal(data->cursor_offset(), 1 * (int32_t) stride);

        data->move_by(1);

        assert_equal(data->cursor_position(), 2);
        assert_equal(data->cursor_offset(), 2 * (int32_t) stride);
    }

    void test_clone_into() {
        smlt::VertexSpecification spec = { smlt::VERTEX_ATTRIBUTE_2F };

        smlt::VertexData source(spec);

        for(auto i = 0; i < 5; ++i) {
            source.position(i, 0);
            source.move_next();
        }

        smlt::VertexData dest(spec);

        // Should be wiped by clone
        dest.position(-1, -1);

        // Clone the data
        assert_true(source.clone_into(dest));

        // Check the data is valid
        assert_equal(dest.count(), source.count());
        assert_equal(smlt::Vec2(0, 0), *dest.position_at<smlt::Vec2>(0));
        assert_equal(smlt::Vec2(1, 0), *dest.position_at<smlt::Vec2>(1));
        assert_equal(smlt::Vec2(2, 0), *dest.position_at<smlt::Vec2>(2));
        assert_equal(smlt::Vec2(3, 0), *dest.position_at<smlt::Vec2>(3));
        assert_equal(smlt::Vec2(4, 0), *dest.position_at<smlt::Vec2>(4));
        assert_equal(source.stride(), dest.stride());

        assert_equal(dest.cursor_position(), 0);
    }

    void test_basic_usage() {
        smlt::VertexSpecification spec = smlt::VertexSpecification::POSITION_AND_DIFFUSE;
        spec.texcoord0_attribute = smlt::VERTEX_ATTRIBUTE_2F;

        smlt::VertexData data(spec);

        assert_equal(0u, data.data_size());

        data.position(0, 0, 0);
        data.tex_coord0(1, 1);

        // sizeof(float) * 5 + sizeof(byte) * 4 - but rounded to the nearest 16 byte boundary == 32
        assert_equal(32u, data.data_size());
        data.move_next();
        data.position(0, 0, 0);
        data.tex_coord0(2, 2);
        data.done();

        // sizeof(float) * 10 + sizeof(byte) * 8, but rounded to the nearest 16 byte boundary == 64
        assert_equal(64u, data.data_size());
    }
};

#endif // TEST_VERTEX_DATA_H
