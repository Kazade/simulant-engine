#ifndef TEST_VERTEX_DATA_H
#define TEST_VERTEX_DATA_H

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

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
        auto spec = VertexFormat::standard();

        smlt::VertexData::ptr data = std::make_shared<VertexData>(spec);

        assert_equal(0, (int32_t)data->vertex_specification()
                            .offset(VERTEX_ATTR_NAME_POSITION)
                            .value());
        assert_equal(sizeof(float) * 3, (uint32_t)data->vertex_specification()
                                            .offset(VERTEX_ATTR_NAME_TEXCOORD_0)
                                            .value());
        assert_equal(sizeof(float) * 5, (uint32_t)data->vertex_specification()
                                            .offset(VERTEX_ATTR_NAME_COLOR)
                                            .value());
    }

    void test_colors_dont_overflow() {
        auto spec = VertexFormat::position_and_color();

        smlt::VertexData::ptr data = std::make_shared<VertexData>(spec);
        data->position(0, 0, 0);
        data->color(smlt::Color(1.1, 1.1, 1.1, 1.1));
        data->move_next();

        uint8_t* color =
            &(data->data()[spec.offset(VERTEX_ATTR_NAME_COLOR).value()]);

        assert_equal(color[0], 255);
        assert_equal(color[1], 255);
        assert_equal(color[2], 255);
        assert_equal(color[3], 255);
    }

    void test_moving_cursor() {
        auto spec =
            VertexFormatBuilder()
                .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
                     VERTEX_ATTR_TYPE_FLOAT)
                .add(VERTEX_ATTR_NAME_NORMAL, VERTEX_ATTR_ARRANGEMENT_XYZ,
                     VERTEX_ATTR_TYPE_FLOAT)
                .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
                     VERTEX_ATTR_TYPE_FLOAT)
                .build();

        auto data = std::make_shared<VertexData>(spec);
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
        auto spec = VertexFormat::position_only();

        smlt::VertexData source(spec);

        for(auto i = 0; i < 5; ++i) {
            source.position(i, 0, 0);
            source.move_next();
        }

        smlt::VertexData dest(spec);

        // Should be wiped by clone
        dest.position(-1, -1, 0);

        // Clone the data
        assert_true(source.clone_into(dest));

#define POSITION(n) dest.attr_as<Vec2>(VERTEX_ATTR_NAME_POSITION, (n)).value()
        // Check the data is valid
        assert_equal(dest.count(), source.count());
        assert_equal(smlt::Vec2(0, 0), POSITION(0));
        assert_equal(smlt::Vec2(1, 0), POSITION(1));
        assert_equal(smlt::Vec2(2, 0), POSITION(2));
        assert_equal(smlt::Vec2(3, 0), POSITION(3));
        assert_equal(smlt::Vec2(4, 0), POSITION(4));
        assert_equal(source.stride(), dest.stride());
#undef POSITION

        assert_equal(dest.cursor_position(), 0);
    }

    void test_basic_usage() {
        auto spec =
            VertexFormatBuilder()
                .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
                     VERTEX_ATTR_TYPE_FLOAT)
                .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
                     VERTEX_ATTR_TYPE_FLOAT)
                .add(VERTEX_ATTR_NAME_COLOR, VERTEX_ATTR_ARRANGEMENT_RGBA,
                     VERTEX_ATTR_TYPE_UNSIGNED_BYTE)
                .build();

        smlt::VertexData data(spec);

        assert_equal(0u, data.data_size());

        data.position(0, 0, 0);
        data.tex_coord0(1, 1);

        // sizeof(float) * 5 + sizeof(byte) * 4
        assert_equal(24u, data.data_size());
        data.move_next();
        data.position(0, 0, 0);
        data.tex_coord0(2, 2);
        data.done();

        // sizeof(float) * 10 + sizeof(byte) * 8
        assert_equal(48u, data.data_size());
    }
};

class VertexFormatTest: public smlt::test::TestCase {
public:
    void test_offsets() {
        VertexFormat format =
            VertexFormatBuilder()
                .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
                     VERTEX_ATTR_TYPE_FLOAT)
                .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
                     VERTEX_ATTR_TYPE_FLOAT)
                .add(VERTEX_ATTR_NAME_COLOR, VERTEX_ATTR_ARRANGEMENT_BGRA,
                     VERTEX_ATTR_TYPE_UNSIGNED_BYTE)
                .add(VERTEX_ATTR_NAME_NORMAL, VERTEX_ATTR_ARRANGEMENT_XYZ,
                     VERTEX_ATTR_TYPE_FLOAT)
                .build();

        assert_equal(0u, format.offset(VERTEX_ATTR_NAME_POSITION).value());
        assert_equal(sizeof(float) * 3,
                     format.offset(VERTEX_ATTR_NAME_TEXCOORD_0).value());
        assert_equal(sizeof(float) * 5,
                     format.offset(VERTEX_ATTR_NAME_COLOR).value());
        assert_equal(sizeof(float) * 5 + sizeof(uint32_t),
                     format.offset(VERTEX_ATTR_NAME_NORMAL).value());
    }

    void test_alignment() {
        VertexFormat format =
            VertexFormatBuilder()
                .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
                     VERTEX_ATTR_TYPE_FLOAT, 32) // 32 byte alignment
                .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
                     VERTEX_ATTR_TYPE_FLOAT)
                .build();

        assert_equal(format.stride(), 32u);
        assert_equal(format.attr(VERTEX_ATTR_NAME_POSITION)->alignment, 32u);

        format =
            VertexFormatBuilder()
                .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
                     VERTEX_ATTR_TYPE_FLOAT)
                .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
                     VERTEX_ATTR_TYPE_FLOAT, 32) // 32 byte alignment
                .build();

        assert_equal(format.stride(), 32u + (sizeof(float) * 2));
    }

    void test_separate() {
        VertexFormat format =
            VertexFormatBuilder(VERTEX_LAYOUT_SEPARATE)
                .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
                     VERTEX_ATTR_TYPE_FLOAT)
                .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
                     VERTEX_ATTR_TYPE_FLOAT)
                .build();

        assert_equal(format.stride(), sizeof(float) * 3);
        assert_equal(format.offset(VERTEX_ATTR_NAME_POSITION).value(), 0u);
        assert_equal(format.offset(VERTEX_ATTR_NAME_TEXCOORD_0).value(), 0u);
    }

    void test_vertex_data_editing() {
        VertexFormat format =
            VertexFormatBuilder()
                .add(VERTEX_ATTR_NAME_POSITION, VERTEX_ATTR_ARRANGEMENT_XYZ,
                     VERTEX_ATTR_TYPE_FLOAT)
                .add(VERTEX_ATTR_NAME_TEXCOORD_0, VERTEX_ATTR_ARRANGEMENT_XY,
                     VERTEX_ATTR_TYPE_FLOAT)
                .build();

        assert_equal(format.stride(), sizeof(float) * 5);

        VertexData data(format);
        data.resize(4);

        data.position(0, 0, 0);
        data.move_next();

        data.position(1, 0, 0);
        data.move_next();

        data.position(2, 0, 0);
        data.move_next();

        data.position(3, 0, 0);
        data.move_next();
        data.done();

        assert_true(data.is_dirty());

        assert_equal(((float*) data.data())[0], 0.0f);
        assert_equal(((float*) data.data())[5], 1.0f);
        assert_equal(((float*) data.data())[10], 2.0f);
        assert_equal(((float*) data.data())[15], 3.0f);
    }
};
}

#endif // TEST_VERTEX_DATA_H
