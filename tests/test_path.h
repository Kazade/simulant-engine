#pragma once

#include <simulant/test.h>

namespace {

using namespace smlt;

class PathTests : public smlt::test::SimulantTestCase {
public:

    void test_replace_ext() {
        Path p1("/etc/my_path");
        Path p2("/etc/my_path.png");

        assert_equal(p1.replace_ext("dtex").str(), "/etc/my_path.dtex");
        assert_equal(p2.replace_ext("dtex").str(), "/etc/my_path.dtex");
        assert_equal(p1.replace_ext(".dtex").str(), "/etc/my_path.dtex");
    }
};

}
