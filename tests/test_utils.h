#pragma once

#include "simulant/test.h"
#include "../simulant/utils/string.h"

namespace {

using namespace smlt;

class StringUtilsTest : public smlt::test::TestCase {
public:
    void test_split() {
        std::string src = "this/is/a/test";

        auto ret = split(src, "/", 2);

        assert_equal(ret.size(), 3u);
        assert_equal("this", ret[0]);
        assert_equal("is", ret[1]);
        assert_equal("a/test", ret[2]);
    }

    void test_strip() {
        std::string src = " \nthings\r\t";

        auto ret = strip(src);

        assert_equal("things", ret);

        src = "\t\n\r ";
        ret = strip(src);

        assert_equal("", ret);
    }

    void test_ends_with() {
        std::string src = "apple";

        assert_true(ends_with(src, "ple"));
        assert_false(ends_with(src, "anas"));
        assert_false(ends_with(src, "bananas"));
    }
};


}
