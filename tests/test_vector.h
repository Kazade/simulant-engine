#ifndef TEST_VECTOR_H
#define TEST_VECTOR_H

#include "simulant/simulant.h"
#include "simulant/test.h"

class Vec4Test : public smlt::test::TestCase {
public:
    void test_component_constructor() {
        smlt::Vec4 v(1, 2, 3, 4);

        assert_equal(v.x, 1);
        assert_equal(v.y, 2);
        assert_equal(v.z, 3);
        assert_equal(v.w, 4);
    }

    void test_vec3_constructor() {
        smlt::Vec4 v(smlt::Vec3(1, 2, 3), 4);
        assert_equal(v.x, 1);
        assert_equal(v.y, 2);
        assert_equal(v.z, 3);
        assert_equal(v.w, 4);
    }

    void test_default_constructor() {
        smlt::Vec4 v;
        assert_equal(v.x, 0);
        assert_equal(v.y, 0);
        assert_equal(v.z, 0);
        assert_equal(v.w, 0);
    }

    void test_assignment() {
        smlt::Vec4 b(1, 2, 3, 4);
        smlt::Vec4 a;

        a = b;

        assert_equal(a.x, 1);
        assert_equal(a.y, 2);
        assert_equal(a.z, 3);
        assert_equal(a.w, 4);
    }
};

class Vec3Test : public smlt::test::TestCase {
public:
    void test_component_constructor() {
        smlt::Vec3 v(1, 2, 3);

        assert_equal(v.x, 1);
        assert_equal(v.y, 2);
        assert_equal(v.z, 3);
    }

    void test_vec2_constructor() {
        smlt::Vec3 v(smlt::Vec2(1, 2), 3);
        assert_equal(v.x, 1);
        assert_equal(v.y, 2);
        assert_equal(v.z, 3);
    }

    void test_default_constructor() {
        smlt::Vec3 v;
        assert_equal(v.x, 0);
        assert_equal(v.y, 0);
        assert_equal(v.z, 0);
    }

    void test_assignment() {
        smlt::Vec3 b(1, 2, 3);
        smlt::Vec3 a;

        a = b;

        assert_equal(a.x, 1);
        assert_equal(a.y, 2);
        assert_equal(a.z, 3);
    }

    void test_rotated_by_mat4() {
        smlt::Mat4 rot = smlt::Mat4::as_rotation_y(smlt::Degrees(90));

        smlt::Vec3 v1(1, 0, 0);

        auto ret = v1.rotated_by(rot);

        assert_close(ret.x, 0.0f, 0.00001f);
        assert_close(ret.y, 0.0f, 0.00001f);
        assert_close(ret.z, -1.0f, 0.00001f);
    }
};

class Vec2Test : public smlt::test::TestCase {
public:
    void test_component_constructor() {
        smlt::Vec2 v(1, 2);

        assert_equal(v.x, 1);
        assert_equal(v.y, 2);
    }

    void test_default_constructor() {
        smlt::Vec2 v;
        assert_equal(v.x, 0);
        assert_equal(v.y, 0);
    }

    void test_assignment() {
        smlt::Vec2 b(1, 2);
        smlt::Vec2 a;

        a = b;

        assert_equal(a.x, 1);
        assert_equal(a.y, 2);
    }

    void test_vec2_rotated_by() {
        smlt::Vec2 up(0, 1);

        assert_close(1, up.rotated_by(smlt::Degrees(-90)).x, 0.0000001);
        assert_close(0, up.rotated_by(smlt::Degrees(-90)).y, 0.0000001);

        assert_close(-1, up.rotated_by(smlt::Degrees(90)).x, 0.0000001);
        assert_close(0, up.rotated_by(smlt::Degrees(90)).y, 0.0000001);
    }

    void test_vec2_scale() {
        smlt::Vec2 up(0, 1);

        assert_equal(smlt::Vec2(0, 10), up * 10);

        up *= 10;

        assert_equal(smlt::Vec2(0, 10), up);
    }
};

#endif // TEST_VECTOR_H
