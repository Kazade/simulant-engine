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


class StringFormatterTests : public smlt::test::TestCase {
public:
    void test_formatting_int8() {
        assert_equal(
            _F("{0}").format((int8_t) 1),
            "1"
        );

        assert_equal(
            _F("{0} {1}").format((int8_t) 1, 2),
            "1 2"
        );
    }

    void test_basic_formatting() {
        assert_equal(
            _F("{0} {0} {1} {2}").format(1, 2, 3),
            "1 1 2 3"
        );
    }

    void test_precision_formatting() {
        assert_equal(
            _F("{0:.3} {0:.3} {1:.4} {2:.2}").format(11.1111, 2.2222, 3.3333),
            "11.1 11.1 2.222 3.3"
        );
    }
};

}
