#ifndef TEST_MAT3_H
#define TEST_MAT3_H

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"

class Mat4Test : public TestCase {
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


class Mat3Test : public TestCase {
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
