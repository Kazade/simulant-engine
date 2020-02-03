#pragma once


#include <simulant/test.h>
#include "../simulant/generic/containers/contiguous_map.h"

namespace {

using namespace smlt;


class ContiguousMultiMapTest : public smlt::test::SimulantTestCase {
public:

    template<typename F, typename... Args>
    float time_execution(uint32_t iterations, const F& function, Args&& ...args) {
        auto start = std::chrono::high_resolution_clock::now();
        auto i = iterations;
        while(i--) {
            function(std::forward<Args>(args)...);
        }
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<float, std::milli> diff = end - start;
        return diff.count();
    }

    void test_performance() {
        ContiguousMultiMap<int, int> CMMap;
        std::multimap<int, int> MMap;

        auto f1 = [&CMMap]() {
            for(int i = 0; i < 10; ++i) {
                CMMap.insert(i, i);
            }

            CMMap.clear();
        };

        auto f2 = [&MMap]() {
            for(int i = 0; i < 10; ++i) {
                MMap.insert(std::make_pair(i, i));
            }

            MMap.clear();
        };

        float perf1 = time_execution(10000, f1);
        float perf2 = time_execution(10000, f2);

        assert_true(perf1 < perf2);
    }

    void test_insertion_performance() {
        ContiguousMultiMap<int, int> CMMap;
        std::multimap<int, int> MMap;

        auto f1 = [&CMMap]() {
            for(int i = 0; i < 100; ++i) {
                CMMap.insert(i, i);
            }
        };

        auto f2 = [&MMap]() {
            for(int i = 0; i < 100; ++i) {
                MMap.insert(std::make_pair(i, i));
            }
        };

        float perf1 = time_execution(1000, f1);
        float perf2 = time_execution(1000, f2);

        /* FIXME: We should be faster! */
        assert_close(perf1, perf2, 25.0f);
    }

    void test_construction() {
        ContiguousMultiMap<std::string, int> map(5);

        assert_true(map.empty());
        assert_false(map.size());
    }

    void test_upper_bound() {
        ContiguousMultiMap<std::string, int> map;

        map.insert("00000", 0);
        map.insert("00001", 1);
        map.insert("00001", 1);
        map.insert("00001", 1);
        map.insert("00002", 2);

        assert_equal(map.upper_bound("00001")->second, 2);
    }

    void test_insertion() {
        ContiguousMultiMap<std::string, int> map;

        map.insert("first", 1);
        map.insert("second", 2);
        map.insert("first", 3);

        assert_equal(map.size(), 3u);

        map.clear();

        assert_equal(map.size(), 0u);
        assert_true(map.empty());
    }

    void test_iteration() {
        ContiguousMultiMap<int, std::string> map;

        map.insert(3, "three");
        map.insert(2, "two");
        map.insert(3, "three");
        map.insert(1, "one");

        std::vector<int> keys;
        for(auto& p: map) {
            keys.push_back(p.first);
        }

        assert_equal(keys[0], 1);
        assert_equal(keys[1], 2);
        assert_equal(keys[2], 3);
        assert_equal(keys[3], 3);
    }
};

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
        skip_if(true, "Not yet implemented");
/*
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
        assert_equal(keys[3], 4); */
    }
};

}
