#ifndef TEST_MAT3_H
#define TEST_MAT3_H

#include "simulant/simulant.h"
#include "simulant/test.h"

class Mat4Test : public smlt::test::TestCase {
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

        assert_close(m[0], acosf(smlt::Radians(smlt::Degrees(90)).value), 0.0000001);
        assert_close(m[1], asinf(smlt::Radians(smlt::Degrees(90)).value), 0.0000001);
        assert_close(m[2], 0, 0.0000001);
        assert_close(m[3], 0, 0.0000001);

        assert_close(m[4], -asinf(smlt::Radians(smlt::Degrees(90)).value), 0.0000001);
        assert_close(m[5], acosf(smlt::Radians(smlt::Degrees(90)).value), 0.0000001);
        assert_close(m[6], 0, 0.0000001);
        assert_close(m[7], 0, 0.0000001);

        assert_close(m[8], 0, 0.0000001);
        assert_close(m[9], 0, 0.0000001);
        assert_close(m[10], 1, 0.0000001);
        assert_close(m[11], 0, 0.0000001);

        assert_close(m[12], 0, 0.0000001);
        assert_close(m[13], 0, 0.0000001);
        assert_close(m[14], 0, 0.0000001);
        assert_close(m[15], 1, 0.0000001);
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
};


class Mat3Test : public smlt::test::TestCase {
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

#endif // TEST_MAT3_H
