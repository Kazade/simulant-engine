#pragma once

#include <simulant/test.h>
#include <simulant/color.h>

namespace {

using namespace smlt;

class PackedColorTests : public smlt::test::SimulantTestCase {
public:
    void test_conversion_to_and_from_color() {
        PackedColor4444 pc = Color::YELLOW;

        assert_equal(pc.r8(), 255);
        assert_equal(pc.g8(), 255);
        assert_equal(pc.b8(), 0);
        assert_equal(pc.a8(), 255);

        assert_equal(pc.rf(), 1.0f);
        assert_equal(pc.gf(), 1.0f);
        assert_equal(pc.bf(), 0.0f);
        assert_equal(pc.af(), 1.0f);

        Color c = pc;

        assert_equal(c, Color::YELLOW);
    }
};

}
