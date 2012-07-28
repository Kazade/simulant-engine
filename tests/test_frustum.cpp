#include <unittest++/UnitTest++.h>

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
    frustum.build_frustum(modelview_projection.mat);

    std::vector<kmVec3> near_corners = frustum.near_corners();

    //Bottom left near corner
    CHECK_EQUAL(near_corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT].x, -1.0);
    CHECK_EQUAL(near_corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT].y, -1.0);
    CHECK_EQUAL(near_corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT].z, 1.0); //Near distance

    //Bottom right near corner
    CHECK_EQUAL(near_corners[1].x, 1.0);
    CHECK_EQUAL(near_corners[1].y, -1.0);
    CHECK_EQUAL(near_corners[1].z, 1.0); //Near distance

    //Top right near corner
    CHECK_EQUAL(near_corners[2].x, 1.0);
    CHECK_EQUAL(near_corners[2].y, 1.0);
    CHECK_EQUAL(near_corners[2].z, 1.0); //far distance

    //Top left near corner
    CHECK_EQUAL(near_corners[3].x, -1.0);
    CHECK_EQUAL(near_corners[3].y, 1.0);
    CHECK_EQUAL(near_corners[3].z, 1.0); //far distance
}
