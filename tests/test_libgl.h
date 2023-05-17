#ifndef TEST_LIBGL_H
#define TEST_LIBGL_H

/* These tests don't really belong in Simulant, but they're here until (if)
 * I set up a test harness in the Dreamcast libGL fork
 */



#include "../deps/libgl/containers/named_array.h"
#include "../deps/libgl/containers/aligned_vector.h"

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

        assert_equal(new_id, 0u);
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
            assert_equal(ptr, named_array_get(&test, i));
        }

        named_array_cleanup(&test);
    }

};

class LibGLAlignedVectorTests : public smlt::test::TestCase {
public:

    void test_aligned_vector() {
        AlignedVector vector;

        aligned_vector_init(&vector, sizeof(SomeStructure));

        // Make sure we have some initial capacity
        assert_equal(aligned_vector_capacity(&vector), ALIGNED_VECTOR_CHUNK_SIZE);
        assert_equal(aligned_vector_size(&vector), 0u); // But nothing in there

        SomeStructure objects[3];
        objects[0].x = 0.0f;
        objects[1].x = 1.0f;
        objects[2].x = 2.0f;

        aligned_vector_push_back(&vector, objects, 3);

        assert_equal(aligned_vector_size(&vector), 3u);

        assert_equal(0.0f, ((SomeStructure*) aligned_vector_at(&vector, 0))->x);
        assert_equal(1.0f, ((SomeStructure*) aligned_vector_at(&vector, 1))->x);
        assert_equal(2.0f, ((SomeStructure*) aligned_vector_at(&vector, 2))->x);

        aligned_vector_clear(&vector);

        assert_equal(aligned_vector_size(&vector), 0u);

        // Capacity should have remained unchanged
        assert_equal(aligned_vector_capacity(&vector), ALIGNED_VECTOR_CHUNK_SIZE);
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
        assert_equal(aligned_vector_capacity(&vector), ALIGNED_VECTOR_CHUNK_SIZE);

        aligned_vector_shrink_to_fit(&vector);

        assert_equal(aligned_vector_capacity(&vector), 0u);

        SomeStructure s;
        aligned_vector_push_back(&vector, &s, 1);

        assert_equal(aligned_vector_size(&vector), 1u);
        assert_equal(aligned_vector_capacity(&vector), ALIGNED_VECTOR_CHUNK_SIZE);

        aligned_vector_push_back(&vector, &s, 1);

        assert_equal(aligned_vector_size(&vector), 2u);
        assert_equal(aligned_vector_capacity(&vector), ALIGNED_VECTOR_CHUNK_SIZE);

        aligned_vector_push_back(&vector, &s, 1);

        assert_equal(aligned_vector_size(&vector), 3u);
        assert_equal(aligned_vector_capacity(&vector), ALIGNED_VECTOR_CHUNK_SIZE);
        aligned_vector_cleanup(&vector);
    }
};

}

#endif // TEST_LIBGL_H
