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
        smlt::VertexSpecification spec = VertexSpecification{
            smlt::VERTEX_ATTRIBUTE_3F,
            smlt::VERTEX_ATTRIBUTE_3F,
            smlt::VERTEX_ATTRIBUTE_2F
        };

        smlt::VertexData::ptr data = std::make_shared<VertexData>(spec);

        assert_equal(0, (int32_t) data->vertex_specification().position_offset());
        assert_equal(sizeof(float) * 3, (uint32_t) data->vertex_specification().texcoord0_offset());
        assert_equal(sizeof(float) * 5, (uint32_t) data->vertex_specification().normal_offset());
    }

    void test_position_works_with_a_subset_of_components() {
        /* Position should automatically fill z == 0, and w == 1 if they aren't
         * specified */

        smlt::VertexSpecification spec;
        spec.position_attribute = VERTEX_ATTRIBUTE_4F;

        smlt::VertexData::ptr data = std::make_shared<VertexData>(spec);

        data->position(smlt::Vec2(9.0f, 8.0f));
        data->move_next();

        auto pos = data->position_at<smlt::Vec4>(0);
        assert_close(pos->x, 9.0f, 0.00001f);
        assert_close(pos->y, 8.0f, 0.00001f);
        assert_close(pos->z, 0.0f, 0.00001f);
        assert_close(pos->w, 1.0f, 0.00001f);
    }

    void test_packed_normal_offsets() {
        smlt::VertexSpecification spec;
        spec.position_attribute = VERTEX_ATTRIBUTE_3F;
        spec.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
        spec.diffuse_attribute = VERTEX_ATTRIBUTE_4UB;
        spec.normal_attribute = VERTEX_ATTRIBUTE_PACKED_VEC4_1I;
        spec.texcoord1_attribute = VERTEX_ATTRIBUTE_2F;

        assert_equal((uint32_t) spec.diffuse_offset(), sizeof(float) * 5u);
        assert_equal((uint32_t) spec.normal_offset(), (sizeof(float) * 5u) + sizeof(uint32_t));
        assert_equal((uint32_t) spec.texcoord1_offset(), (sizeof(float) * 5u) + (sizeof(uint32_t) * 2));

        smlt::VertexData::ptr data = std::make_shared<VertexData>(spec);
        data->position(smlt::Vec3());
        data->diffuse(smlt::Color(0, 0, 0, 0));
        data->normal(smlt::Vec3::POSITIVE_Y);
        data->move_next();

        data->position(smlt::Vec3());
        data->diffuse(smlt::Color(0, 0, 0, 0));
        data->normal(smlt::Vec3::NEGATIVE_X);
        data->move_next();

        data->position(smlt::Vec3());
        data->diffuse(smlt::Color(0, 0, 0, 0));
        data->normal(smlt::Vec3(0.5, -0.5, 0.5));
        data->move_next();

        const smlt::Vec3* n = data->normal_at<smlt::Vec3>(0);
        assert_close(n->x, 0.0f, 0.01f);
        assert_close(n->y, 1.0f, 0.01f);
        assert_close(n->z, 0.0f, 0.01f);

        const smlt::Vec3* n2 = data->normal_at<smlt::Vec3>(1);
        assert_close(n2->x, -1.0f, 0.01f);
        assert_close(n2->y, 0.0f, 0.01f);
        assert_close(n2->z, 0.0f, 0.01f);

        const smlt::Vec3* n3 = data->normal_at<smlt::Vec3>(2);
        assert_close(n3->x, 0.5f, 0.01f);
        assert_close(n3->y, -0.5f, 0.01f);
        assert_close(n3->z, 0.5f, 0.01f);
    }

    void test_colors_dont_overflow() {
        smlt::VertexSpecification spec = VertexSpecification{
            smlt::VERTEX_ATTRIBUTE_3F
        };

        spec.diffuse_attribute = smlt::VERTEX_ATTRIBUTE_4UB;

        smlt::VertexData::ptr data = std::make_shared<VertexData>(spec);
        data->position(0, 0, 0);
        data->diffuse(smlt::Color(1.1, 1.1, 1.1, 1.1));
        data->move_next();

        uint8_t* color = &(data->data()[spec.diffuse_offset()]);

        assert_equal(color[0], 255);
        assert_equal(color[1], 255);
        assert_equal(color[2], 255);
        assert_equal(color[3], 255);
    }

    void test_moving_cursor() {
        smlt::VertexSpecification spec = VertexSpecification{
            smlt::VERTEX_ATTRIBUTE_3F,
            smlt::VERTEX_ATTRIBUTE_3F,
            smlt::VERTEX_ATTRIBUTE_2F
        };

        smlt::VertexData::ptr data = std::make_shared<VertexData>(spec);
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
        smlt::VertexSpecification spec = VertexSpecification{smlt::VERTEX_ATTRIBUTE_2F};

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

}

#endif // TEST_VERTEX_DATA_H
