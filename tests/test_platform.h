#pragma once

#include "simulant/simulant.h"


namespace {

using namespace smlt;

class PlatformTests : public smlt::test::SimulantTestCase {
public:
    void test_current_user() {
        auto user = smlt::get_platform()->current_user();

        #if defined(__linux__)
        assert_false(user.username.empty());
        #endif
    }
};

}
