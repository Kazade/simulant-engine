#pragma once


#include "../simulant/generic/manual_manager.h"
#include "../simulant/generic/unique_id.h"

namespace {

using namespace smlt;

class TestObject;

typedef UniqueID<TestObject*> TestObjectID;

class TestObject {
public:
    TestObject(TestObjectID id): id_(id) {}
    bool init() { return true; }
    void cleanup();

    TestObjectID id() const { return id_; }

private:
    TestObjectID id_;
};


class ManualManagerTest : public smlt::test::SimulantTestCase {
public:
    void test_adding_more_than_64_elements() {
        generic::ManualManager<TestObject, TestObjectID> manager;

        for(auto i = 0; i < 66; ++i) {
            auto id = manager.make();
            assert_true(id);

            // This weird test actually is for testing the getter which had problems
            // when a new buffer as allocated
            assert_equal(id.fetch()->id(), id);
        }

        assert_equal(66u, manager.count());
    }
};


}
