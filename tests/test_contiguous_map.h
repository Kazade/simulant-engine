#pragma once

#include "../simulant/generic/containers/contiguous_map.h"
#include <simulant/test.h>

namespace {

using namespace smlt;

class ContiguousMultiMapTest: public smlt::test::SimulantTestCase {
public:
    template<typename F, typename... Args>
    float time_execution(uint32_t iterations, const F& function,
                         Args&&... args) {
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

        assert_true(perf1 < (perf2 * 1.10f));
    }

    void test_insertion_performance() {
        ContiguousMultiMap<int, int> CMMap;
        std::multimap<int, int> MMap;

        CMMap.reserve(25 * 100);

        auto f1 = [&CMMap]() {
            for(int i = 0; i < 25; ++i) {
                CMMap.insert(i, i);
            }
        };

        auto f2 = [&MMap]() {
            for(int i = 0; i < 25; ++i) {
                MMap.insert(std::make_pair(i, i));
            }
        };

        float perf1 = time_execution(100, f1);
        float perf2 = time_execution(100, f2);

        /* FIXME: We should be faster, or at least close! (within 10%) */
        assert_true(perf1 < (perf2 * 1.1f));
    }

    void test_complex_insertion() {
        ContiguousMultiMap<int, int> map;

        map.insert(2, 2);
        assert_equal(map.path_key(""), 2);

        map.insert(1, 1);
        assert_equal(map.path_key(""), 2);
        assert_equal(map.path_key("L"), 1);

        map.insert(4, 4);

        assert_equal(map.path_key(""), 2);
        assert_equal(map.path_key("L"), 1);
        assert_equal(map.path_key("R"), 4);

        map.insert(5, 5);

        assert_equal(map.path_key(""), 2);
        assert_equal(map.path_key("L"), 1);
        assert_equal(map.path_key("R"), 4);
        assert_equal(map.path_key("RR"), 5);

        map.insert(9, 9);

        assert_equal(map.path_key(""), 2);
        assert_equal(map.path_key("L"), 1);
        assert_equal(map.path_key("R"), 5);
        assert_equal(map.path_key("RL"), 4);
        assert_equal(map.path_key("RR"), 9);

        map.insert(3, 3);

        assert_equal(map.path_key(""), 2);
        assert_equal(map.path_key("L"), 1);
        assert_equal(map.path_key("R"), 5);
        assert_equal(map.path_key("RL"), 4);
        assert_equal(map.path_key("RR"), 9);
        assert_equal(map.path_key("RLL"), 3);

        map.insert(6, 6);

        assert_equal(map.path_key(""), 2);
        assert_equal(map.path_key("L"), 1);
        assert_equal(map.path_key("R"), 5);
        assert_equal(map.path_key("RL"), 4);
        assert_equal(map.path_key("RR"), 9);
        assert_equal(map.path_key("RLL"), 3);
        assert_equal(map.path_key("RRL"), 6);

        map.insert(7, 7);

        assert_equal(map.path_key(""), 2);
        assert_equal(map.path_key("L"), 1);
        assert_equal(map.path_key("R"), 5);
        assert_equal(map.path_key("RL"), 4);
        assert_equal(map.path_key("RR"), 7);
        assert_equal(map.path_key("RLL"), 3);
        assert_equal(map.path_key("RRL"), 6);
        assert_equal(map.path_key("RRR"), 9);
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

    void test_stable_insertion() {
        ContiguousMultiMap<int, std::string> map;
        map.insert(1, "A");
        map.insert(1, "B");
        map.insert(1, "C");
        map.insert(1, "D");

        std::string result = "";

        for(auto& p: map) {
            result += p.second;
        }

        assert_equal("ABCD", result);
    }

    void test_iteration() {
        ContiguousMultiMap<int, std::string> map;

        map.insert(3, "three");
        map.insert(2, "two");
        map.insert(3, "three");
        map.insert(1, "one");
        map.insert(4, "four");
        map.insert(4, "fourf");
        map.insert(-1, "minus1");
        map.insert(4, "fourff");
        map.insert(-2, "minus2");
        map.insert(-3, "minus3");

        std::vector<int> keys;
        for(auto& p: map) {
            keys.push_back(p.first);
        }

        assert_equal(keys.size(), 10u);
        assert_equal(keys[0], -3);
        assert_equal(keys[1], -2);
        assert_equal(keys[2], -1);
        assert_equal(keys[3], 1);
        assert_equal(keys[4], 2);
        assert_equal(keys[5], 3);
        assert_equal(keys[6], 3);
        assert_equal(keys[7], 4);
        assert_equal(keys[8], 4);
        assert_equal(keys[9], 4);
    }
};

class ContiguousMapTest: public smlt::test::SimulantTestCase {
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
        assert_raises(std::out_of_range, [&]() {
            map.at("third");
        });
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

} // namespace
