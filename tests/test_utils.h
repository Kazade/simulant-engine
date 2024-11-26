#pragma once

#include "../simulant/utils/float.h"
#include "../simulant/utils/string.h"
#include "simulant/test.h"

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


class FastDivideTests : public smlt::test::TestCase {
public:
    void test_fast_divide() {
        assert_close(fast_divide(1.0f, 2.0f), 1.0f / 2.0f, 0.00001f);
        assert_close(fast_divide(1.0f, -2.0f), 1.0f / -2.0f, 0.00001f);
        assert_close(fast_divide(100.0f, 0.001f), 100.0f / 0.001f, 0.00001f);
        assert_close(fast_divide(-5.0f, 2.0f), -5.0f / 2.0f, 0.00001f);
        assert_close(fast_divide(100000000.0f, 2.0f), 100000000.0f / 2.0f, 0.00001f);
    }
};

class FloatTests: public test::TestCase {
public:
    void test_float10_conversion() {
        float a = 1.25f;
        auto b = float10_from_float(a);
        auto i = b.value();
        assert_false(i.f.unused);
        auto c = float10_to_float(b.value());
        assert_equal(a, c);
    }
};
}
