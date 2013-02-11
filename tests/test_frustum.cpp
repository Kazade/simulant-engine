#include <UnitTest++.h>

#include <vector>

#include "kglt/kglt.h"

using namespace kglt;

TEST(test_frustum_generation) {
    Frustum frustum;

    CHECK(!frustum.initialized());

    //Create an orthographic projection, and a modelview identity matrix
    kmMat4 projection, modelview;
    kmMat4OrthographicProjection(&projection, -1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
    kmMat4Identity(&modelview);

    //Create the modelview projection matrix
    kmMat4 modelview_projection;
    kmMat4Multiply(&modelview_projection, &projection, &modelview);

    //Build the frustum from the modelview projection matrix
    frustum.build(&modelview_projection);
    CHECK(frustum.initialized());

    std::vector<kmVec3> near_corners = frustum.near_corners();

    //Bottom left near corner
    CHECK_CLOSE(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_LEFT].x, 0.00001);
    CHECK_CLOSE(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_LEFT].y, 0.00001);
    CHECK_CLOSE(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_LEFT].z, 0.00001); //Near distance

    //Bottom right near corner
    CHECK_CLOSE(1.0, near_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].x, 0.00001);
    CHECK_CLOSE(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].y, 0.00001);
    CHECK_CLOSE(-1.0, near_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].z, 0.00001); //Near distance

    //Top right near corner
    CHECK_CLOSE(1.0, near_corners[FRUSTUM_CORNER_TOP_RIGHT].x, 0.00001);
    CHECK_CLOSE(1.0, near_corners[FRUSTUM_CORNER_TOP_RIGHT].y, 0.00001);
    CHECK_CLOSE(-1.0, near_corners[FRUSTUM_CORNER_TOP_RIGHT].z, 0.00001); //Near distance

    //Top left near corner
    CHECK_CLOSE(-1.0, near_corners[FRUSTUM_CORNER_TOP_LEFT].x, 0.00001);
    CHECK_CLOSE(1.0, near_corners[FRUSTUM_CORNER_TOP_LEFT].y, 0.00001);
    CHECK_CLOSE(-1.0, near_corners[FRUSTUM_CORNER_TOP_LEFT].z, 0.00001); //Near distance

    std::vector<kmVec3> far_corners = frustum.far_corners();

    //Bottom left near corner
    CHECK_CLOSE(-1.0, far_corners[FRUSTUM_CORNER_BOTTOM_LEFT].x, 0.00001);
    CHECK_CLOSE(-1.0, far_corners[FRUSTUM_CORNER_BOTTOM_LEFT].y, 0.00001);
    CHECK_CLOSE(-10.0, far_corners[FRUSTUM_CORNER_BOTTOM_LEFT].z, 0.00001); //Near distance

    //Bottom right near corner
    CHECK_CLOSE(1.0, far_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].x, 0.00001);
    CHECK_CLOSE(-1.0, far_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].y, 0.00001);
    CHECK_CLOSE(-10.0, far_corners[FRUSTUM_CORNER_BOTTOM_RIGHT].z, 0.00001); //Near distance

    //Top right near corner
    CHECK_CLOSE(1.0, far_corners[FRUSTUM_CORNER_TOP_RIGHT].x, 0.00001);
    CHECK_CLOSE(1.0, far_corners[FRUSTUM_CORNER_TOP_RIGHT].y, 0.00001);
    CHECK_CLOSE(-10.0, far_corners[FRUSTUM_CORNER_TOP_RIGHT].z, 0.00001); //Near distance

    //Top left near corner
    CHECK_CLOSE(-1.0, far_corners[FRUSTUM_CORNER_TOP_LEFT].x, 0.00001);
    CHECK_CLOSE(1.0, far_corners[FRUSTUM_CORNER_TOP_LEFT].y, 0.00001);
    CHECK_CLOSE(-10.0, far_corners[FRUSTUM_CORNER_TOP_LEFT].z, 0.00001); //Near distance

    CHECK_CLOSE(2.0, frustum.near_height(), 0.0001);
    CHECK_CLOSE(2.0, frustum.far_height(), 0.0001);
    CHECK_CLOSE(9.0, frustum.depth(), 0.0001);
}
