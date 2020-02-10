#pragma once

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
};

}
