#pragma once

#include <kaztest/kaztest.h>

#include "../simulant/partitioners/impl/hgsh.h"

namespace {

using namespace smlt;

class SpatialHashTests : public TestCase {
public:
    void set_up() {
        hash_ = new smlt::HGSH();
        new_entry_ = new HGSHEntry();
    }

    void tear_down() {
        delete hash_;
        delete new_entry_;
    }

    void test_key_construction() {
        Key test1 = make_key(1, 0, 0, 0);
        Key test2 = make_key(2, 0.5, 0, 0);

        assert_equal(15, test1.ancestors);
        assert_true(test1.hash_path[15]);

        assert_equal(14, test2.ancestors);
        assert_true(test2.hash_path[14]);
        assert_equal(0, test2.hash_path[15]);

        assert_true(test2.is_ancestor_of(test1));
        assert_false(test1.is_ancestor_of(test2));
        assert_false(test1.is_ancestor_of(test1)); // Keys are not ancestors of themselves
    }

    void test_key_comparison() {
        Key key1, key2, key3;

        key1.hash_path[0] = 1; key1.hash_path[1] = 2;
        key2.hash_path[0] = 1; key2.hash_path[1] = 3;
        key3.hash_path[0] = 1; key3.hash_path[1] = 1; key3.hash_path[2] = 2;

        assert_true(key3 < key1);
        assert_false(key1 < key3);

        assert_true(key3 < key2);
        assert_false(key2 < key3);

        assert_true(key1 < key2);
        assert_false(key2 < key1);
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
        HGSHEntry entry1, entry2, entry3;

        AABB box1(Vec3(0.5, 0.5, 0.5), 0.5);
        AABB box2(Vec3(0, 0, 0), 5.0);
        AABB box3(Vec3(10, 10, 10), 1.0);

        hash_->insert_object_for_box(box1, &entry1);
        hash_->insert_object_for_box(box2, &entry2);
        hash_->insert_object_for_box(box3, &entry3);

        auto results = hash_->find_objects_within_box(AABB(Vec3(), 10));

        assert_equal(results.size(), 2);

        results = hash_->find_objects_within_box(AABB(Vec3(150.0, 150.0, 150.0), 1.0));

        assert_equal(results.size(), 0);

        results = hash_->find_objects_within_box(AABB(Vec3(), 400));

        assert_equal(results.size(), 3);
    }

    void test_removing_objects_from_the_hash() {
        test_adding_objects_to_the_hash();

        auto results = hash_->find_objects_within_box(AABB(Vec3(), 10));

        assert_equal(results.size(), 1);

        hash_->remove_object(*results.begin());

        results = hash_->find_objects_within_box(AABB(Vec3(), 10));

        assert_equal(results.size(), 0);
    }

private:
    smlt::HGSH* hash_ = nullptr;
    HGSHEntry* new_entry_ = nullptr;

};


}
