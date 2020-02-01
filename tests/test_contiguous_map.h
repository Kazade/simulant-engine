#pragma once


#include <simulant/test.h>
#include "../simulant/generic/containers/contiguous_map.h"

namespace {

using namespace smlt;

class ContiguousMapTest : public smlt::test::SimulantTestCase {
public:
    void test_construction() {
        ContiguousMap<std::string, int> map(5);

        assert_true(map.empty());
        assert_false(map.size());
    }

    void test_insertion() {
        ContiguousMap<std::string, int> map;

        map.insert("first", 1);
        map.insert("second", 2);

        assert_equal(map.size(), 2u);

        map.clear();

        assert_equal(map.size(), 0u);
        assert_true(map.empty());
    }

    void test_find() {
        ContiguousMap<std::string, int> map;

        map.insert("first", 1);
        map.insert("second", 2);

        assert_equal(map.at("first"), 1);
        assert_equal(map.at("second"), 2);
        assert_raises(
            std::out_of_range,
            std::bind(&ContiguousMap<std::string, int>::at, &map, "third")
        );
    }
};

}
