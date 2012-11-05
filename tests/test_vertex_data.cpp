#include <unittest++/UnitTest++.h>

#include "kglt/kglt.h"
#include "kglt/vertex_data.h"

using namespace kglt;

SUITE(test_vertex_data) {

    TEST(test_offsets) {
        VertexData::ptr data = VertexData::create();

        CHECK_EQUAL(0, data->position_offset());
        CHECK_EQUAL(sizeof(float) * 3, data->normal_offset());
    }

}
