#pragma once

#include <memory>
#include <string>

#include <simulant/test.h>
#include "../simulant/generic/optional.h"

namespace {

using namespace smlt;

class OptionalTest : public smlt::test::SimulantTestCase {
public:
    void test_move_operator() {
        smlt::optional<int> int1 = 42;
        smlt::optional<int> int2 = std::move(int1);

        assert_true(int2.has_value());
        assert_equal(int2.value(), 42);
    }

    void test_destruction() {
        bool called = false;
        struct Thing {
            Thing(bool& called):
                called(called) {}

            ~Thing() {
                called = true;
            }

            bool& called;
        };

        Thing t(called);
        {
            smlt::optional<Thing> ot = std::move(t);
        }

        assert_true(called);
    }

    void test_value_or_without_a_value() {
        smlt::optional<std::string> empty;
        const smlt::optional<std::string> const_empty = smlt::no_value;

        assert_equal(empty.value_or("default"), "default");
        assert_equal(const_empty.value_or("default"), "default");
        assert_equal(smlt::optional<std::string>().value_or("default"), "default");
    }

    void test_value_or_leaves_the_value_in_place() {
        smlt::optional<std::string> text = std::string("some text long enough to be on the heap");

        // A temporary default, and a default that's an lvalue
        std::string def = "default";
        assert_equal(text.value_or(""), "some text long enough to be on the heap");
        assert_equal(text.value_or(def), "some text long enough to be on the heap");

        assert_true(text.has_value());
        assert_equal(text.value(), "some text long enough to be on the heap");
        assert_equal(def, "default");
    }

    void test_value_or_on_a_const_optional() {
        const smlt::optional<std::string> text = std::string("text");

        assert_equal(text.value_or(""), "text");
        assert_equal(text.value(), "text");
    }

    void test_value_or_moves_out_of_a_temporary() {
        // These can't be copied, so this can only work by moving
        auto value = smlt::optional<std::unique_ptr<int>>(std::unique_ptr<int>(new int(42)))
            .value_or(std::unique_ptr<int>());
        assert_equal(*value, 42);

        auto def = smlt::optional<std::unique_ptr<int>>()
            .value_or(std::unique_ptr<int>(new int(7)));
        assert_equal(*def, 7);
    }
};

}
