#pragma once

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"

class QuaternionTest : public TestCase {
public:

    void test_component_constructor() {
        smlt::Quaternion q(1, 2, 3, 4);

        assert_equal(q.x, 1);
        assert_equal(q.y, 2);
        assert_equal(q.z, 3);
        assert_equal(q.w, 4);
    }

    void test_to_from_euler() {
        Quaternion q(smlt::Degrees(90), smlt::Degrees(0), smlt::Degrees(0));

        auto euler = q.to_euler();

        assert_true(smlt::Degrees(90) == euler.x);
        assert_true(smlt::Degrees(0) == euler.y);
        assert_true(smlt::Degrees(0) == euler.z);
    }

    void test_mat3_to_quaternion() {
        smlt::Mat3 mat;
        for(uint32_t i = 0; i < 9; ++i) mat[i] = 0;

        // 90 degree rotation around z
        mat[1] = 1;
        mat[3] = -1;
        mat[8] = 1;

        smlt::Quaternion quat(mat);

        assert_close(quat.x, 0, 0.001);
        assert_close(quat.y, 0, 0.001);
        assert_close(quat.z, 0.707, 0.001);
        assert_close(quat.w, 0.707, 0.001);
    }

};
