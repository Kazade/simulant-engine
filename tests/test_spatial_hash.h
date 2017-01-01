#pragma once

#include <kaztest/kaztest.h>

#include "../simulant/partitioners/impl/spatial_hash.h"

namespace {

using namespace smlt;

class SpatialHashTests : public TestCase {
public:
    void set_up() {
        hash_ = new smlt::SpatialHash();
        new_entry_ = new SpatialHashEntry();
    }

    void tear_down() {
        delete hash_;
        delete new_entry_;
    }

    void test_key_construction() {
        Key test1 = make_key(1, 0, 0, 0);
        Key test2 = make_key(2, 0.5, 0, 0);

        assert_equal(15, test1.ancestors);
        assert_equal(14, test2.ancestors);

        assert_true(test2.is_ancestor_of(test1));
        assert_false(test1.is_ancestor_of(test2));
        assert_true(test1.is_ancestor_of(test1)); // Keys are ancestors of themselves
    }

    void test_key_comparison() {
        Key key1, key2, key3, key4, key5;

        key1.hash_path[0] = {1}; key1.hash_path[1] = {2};
        key1.ancestors = 1;

        key2.hash_path[0] = {1}; key2.hash_path[1] = {3};
        key2.ancestors = 1;

        key3.hash_path[0] = {1}; key3.hash_path[1] = {1}; key3.hash_path[2] = {2};
        key3.ancestors = 2;

        key4.hash_path[0] = {3};
        key4.ancestors = 0;

        key5.hash_path[0] = {1};
        key5.ancestors = 0;

        assert_true(key3 < key1);
        assert_false(key1 < key3);

        assert_true(key3 < key2);
        assert_false(key2 < key3);

        assert_true(key1 < key2);
        assert_false(key2 < key1);

        assert_true(key3 < key4);
        assert_true(key2 < key4);
        assert_true(key1 < key4);

        assert_false(key4 < key3);
        assert_false(key4 < key2);
        assert_false(key4 < key1);

        assert_true(key5 < key3);
        assert_true(key5 < key2);
        assert_true(key5 < key1);

        assert_false(key3 < key5);
        assert_false(key2 < key5);
        assert_false(key1 < key5);
    }

    void test_adding_objects_to_the_hash() {
        AABB box1(Vec3(0.5, 0.5, 0.5), 0.5);
        AABB box2(Vec3(0, 0, 0), 5.0);

        hash_->insert_object_for_box(box1, new_entry_);

        assert_equal(new_entry_->keys().size(), 1);

        hash_->insert_object_for_box(box2, new_entry_);

        assert_equal(new_entry_->keys().size(), 9);
    }

    void test_retrieving_objects_within_a_box() {
        SpatialHashEntry entry1, entry2, entry3;

        AABB box1(Vec3(0.5, 0.5, 0.5), 0.5);
        AABB box2(Vec3(0, 0, 0), 5.0);
        AABB box3(Vec3(10, 10, 10), 1.0);

        hash_->insert_object_for_box(box1, &entry1);
        hash_->insert_object_for_box(box2, &entry2);
        hash_->insert_object_for_box(box3, &entry3);

        auto results = hash_->find_objects_within_box(AABB(Vec3(), 5.0));

        assert_equal(results.size(), 2);

        results = hash_->find_objects_within_box(AABB(Vec3(150.0, 150.0, 150.0), 1.0));

        assert_equal(results.size(), 0);

        results = hash_->find_objects_within_box(AABB(Vec3(), 400));

        assert_equal(results.size(), 3);

        results = hash_->find_objects_within_box(AABB(Vec3(1000, 1000, 1000), 400));

        assert_equal(results.size(), 0);
    }

    void test_inserting_and_getting_flat_objects() {
        SpatialHashEntry entry1, entry2, entry3;

        AABB box1(Vec3(0, 0, 0), Vec3(1, 1, 0));
        AABB box2(Vec3(0, 0, 0), Vec3(1, 0, 1));
        AABB box3(Vec3(0, 0, 0), Vec3(0, 1, 1));

        hash_->insert_object_for_box(box1, &entry1);
        hash_->insert_object_for_box(box2, &entry2);
        hash_->insert_object_for_box(box3, &entry3);

        auto results = hash_->find_objects_within_box(AABB(Vec3(), 5.0));

        assert_equal(results.size(), 3);
    }

    void test_retrieving_objects_within_frustum() {
        SpatialHashEntry entry1, entry2, entry3, entry4;

        AABB box1(Vec3(0.5, 0.5, -0.5), 0.5);
        AABB box2(Vec3(0, 0, -1), 5.0);
        AABB box3(Vec3(10, 10, -200), 1.0);
        AABB box4(Vec3(0, 0, 1), 1.0);

        hash_->insert_object_for_box(box1, &entry1);
        hash_->insert_object_for_box(box2, &entry2);
        hash_->insert_object_for_box(box3, &entry3);
        hash_->insert_object_for_box(box4, &entry4);

        Frustum frustum;

        assert_true(!frustum.initialized());

        //Create an orthographic projection, and a modelview idactor matrix
        kmMat4 projection, modelview;
        kmMat4PerspectiveProjection(&projection, 45.0, 16.0 / 9.0, 0.1, 100.0);
        kmMat4Identity(&modelview);

        //Create the modelview projection matrix
        kmMat4 modelview_projection;
        kmMat4Multiply(&modelview_projection, &projection, &modelview);

        //Build the frustum from the modelview projection matrix
        frustum.build(&modelview_projection);
        assert_true(frustum.initialized());

        auto results = hash_->find_objects_within_frustum(frustum);

        assert_equal(results.size(), 2);
    }

    void test_removing_objects_from_the_hash() {
        test_adding_objects_to_the_hash();

        auto results = hash_->find_objects_within_box(AABB(Vec3(), 10));

        assert_equal(results.size(), 1);

        hash_->remove_object(*results.begin());

        results = hash_->find_objects_within_box(AABB(Vec3(), 10));

        assert_equal(results.size(), 0);
    }

    void test_frustum_box_generation() {
        Frustum frustum;

        assert_true(!frustum.initialized());

        //Create an orthographic projection, and a modelview idactor matrix
        kmMat4 projection, modelview;
        kmMat4PerspectiveProjection(&projection, 45.0, 16.0 / 9.0, 1.0, 100.0);
        kmMat4Identity(&modelview);

        //Create the modelview projection matrix
        kmMat4 modelview_projection;
        kmMat4Multiply(&modelview_projection, &projection, &modelview);

        //Build the frustum from the modelview projection matrix
        frustum.build(&modelview_projection);
        assert_true(frustum.initialized());

        std::vector<AABB> result;
        generate_boxes_for_frustum(frustum, result);

        assert_close(result[0].min.z, -1.0, 0.0001);
        assert_close(result.back().max.z, -101.0, 0.0001);
    }

private:
    smlt::SpatialHash* hash_ = nullptr;
    SpatialHashEntry* new_entry_ = nullptr;

};


}
