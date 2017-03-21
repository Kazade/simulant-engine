#ifndef TEST_FRUSTUM_H
#define TEST_FRUSTUM_H

#include "kaztest/kaztest.h"

#include "simulant/simulant.h"
#include "global.h"

using namespace smlt;

class FrustumTest : public SimulantTestCase {
public:
    void test_frustum_generation() {
        Frustum frustum;

        assert_true(!frustum.initialized());

        //Create an orthographic projection, and a modelview idactor matrix
        Mat4 projection = Mat4::as_orthographic(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
        Mat4 modelview;

        //Create the modelview projection matrix
        Mat4 modelview_projection = projection * modelview;

        //Build the frustum from the modelview projection matrix
        frustum.build(&modelview_projection);
        assert_true(frustum.initialized());

        std::vector<Vec3> near_corners = frustum.near_corners();

        //Bottom left near corner
        assert_close(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_LEFT].x, 0.00001);
        assert_close(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_LEFT].y, 0.00001);
        assert_close(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_LEFT].z, 0.00001); //Near distance

        //Bottom right near corner
        assert_close(1.0, near_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].x, 0.00001);
        assert_close(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].y, 0.00001);
        assert_close(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].z, 0.00001); //Near distance

        //Top right near corner
        assert_close(1.0, near_corners[FRUSTUM_CORNER_TOP_RIGHT].x, 0.00001);
        assert_close(1.0, near_corners[FRUSTUM_CORNER_TOP_RIGHT].y, 0.00001);
        assert_close(-1.0, near_corners[FRUSTUM_CORNER_TOP_RIGHT].z, 0.00001); //Near distance

        //Top left near corner
        assert_close(-1.0, near_corners[FRUSTUM_CORNER_TOP_LEFT].x, 0.00001);
        assert_close(1.0, near_corners[FRUSTUM_CORNER_TOP_LEFT].y, 0.00001);
        assert_close(-1.0, near_corners[FRUSTUM_CORNER_TOP_LEFT].z, 0.00001); //Near distance

        std::vector<Vec3> far_corners = frustum.far_corners();

        //Bottom left near corner
        assert_close(-1.0, far_corners[FRUSTUM_CORNER_BOTTOM_LEFT].x, 0.00001);
        assert_close(-1.0, far_corners[FRUSTUM_CORNER_BOTTOM_LEFT].y, 0.00001);
        assert_close(-10.0, far_corners[FRUSTUM_CORNER_BOTTOM_LEFT].z, 0.00001); //Near distance

        //Bottom right near corner
        assert_close(1.0, far_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].x, 0.00001);
        assert_close(-1.0, far_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].y, 0.00001);
        assert_close(-10.0, far_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].z, 0.00001); //Near distance

        //Top right near corner
        assert_close(1.0, far_corners[FRUSTUM_CORNER_TOP_RIGHT].x, 0.00001);
        assert_close(1.0, far_corners[FRUSTUM_CORNER_TOP_RIGHT].y, 0.00001);
        assert_close(-10.0, far_corners[FRUSTUM_CORNER_TOP_RIGHT].z, 0.00001); //Near distance

        //Top left near corner
        assert_close(-1.0, far_corners[FRUSTUM_CORNER_TOP_LEFT].x, 0.00001);
        assert_close(1.0, far_corners[FRUSTUM_CORNER_TOP_LEFT].y, 0.00001);
        assert_close(-10.0, far_corners[FRUSTUM_CORNER_TOP_LEFT].z, 0.00001); //Near distance

        assert_close(2.0, frustum.near_height(), 0.0001);
        assert_close(2.0, frustum.far_height(), 0.0001);
        assert_close(9.0, frustum.depth(), 0.0001);
    }

    void test_aspect_ratio() {
        Frustum frustum;

        assert_true(!frustum.initialized());

        //Create an orthographic projection, and a modelview idactor matrix
        Mat4 projection = Mat4::as_orthographic(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
        Mat4 modelview;

        //Create the modelview projection matrix
        Mat4 modelview_projection = projection * modelview;

        //Build the frustum from the modelview projection matrix
        frustum.build(&modelview_projection);
        assert_true(frustum.initialized());

        assert_close(frustum.aspect_ratio(), 1.0, 0.0001);
    }

    void test_field_of_view() {
        Frustum frustum;

        assert_true(!frustum.initialized());

        //Create an orthographic projection, and a modelview idactor matrix
        Mat4 projection = Mat4::as_projection(45.0, 16.0 / 9.0, 1.0, 100.0);
        Mat4 modelview;

        //Create the modelview projection matrix
        Mat4 modelview_projection = projection * modelview;

        //Build the frustum from the modelview projection matrix
        frustum.build(&modelview_projection);
        assert_true(frustum.initialized());

        assert_close(frustum.field_of_view().value, 45.0, 0.0001);
    }

    void test_depth() {
        Frustum frustum;

        assert_true(!frustum.initialized());

        //Create an orthographic projection, and a modelview idactor matrix
        Mat4 projection = Mat4::as_projection(45.0, 16.0 / 9.0, 1.0, 100.0);
        Mat4 modelview;

        //Create the modelview projection matrix
        Mat4 modelview_projection = projection * modelview;

        //Build the frustum from the modelview projection matrix
        frustum.build(&modelview_projection);
        assert_true(frustum.initialized());

        assert_close(frustum.depth(), 99.0, 0.001);
    }
};

#endif // TEST_FRUSTUM_H
