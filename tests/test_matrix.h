#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class Mat4Test: public smlt::test::TestCase {
public:
    void test_default_constructor() {
        smlt::Mat4 mat;

        for(uint32_t i = 0; i < 16; ++i) {
            if(i == 0 || i == 5 || i == 10 || i == 15) {
                assert_equal(mat[i], 1);
            } else {
                assert_equal(mat[i], 0);
            }
        }
    }

    void test_from_rotation_z() {
        smlt::Mat4 m = smlt::Mat4::as_rotation_z(smlt::Degrees(90));

        assert_close(m[0],
                     std::acos(smlt::Radians(smlt::Degrees(90)).to_float()),
                     0.0000001f);
        assert_close(m[1],
                     std::asin(smlt::Radians(smlt::Degrees(90)).to_float()),
                     0.0000001f);
        assert_close(m[2], 0.0f, 0.0000001f);
        assert_close(m[3], 0.0f, 0.0000001f);

        assert_close(m[4],
                     -std::asin(smlt::Radians(smlt::Degrees(90)).to_float()),
                     0.0000001f);
        assert_close(m[5],
                     std::acos(smlt::Radians(smlt::Degrees(90)).to_float()),
                     0.0000001f);
        assert_close(m[6], 0.0f, 0.0000001f);
        assert_close(m[7], 0.0f, 0.0000001f);

        assert_close(m[8], 0.0f, 0.0000001f);
        assert_close(m[9], 0.0f, 0.0000001f);
        assert_close(m[10], 1.0f, 0.0000001f);
        assert_close(m[11], 0.0f, 0.0000001f);

        assert_close(m[12], 0.0f, 0.0000001f);
        assert_close(m[13], 0.0f, 0.0000001f);
        assert_close(m[14], 0.0f, 0.0000001f);
        assert_close(m[15], 1.0f, 0.0000001f);
    }

    void test_assignment() {
        smlt::Mat4 b;

        for(uint32_t i = 0; i < 16; ++i) {
            b[i] = i;
        }

        smlt::Mat4 a;
        a = b;

        for(uint32_t i = 0; i < 16; ++i) {
            assert_equal(a[i], i);
        }
    }

    void test_as_scaling() {
        Mat4 a = Mat4::as_scale(Vec3(1.0f));
        assert_close(a[0], 1.0f, 0.00001f);
        assert_close(a[5], 1.0f, 0.00001f);
        assert_close(a[10], 1.0f, 0.00001f);
        assert_close(a[15], 1.0f, 0.00001f);
    }

    void test_trs() {
        auto trans = Vec3(10.0f, 20.0f, 30.0f);
        auto rot = Quaternion(0.259f, 0.0f, 0.0f, 0.966f);
        auto scale = Vec3(2.0f, 1.0f, 0.5f);

        auto t = Mat4::as_translation(trans);
        auto r = Mat4::as_rotation(rot);
        auto s = Mat4::as_scale(scale);

        auto trs = t * r * s;

        const float E = 0.00001f;
        assert_close(trs[0], 2.0f, E);
        assert_close(trs[1], 0.0f, E);
        assert_close(trs[2], 0.0f, E);
        assert_close(trs[3], 0.0f, E);

        assert_close(trs[4], 0.0f, E);
        assert_close(trs[5], 0.866f, E);
        assert_close(trs[6], 0.5f, E);
        assert_close(trs[7], 0.0f, E);

        assert_close(trs[8], 0.0f, E);
        assert_close(trs[9], -0.25f, E);
        assert_close(trs[10], 0.433f, E);
        assert_close(trs[11], 0.0f, E);

        assert_close(trs[12], 10.0f, E);
        assert_close(trs[13], 20.0f, E);
        assert_close(trs[14], 30.0f, E);
        assert_close(trs[15], 1.0f, E);
    }
};

class Mat3Test: public smlt::test::TestCase {
public:
    void test_default_constructor() {
        smlt::Mat3 mat;

        for(uint32_t i = 0; i < 9; ++i) {
            if(i == 0 || i == 4 || i == 8) {
                assert_equal(mat[i], 1);
            } else {
                assert_equal(mat[i], 0);
            }
        }
    }

    void test_assignment() {
        smlt::Mat3 b;

        for(uint32_t i = 0; i < 9; ++i) {
            b[i] = i;
        }

        smlt::Mat3 a;
        a = b;

        for(uint32_t i = 0; i < 9; ++i) {
            assert_equal(a[i], i);
        }
    }
};

} // namespace
