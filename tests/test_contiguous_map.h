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

    void test_iteration() {
        ContiguousMap<int, std::string> map;

        map.insert(3, "three");
        map.insert(2, "two");
        map.insert(4, "four");
        map.insert(1, "one");

        std::vector<int> keys;
        for(auto& p: map) {
            keys.push_back(p.first);
        }

        assert_equal(keys[0], 1);
        assert_equal(keys[1], 2);
        assert_equal(keys[2], 3);
        assert_equal(keys[3], 4);
    }
};

}
