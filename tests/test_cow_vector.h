#pragma once

#include <simulant/test.h>
#include "../simulant/generic/cow_vector.h"

namespace {

class CoWVectorTests : public smlt::test::TestCase {
public:

    void test_basic_usage() {
        cow_vector<int> a;
        cow_vector<int> b;

        assert_true(a.unique());
        assert_true(b.unique());

        a = b;

        assert_false(a.unique());
        assert_false(b.unique());

        assert_equal(a.data(), b.data());

        a.push_back(1);

        assert_not_equal(a.data(), b.data());

        assert_true(a.unique());
        assert_true(b.unique());

        {
            cow_vector<int> c = b;

            assert_true(a.unique());
            assert_false(b.unique());
            assert_false(c.unique());
        }

        // FIXME? Is it worth it? If c is destroyed
        // b is unique again, so it would potentially
        // save a pointless copy on the next write
        // if we fixed this
        //assert_true(b.unique());
    }
};

}

