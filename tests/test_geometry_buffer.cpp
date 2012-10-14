#include <unittest++/UnitTest++.h>

#include "kglt/kglt.h"
#include "kglt/utils/geometry_buffer.h"

using namespace kglt;

TEST(test_geometry_buffer_basic) {
    /*GeometryBuffer buffer1(MESH_ARRANGEMENT_TRIANGLES, VERTEX_ATTRIBUTE_POSITION | VERTEX_ATTRIBUTE_TEXCOORD_1);

    CHECK_EQUAL(0, buffer1.offset(VERTEX_ATTRIBUTE_POSITION));
    CHECK_EQUAL(sizeof(float) * 3, buffer1.offset(VERTEX_ATTRIBUTE_TEXCOORD_1));
    CHECK_EQUAL(-1, buffer1.offset(VERTEX_ATTRIBUTE_DIFFUSE)); //There is no diffuse
    CHECK_EQUAL(sizeof(float) * 5, buffer1.stride()); //Stride should be sizeof pos + uv

    buffer1.resize(1); //Resize to store a single vertex

    CHECK_EQUAL(5, (buffer1.stride() / sizeof(float)) * buffer1.count());

    //Vertex 0 should be the same as the start of the data
    CHECK_EQUAL(buffer1.start(), buffer1.vertex(0));

    buffer1.resize(2);

    CHECK_EQUAL(10, (buffer1.stride() / sizeof(float)) * buffer1.count()); //Data size should now be 10

    //Vertex offsets should be begining + stride
    CHECK_EQUAL(buffer1.start() + (buffer1.stride() / sizeof(float)), buffer1.vertex(1));*/
}
