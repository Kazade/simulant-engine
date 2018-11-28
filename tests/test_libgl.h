#ifndef TEST_LIBGL_H
#define TEST_LIBGL_H

/* These tests don't really belong in Simulant, but they're here until (if)
 * I set up a test harness in the Dreamcast libGL fork
 */

#include "global.h"

#include "../deps/libgl/containers/named_array.h"
#include "../deps/libgl/containers/aligned_vector.h"
#include "../deps/libgl/GL/clip.h"

namespace {

struct SomeStructure {
    float x;
    float y;
    float z;
};

class LibGLNamedArrayTests : public smlt::test::TestCase {
public:

    void test_alloc_and_release() {
        NamedArray test;

        named_array_init(&test, sizeof(SomeStructure), 5);

        unsigned int new_id;
        SomeStructure* s = (SomeStructure*) named_array_alloc(&test, &new_id);

        assert_equal(new_id, 1u);
        assert_is_not_null(named_array_get(&test, new_id));
        assert_is_null(named_array_get(&test, 2));

        s->x = 1.0f;
        s->y = 2.0f;
        s->z = 3.0f;

        SomeStructure* s2 = (SomeStructure*) named_array_get(&test, new_id);

        assert_equal(s2->x, s->x);
        assert_equal(s2->y, s->y);
        assert_equal(s2->z, s->z);

        named_array_release(&test, new_id);

        assert_is_null(named_array_get(&test, new_id));

        unsigned int second_id;
        s = (SomeStructure*) named_array_alloc(&test, &second_id);

        /* Should've reused the first id */
        assert_equal(second_id, new_id);

        /* Should have memset the memory so stuff doesn't linger */
        assert_close(s->x, 0.0f, 0.0001f);

        named_array_cleanup(&test);
    }

    void test_alloc_multiple() {
        std::vector<SomeStructure*> pointers;

        NamedArray test;
        named_array_init(&test, sizeof(SomeStructure), 10);

        for(auto i = 0u; i < 5; ++i) {
            auto id = 0u;
            pointers.push_back((SomeStructure*) named_array_alloc(&test, &id));
        }

        for(auto i = 0u; i < 5; ++i) {
            auto ptr = pointers[i];
            assert_equal(ptr, named_array_get(&test, i + 1));
        }
    }

};

class LibGLAlignedVectorTests : public smlt::test::TestCase {
public:

    void test_aligned_vector() {
        AlignedVector vector;

        aligned_vector_init(&vector, sizeof(SomeStructure));

        // Make sure we have some initial capacity
        assert_equal(vector.capacity, ALIGNED_VECTOR_INITIAL_CAPACITY);
        assert_equal(vector.size, 0u); // But nothing in there

        SomeStructure objects[3];
        objects[0].x = 0.0f;
        objects[1].x = 1.0f;
        objects[2].x = 2.0f;

        aligned_vector_push_back(&vector, objects, 3);

        assert_equal(vector.size, 3u);

        assert_equal(0.0f, ((SomeStructure*) aligned_vector_at(&vector, 0))->x);
        assert_equal(1.0f, ((SomeStructure*) aligned_vector_at(&vector, 1))->x);
        assert_equal(2.0f, ((SomeStructure*) aligned_vector_at(&vector, 2))->x);

        aligned_vector_clear(&vector);

        assert_equal(vector.size, 0u);

        // Capacity should have remained unchanged
        assert_equal(vector.capacity, ALIGNED_VECTOR_INITIAL_CAPACITY);
        aligned_vector_cleanup(&vector);
    }

    void test_resizing() {
        struct TestStruct {
            int commands[8];
        };

        AlignedVector vector;
        aligned_vector_init(&vector, sizeof(TestStruct));

        aligned_vector_resize(&vector, 3);

        ((TestStruct*) aligned_vector_at(&vector, 0))->commands[0] = 1;
        ((TestStruct*) aligned_vector_at(&vector, 1))->commands[0] = 2;
        ((TestStruct*) aligned_vector_at(&vector, 2))->commands[0] = 3;

        aligned_vector_resize(&vector, 5);

        ((TestStruct*) aligned_vector_at(&vector, 3))->commands[0] = 4;
        ((TestStruct*) aligned_vector_at(&vector, 4))->commands[0] = 5;

        TestStruct* data = (TestStruct*) vector.data;

        assert_equal(data[0].commands[0], 1);
        assert_equal(data[1].commands[0], 2);
        assert_equal(data[2].commands[0], 3);
        assert_equal(data[3].commands[0], 4);
        assert_equal(data[4].commands[0], 5);
        aligned_vector_cleanup(&vector);
    }

    void test_extending() {
        struct TestStruct {
            int commands[8];
        };

        AlignedVector vector;
        aligned_vector_init(&vector, sizeof(TestStruct));

        aligned_vector_resize(&vector, 3);

        ((TestStruct*) aligned_vector_at(&vector, 0))->commands[0] = 1;
        ((TestStruct*) aligned_vector_at(&vector, 1))->commands[0] = 2;
        ((TestStruct*) aligned_vector_at(&vector, 2))->commands[0] = 3;

        TestStruct* new_struct = (TestStruct*) aligned_vector_extend(&vector, 5);

        new_struct->commands[0] = 4;

        TestStruct* data = (TestStruct*) vector.data;

        assert_equal(data[0].commands[0], 1);
        assert_equal(data[1].commands[0], 2);
        assert_equal(data[2].commands[0], 3);
        assert_equal(data[3].commands[0], 4);

        aligned_vector_cleanup(&vector);
    }


    void test_reallocation() {
        AlignedVector vector;

        aligned_vector_init(&vector, sizeof(SomeStructure));

        // Make sure we have some initial capacity
        assert_equal(vector.capacity, ALIGNED_VECTOR_INITIAL_CAPACITY);

        aligned_vector_shrink_to_fit(&vector);

        assert_equal(vector.capacity, 0u);

        SomeStructure s;
        aligned_vector_push_back(&vector, &s, 1);

        assert_equal(vector.size, 1u);
        assert_equal(vector.capacity, 2u);

        aligned_vector_push_back(&vector, &s, 1);

        assert_equal(vector.size, 2u);
        assert_equal(vector.capacity, 2u);

        aligned_vector_push_back(&vector, &s, 1);

        assert_equal(vector.size, 3u);
        assert_equal(vector.capacity, 5u);
        aligned_vector_cleanup(&vector);
    }
};

const uint32_t VERTEX_CMD_EOL = 0xf0000000;
const uint32_t VERTEX_CMD = 0xe0000000;

const float CLIP_DISTANCE = -0.2f;

class LibGLTriangleStripClippingTests2 : public smlt::test::TestCase {
private:
    AlignedVector input;

    void _init_vector(ClipVertex& vec, float x, float y, float z) {
        vec.xyz[0] = x;
        vec.xyz[1] = y;
        vec.xyz[2] = z;
        vec.w = z * -1;
    }

public:
    void set_up() {
        aligned_vector_init(&input, sizeof(ClipVertex));
    }

    void tear_down() {
        aligned_vector_cleanup(&input);
    }

    void test_long_triangle_strip() {
        ClipVertex vertices[7]; //< Include header

        *((uint32_t*)&vertices[0]) = 0xDEADBEEF; // Fake header

        _init_vector(vertices[1], -3330.062988, -919.664062, 6.152299);
        _init_vector(vertices[2], -371.229980, -871.586792, 5.951777);
        _init_vector(vertices[3], -364.770020, 1478.409058, -3.849674);
        _init_vector(vertices[4], 2594.062988, 1526.486450, -4.050197);
        _init_vector(vertices[5], 1117.876465, 2677.445801, -8.850660);
        _init_vector(vertices[6], 4076.709473, 2725.522949, -9.051184);

        vertices[1].flags = VERTEX_CMD;
        vertices[2].flags = VERTEX_CMD;
        vertices[3].flags = VERTEX_CMD;
        vertices[4].flags = VERTEX_CMD;
        vertices[5].flags = VERTEX_CMD;
        vertices[6].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, 7);

        clipTriangleStrip2(&input, 0, 0);
    }

    void test_vertex_generation() {
        ClipVertex vertices[7]; //< Include header

        *((uint32_t*)&vertices[0]) = 0xDEADBEEF; // Fake header

        _init_vector(vertices[1], 1134.682007, 483.819763, 0.298596);
        _init_vector(vertices[2], 4232.841309, 1396.291748, -3.507177);
        _init_vector(vertices[3], -1192.841553, 2042.530518, -6.202533);
        _init_vector(vertices[4], 1905.317871, 2955.002686, -10.008306);
        _init_vector(vertices[5], -2356.603516, 2821.885742, -9.453097);
        _init_vector(vertices[6], 741.556030, 3734.357910, -13.258870);

        vertices[1].flags = VERTEX_CMD;
        vertices[2].flags = VERTEX_CMD;
        vertices[3].flags = VERTEX_CMD;
        vertices[4].flags = VERTEX_CMD;
        vertices[5].flags = VERTEX_CMD;
        vertices[6].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, 7);

        clipTriangleStrip2(&input, 0, 0);

        assert_equal(input.size, 7u + 6u); /* Should introduce 6 new vertices */

        ClipVertex* clipped = (ClipVertex*) aligned_vector_at(&input, 0);

        assert_equal(clipped[6].flags, VERTEX_CMD_EOL);
        assert_equal(clipped[9].flags, VERTEX_CMD_EOL);

        /* New generated vertices */
        assert_close(clipped[7].xyz[2], -0.2, 0.00001);
        assert_close(clipped[10].xyz[2], -0.2, 0.00001);

        /* Reuse of existing vertices */
        assert_close(clipped[8].xyz[2], vertices[2].xyz[2], 0.00001);
        assert_close(clipped[9].xyz[2], vertices[3].xyz[2], 0.00001);
    }

    void test_triangle_strip_back_to_front() {
        ClipVertex vertices[5]; //< Include header

        *((uint32_t*)&vertices[0]) = 0xDEADBEEF; // Fake header

        _init_vector(vertices[1], -0.5, 0.0, 1);
        _init_vector(vertices[2], -0.5, 0.0,-1);
        _init_vector(vertices[3], 0.5, 0.0, -1);
        _init_vector(vertices[4], 0.5, 0.0, -2);

        vertices[1].flags = vertices[2].flags = vertices[3].flags = VERTEX_CMD;
        vertices[4].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, 5);

        clipTriangleStrip2(&input, 0, 0);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&input, 1);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&input, 2);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&input, 3);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&input, 4);

        assert_equal(v1->flags, VERTEX_CMD_EOL);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD);
        assert_equal(v4->flags, VERTEX_CMD_EOL);
    }

    void test_triangle_strip_front_to_back() {
        ClipVertex vertices[7]; //< Include header

        *((uint32_t*)&vertices[0]) = 0xDEADBEEF; // Fake header

        _init_vector(vertices[1], -0.5, 0.0, -2);
        _init_vector(vertices[2], -0.5, 0.0, -1);
        _init_vector(vertices[3], 0.5, 0.0,  -1);
        _init_vector(vertices[4], 0.5, 0.0, 1);
        _init_vector(vertices[5], 0.5, 0.0, 1);
        _init_vector(vertices[6], 0.5, 0.0, 2);

        vertices[1].flags = vertices[2].flags = vertices[3].flags = vertices[4].flags = vertices[5].flags = VERTEX_CMD;
        vertices[6].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, 7);

        clipTriangleStrip2(&input, 0, 0);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&input, 1);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&input, 2);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&input, 3);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&input, 4);
        ClipVertex* v5 = (ClipVertex*) aligned_vector_at(&input, 5);
        ClipVertex* v6 = (ClipVertex*) aligned_vector_at(&input, 6);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD_EOL);
        assert_equal(v4->flags, VERTEX_CMD_EOL);
        assert_equal(v5->flags, VERTEX_CMD_EOL);
        assert_equal(v6->flags, VERTEX_CMD_EOL);
    }

    void test_triangle_strip_penultimate_and_last() {
        ClipVertex vertices[6]; //< Include header

        *((uint32_t*)&vertices[0]) = 0xDEADBEEF; // Fake header

        _init_vector(vertices[1], -0.5, 0.0, -2);
        _init_vector(vertices[2], -0.5, 0.0, -1);
        _init_vector(vertices[3], 0.5, 0.0,  -1);
        _init_vector(vertices[4], 0.5, 0.0, 2);
        _init_vector(vertices[5], 0.5, 0.0, 2);

        vertices[1].flags = vertices[2].flags = vertices[3].flags = vertices[4].flags = VERTEX_CMD;
        vertices[5].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, 6);

        clipTriangleStrip2(&input, 0, 0);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&input, 1);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&input, 2);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&input, 3);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&input, 4);
        ClipVertex* v5 = (ClipVertex*) aligned_vector_at(&input, 5);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD_EOL);
        assert_equal(v4->flags, VERTEX_CMD_EOL);  // Duplicated header
        assert_equal(v5->flags, VERTEX_CMD_EOL);
    }

    void test_weird_edge_case() {
        /* Two triangles, both need clipping */

        ClipVertex vertices[8]; //< Include header

        uint32_t offset = 3;

        *((uint32_t*)&vertices[offset + 0]) = 0xDEADBEEF; // Fake header

        _init_vector(vertices[offset + 1], -61141.121094, 497.055908, 9.909909);
        _init_vector(vertices[offset + 2], 54741.121094, 497.055908, 9.909909);
        _init_vector(vertices[offset + 3], 2858.878906, 48497.054688, -190.290283);
        _init_vector(vertices[offset + 4], 118741.125000, 48497.054688, -190.290283);

        vertices[offset + 1].flags = VERTEX_CMD;
        vertices[offset + 2].flags = VERTEX_CMD;
        vertices[offset + 3].flags = VERTEX_CMD;
        vertices[offset + 4].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, offset + 5);

        clipTriangleStrip2(&input, offset, 0);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&input, offset + 1);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&input, offset + 2);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&input, offset + 3);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&input, offset + 4);

        assert_equal(v1->flags, VERTEX_CMD_EOL);
        assert_equal(v2->flags, VERTEX_CMD_EOL);
        assert_equal(v3->flags, VERTEX_CMD_EOL);
        assert_equal(v4->flags, VERTEX_CMD_EOL);
    }

    void test_multiple_strips() {
        ClipVertex vertices[13]; //< Include header

        *((uint32_t*)&vertices[0]) = 0xDEADBEEF; // Fake header

        _init_vector(vertices[1], -0.5, 0.0, -2);
        _init_vector(vertices[2], -0.5, 0.0, -2);
        _init_vector(vertices[3], 0.5, 0.0,  -2);
        _init_vector(vertices[4], 0.5, 0.0,  -2);
        _init_vector(vertices[5], 0.5, 0.0, 2);
        _init_vector(vertices[6], 0.5, 0.0, 2);
        _init_vector(vertices[7], 0.5, 0.0, 2);
        _init_vector(vertices[8], 0.5, 0.0, 2);
        _init_vector(vertices[9], 0.5, 0.0, -2);
        _init_vector(vertices[10], 0.5, 0.0, -2);
        _init_vector(vertices[11], 0.5, 0.0, 2);
        _init_vector(vertices[12], 0.5, 0.0, 2);

        for(uint32_t i = 1; i < 12; ++i) {
            vertices[i].flags = VERTEX_CMD;
        }

        vertices[12].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, 13);

        clipTriangleStrip2(&input, 0, 0);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&input, 1);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&input, 2);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&input, 3);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&input, 4);
        ClipVertex* v5 = (ClipVertex*) aligned_vector_at(&input, 5);
        ClipVertex* v6 = (ClipVertex*) aligned_vector_at(&input, 6);
        ClipVertex* v7 = (ClipVertex*) aligned_vector_at(&input, 7);
        ClipVertex* v8 = (ClipVertex*) aligned_vector_at(&input, 8);
        ClipVertex* v9 = (ClipVertex*) aligned_vector_at(&input, 9);
        ClipVertex* v10 = (ClipVertex*) aligned_vector_at(&input, 10);
        ClipVertex* v11 = (ClipVertex*) aligned_vector_at(&input, 11);
        ClipVertex* v12 = (ClipVertex*) aligned_vector_at(&input, 12);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD);
        assert_equal(v4->flags, VERTEX_CMD_EOL);
        assert_equal(v5->flags, VERTEX_CMD_EOL);
        assert_equal(v6->flags, VERTEX_CMD_EOL);
        assert_equal(v7->flags, VERTEX_CMD_EOL);
        assert_equal(v8->flags, VERTEX_CMD_EOL);
        assert_equal(v9->flags, VERTEX_CMD_EOL);
        assert_equal(v10->flags, VERTEX_CMD_EOL);
        assert_equal(v11->flags, VERTEX_CMD_EOL);
        assert_equal(v12->flags, VERTEX_CMD_EOL);
    }

    void test_multiple_triangles() {
        ClipVertex vertices[10]; //< Include header

        *((uint32_t*)&vertices[0]) = 0xDEADBEEF; // Fake header

        _init_vector(vertices[1], -0.5, 0.0, -2);
        _init_vector(vertices[2], -0.5, 0.0, -1);
        _init_vector(vertices[3], 0.5, 0.0,  -1);
        _init_vector(vertices[4], 0.5, 0.0, 1);
        _init_vector(vertices[5], -0.5, 0.0, 1);
        _init_vector(vertices[6], 0.5, 0.0, 2);
        _init_vector(vertices[7], -0.5, 0.0, -2);
        _init_vector(vertices[8], -0.5, 0.0, -1);
        _init_vector(vertices[9], 0.5, 0.0,  -1);

        vertices[1].flags = vertices[2].flags = VERTEX_CMD;
        vertices[3].flags = VERTEX_CMD_EOL;

        vertices[4].flags = vertices[5].flags = VERTEX_CMD;
        vertices[6].flags = VERTEX_CMD_EOL;

        vertices[7].flags = vertices[8].flags = VERTEX_CMD;
        vertices[9].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, 10);

        clipTriangleStrip2(&input, 0, 0);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&input, 1);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&input, 2);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&input, 3);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&input, 4);
        ClipVertex* v5 = (ClipVertex*) aligned_vector_at(&input, 5);
        ClipVertex* v6 = (ClipVertex*) aligned_vector_at(&input, 6);
        ClipVertex* v7 = (ClipVertex*) aligned_vector_at(&input, 7);
        ClipVertex* v8 = (ClipVertex*) aligned_vector_at(&input, 8);
        ClipVertex* v9 = (ClipVertex*) aligned_vector_at(&input, 9);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD_EOL);

        assert_equal(v4->flags, VERTEX_CMD_EOL);
        assert_equal(v5->flags, VERTEX_CMD_EOL);
        assert_equal(v6->flags, VERTEX_CMD_EOL);

        assert_equal(v7->flags, VERTEX_CMD);
        assert_equal(v8->flags, VERTEX_CMD);
        assert_equal(v9->flags, VERTEX_CMD_EOL);
    }
};

}

#endif // TEST_LIBGL_H
