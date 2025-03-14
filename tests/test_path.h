#pragma once

#include <simulant/test.h>

namespace {

using namespace smlt;

class PathTests: public smlt::test::SimulantTestCase {
public:
    void test_replace_ext() {
        Path p1("/etc/my_path");
        Path p2("/etc/my_path.png");

        assert_equal(p1.replace_ext("dtex").str(), "/etc/my_path.dtex");
        assert_equal(p2.replace_ext("dtex").str(), "/etc/my_path.dtex");
        assert_equal(p1.replace_ext(".dtex").str(), "/etc/my_path.dtex");
    }

    void test_append() {
        Path p1("/etc/my_path");
        assert_equal(p1.append("another_path").normalize().str(),
                     Path("/etc/my_path/another_path").normalize().str());
    }

    void test_is_absolute() {
#ifdef _WIN32
        Path p1("C:\\etc\\my_path");
        Path p2("another_path");
#else
        Path p1("/etc/my_path");
        Path p2("another_path");
#endif

        assert_true(p1.is_absolute());
        assert_false(p2.is_absolute());
    }
};

} // namespace
