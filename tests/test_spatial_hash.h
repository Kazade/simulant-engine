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

    void test_adding_objects_to_the_hash() {
        AABB box1(Vec3(0.5, 0.5, 0.5), 0.5);
        AABB box2(Vec3(0, 0, 0), 5.0);

        hash_->insert_object_for_box(box1, new_entry_);

        assert_equal(new_entry_->buckets().size(), 1);
        assert_equal(hash_->grid_count(), 1);

        hash_->insert_object_for_box(box2, new_entry_);

        assert_equal(new_entry_->buckets().size(), 9);
        assert_equal(hash_->grid_count(), 2);
    }

    void test_retrieving_objects_within_a_box() {
        test_adding_objects_to_the_hash(); // Populate two grids with the object


        auto results = hash_->find_objects_within_box(AABB(Vec3(), 10));

        assert_equal(results.size(), 1);

        results = hash_->find_objects_within_box(AABB(Vec3(150.0, 150.0, 150.0), 1.0));

        assert_equal(results.size(), 0);
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
