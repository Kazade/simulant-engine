#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class QuaternionTest : public smlt::test::TestCase {
public:
    void test_equals() {
        Quaternion q1(0, 0, 0, 1);
        Quaternion q2(0, 0, 0, 0.9999);
        Quaternion q3(0, 0, 0, 1);

        assert_false(q1.equals(q2));
        assert_false(q2.equals(q1));
        assert_true(q1.equals(q3));
        assert_true(q3.equals(q1));
    }

    void test_component_constructor() {
        smlt::Quaternion q(1, 2, 3, 4);

        assert_equal(q.x, 1);
        assert_equal(q.y, 2);
        assert_equal(q.z, 3);
        assert_equal(q.w, 4);
    }

    void test_slerp() {
        Quaternion q0(smlt::Degrees(-90), smlt::Degrees(0), smlt::Degrees(0));
        Quaternion q1(smlt::Degrees(90), smlt::Degrees(0), smlt::Degrees(0));

        Quaternion q2 = q0.slerp(q1, 0.0f);

        assert_equal(q0, q2);

        q2 = q0.slerp(q1, 1.0f);

        assert_equal(q1, q2);
    }

    void test_to_from_euler() {
        Quaternion q(smlt::Degrees(90), smlt::Degrees(0), smlt::Degrees(0));

        auto euler = q.to_euler();

        assert_close(smlt::Degrees(90).to_float(), euler.x.to_float(), 0.000001f);
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

        assert_close(quat.x, 0.0f, 0.001f);
        assert_close(quat.y, 0.0f, 0.001f);
        assert_close(quat.z, 0.707f, 0.001f);
        assert_close(quat.w, 0.707f, 0.001f);
    }

    void test_forward_right_up() {
        Quaternion q;

        assert_equal(Vec3(0, 0, -1), q.forward());
        assert_equal(Vec3(1, 0, 0), q.right());
        assert_equal(Vec3(0, 1, 0), q.up());

        q = Quaternion(Vec3(0, 1, 0), Degrees(90));

        assert_close(-1.0f, q.forward().x, 0.0001f);
    }

};

}
