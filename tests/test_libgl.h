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

class LibGLNamedArrayTests : public TestCase {
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

class LibGLAlignedVectorTests : public TestCase {
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
    }
};

const uint32_t VERTEX_CMD_EOL = 0xf0000000;
const uint32_t VERTEX_CMD = 0xe0000000;

const float CLIP_DISTANCE = -0.2f;

class LibGLTriangleStripClippingTests : public TestCase {
public:
    void set_up() {
        aligned_vector_init(&input, sizeof(ClipVertex));
        aligned_vector_init(&output, sizeof(ClipVertex));
    }

    void tear_down() {
        aligned_vector_cleanup(&input);
        aligned_vector_cleanup(&output);
    }

    void _init_vector(ClipVertex& vec, float x, float y, float z) {
        vec.xyz[0] = x;
        vec.xyz[1] = y;
        vec.xyz[2] = z;
        vec.w = z * -1;
    }

    void test_all_vertices_visible() {
        ClipVertex vertices[4];

        _init_vector(vertices[0], -0.5, 0.0, -1);
        _init_vector(vertices[1], -0.5, 0.0, -2);
        _init_vector(vertices[2], 0.5, 0.0, -1);
        _init_vector(vertices[3], 0.5, 0.0, -2);

        aligned_vector_push_back(&input, vertices, 4);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 4u);
    }

    void test_no_vertices_visible() {
        ClipVertex vertices[4];

        _init_vector(vertices[0], -0.5, 0.0, 0.1);
        _init_vector(vertices[1], -0.5, 0.0, 1);
        _init_vector(vertices[2], 0.5, 0.0, 0.1);
        _init_vector(vertices[3], 0.5, 0.0, 1);

        aligned_vector_push_back(&input, vertices, 4);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 0u);
    }

    void test_first_vertex_clipped_single_triangle() {
        /* Case that the strip is a single triangle but the first vertex is behind the plane */

        ClipVertex vertices[3];

        _init_vector(vertices[0], 0, 0.0, 1);
        _init_vector(vertices[1], 0.5, 0.0, -1);
        _init_vector(vertices[2], -0.5, 0.0, -1);

        aligned_vector_push_back(&input, vertices, 3);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 4);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&output, 3);

        assert_close(v1->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v2->xyz[2], -1.0f, 0.00001f);
        assert_close(v3->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v4->xyz[2], -1.0f, 0.00001f);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD);
        assert_equal(v4->flags, VERTEX_CMD_EOL);
    }

    void test_second_vertex_clipped_single_triangle() {
        /* Case that the strip is a single triangle but the second vertex is behind the plane */

        ClipVertex vertices[3];

        _init_vector(vertices[0], -0.5, 0.0, -1);
        _init_vector(vertices[1], 0, 0.0, 1);
        _init_vector(vertices[2], 0.5, 0.0, -1);

        aligned_vector_push_back(&input, vertices, 3);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 4);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&output, 3);

        assert_close(v1->xyz[2], -1.0f, 0.00001f);
        assert_close(v2->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v3->xyz[2], -1.0f, 0.00001f);
        assert_close(v4->xyz[2], CLIP_DISTANCE, 0.00001f);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD);
        assert_equal(v4->flags, VERTEX_CMD_EOL);
    }

    void test_third_vertex_clipped_single_triangle() {
        /* Case that the strip is a single triangle but the third vertex is behind the plane */

        ClipVertex vertices[3];

        _init_vector(vertices[0], 0.5, 0.0, -1);
        _init_vector(vertices[1], -0.5, 0.0, -1);
        _init_vector(vertices[2], 0, 0.0, 1);

        aligned_vector_push_back(&input, vertices, 3);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 4);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&output, 3);

        assert_close(v1->xyz[2], -1.0f, 0.00001f);
        assert_close(v2->xyz[2], -1.0f, 0.00001f);
        assert_close(v3->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v4->xyz[2], CLIP_DISTANCE, 0.00001f);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD);
        assert_equal(v4->flags, VERTEX_CMD_EOL);
    }

    void test_first_vertex_clipped_multiple_triangles() {
        /* Case that the strip is two triangles but the first vertex is behind the plane */

        ClipVertex vertices[4];

        _init_vector(vertices[0], 0, 0, 1);
        _init_vector(vertices[1], 0.5, 0, -1);
        _init_vector(vertices[2], -0.5, 0, -1);
        _init_vector(vertices[3], 0, 0, -2);

        vertices[0].flags = vertices[1].flags = vertices[2].flags = VERTEX_CMD;
        vertices[3].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, 4);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 7);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&output, 3);
        ClipVertex* v5 = (ClipVertex*) aligned_vector_at(&output, 4);
        ClipVertex* v6 = (ClipVertex*) aligned_vector_at(&output, 5);
        ClipVertex* v7 = (ClipVertex*) aligned_vector_at(&output, 6);

        assert_close(v1->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v2->xyz[2], -1.0f, 0.00001f);
        assert_close(v3->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v4->xyz[2], -1.0f, 0.00001f);
        assert_close(v5->xyz[2], -1.0f, 0.00001f);
        assert_close(v6->xyz[2], -1.0f, 0.00001f);
        assert_close(v7->xyz[2], -2.0f, 0.00001f);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD);
        assert_equal(v4->flags, VERTEX_CMD_EOL);
        assert_equal(v5->flags, VERTEX_CMD);
        assert_equal(v6->flags, VERTEX_CMD);
        assert_equal(v7->flags, VERTEX_CMD_EOL);
    }

    void test_second_vertex_clipped_multiple_triangles() {
        /* Case that the strip is two triangles but the second vertex is behind the plane */

        ClipVertex vertices[4];

        _init_vector(vertices[0], -0.5, 0, -1);
        _init_vector(vertices[1], 0, 0, 1);
        _init_vector(vertices[2], 0, 0, -2);
        _init_vector(vertices[3], 0.05, 0, -1);

        vertices[0].flags = vertices[1].flags = vertices[2].flags = VERTEX_CMD;
        vertices[3].flags = VERTEX_CMD_EOL;

        aligned_vector_push_back(&input, vertices, 4);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 8);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);
        ClipVertex* v4 = (ClipVertex*) aligned_vector_at(&output, 3);
        ClipVertex* v5 = (ClipVertex*) aligned_vector_at(&output, 4);
        ClipVertex* v6 = (ClipVertex*) aligned_vector_at(&output, 5);
        ClipVertex* v7 = (ClipVertex*) aligned_vector_at(&output, 6);
        ClipVertex* v8 = (ClipVertex*) aligned_vector_at(&output, 7);

        assert_close(v1->xyz[2], -1.0f, 0.00001f);
        assert_close(v2->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v3->xyz[2], -2.0f, 0.00001f);
        assert_close(v4->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v5->xyz[2], -2.0f, 0.00001f);
        assert_close(v6->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v7->xyz[2], -1.0f, 0.00001f);
        assert_close(v8->xyz[2], CLIP_DISTANCE, 0.00001f);


        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD);
        assert_equal(v4->flags, VERTEX_CMD_EOL);
        assert_equal(v5->flags, VERTEX_CMD);
        assert_equal(v6->flags, VERTEX_CMD);
        assert_equal(v7->flags, VERTEX_CMD);
        assert_equal(v8->flags, VERTEX_CMD_EOL);
    }

    void test_third_vertex_clipped_multiple_triangles() {
        /* Case that the strip is two triangles but the third vertex is behind the plane */
    }

    void test_penultimate_vertex_clipped_multiple_triangles() {
        /* Case that the strip is two triangles but the second-to-last vertex is behind the plane */
    }

    void test_last_vertex_clipped_multiple_triangles() {
        /* Case that the strip is two triangles but the last vertex is behind the plane */
    }

    void test_first_vertex_visible_single_triangle() {
        ClipVertex vertices[3];

        _init_vector(vertices[0], 0, 0.0, -1);
        _init_vector(vertices[1], -0.5, 0.0, 1);
        _init_vector(vertices[2], 0.5, 0.0, 1);

        aligned_vector_push_back(&input, vertices, 3);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 3);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);

        assert_close(v1->xyz[2], -1.0f, 0.00001f);
        assert_close(v2->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v3->xyz[2], CLIP_DISTANCE, 0.00001f);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD_EOL);
    }

    void test_second_vertex_visible_single_triangle() {
        ClipVertex vertices[3];

        _init_vector(vertices[0], 0.5, 0.0, 1);
        _init_vector(vertices[1], 0, 0.0, -1);
        _init_vector(vertices[2], -0.5, 0.0, 1);

        aligned_vector_push_back(&input, vertices, 3);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 3);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);

        assert_close(v1->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v2->xyz[2], -1.0f, 0.00001f);
        assert_close(v3->xyz[2], CLIP_DISTANCE, 0.00001f);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD_EOL);
    }

    void test_third_vertex_visible_single_triangle() {
        ClipVertex vertices[3];

        _init_vector(vertices[0], -0.5, 0.0, 1);
        _init_vector(vertices[1], 0.5, 0.0, 1);
        _init_vector(vertices[2], 0, 0.0, -1);

        aligned_vector_push_back(&input, vertices, 3);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 3);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);

        assert_close(v1->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v2->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v3->xyz[2], -1.0f, 0.00001f);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD_EOL);
    }

    void test_first_vertex_visible_multiple_triangles() {
        ClipVertex vertices[4];

        _init_vector(vertices[0], 0, 0.0, -1);
        _init_vector(vertices[1], -0.5, 0.0, 1);
        _init_vector(vertices[2], 0.5, 0.0, 1);
        _init_vector(vertices[3], 0.0, 0.0, 2);

        aligned_vector_push_back(&input, vertices, 4);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 3);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);

        assert_close(v1->xyz[2], -1.0f, 0.00001f);
        assert_close(v2->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v3->xyz[2], CLIP_DISTANCE, 0.00001f);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD_EOL);
    }

    void test_last_vertex_visible_multiple_triangles() {
        ClipVertex vertices[4];

        _init_vector(vertices[0], 0, 0.0, 2);
        _init_vector(vertices[1], 0.5, 0.0, 1);
        _init_vector(vertices[2], -0.5, 0.0, 1);
        _init_vector(vertices[3], 0.0, 0.0, -1);

        aligned_vector_push_back(&input, vertices, 4);

        clipTriangleStrip(&input, &output);

        assert_equal(output.size, 3);

        ClipVertex* v1 = (ClipVertex*) aligned_vector_at(&output, 0);
        ClipVertex* v2 = (ClipVertex*) aligned_vector_at(&output, 1);
        ClipVertex* v3 = (ClipVertex*) aligned_vector_at(&output, 2);

        assert_close(v1->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v2->xyz[2], CLIP_DISTANCE, 0.00001f);
        assert_close(v3->xyz[2], -1.0f, 0.00001f);

        assert_equal(v1->flags, VERTEX_CMD);
        assert_equal(v2->flags, VERTEX_CMD);
        assert_equal(v3->flags, VERTEX_CMD_EOL);
    }

private:
    AlignedVector input;
    AlignedVector output;
};


}

#endif // TEST_LIBGL_H
