#ifndef TEST_STATIC_CHUNK_H
#define TEST_STATIC_CHUNK_H

#include <kaztest/kaztest.h>
#include "kglt/partitioners/static_chunk.h"

class PolygonTests:
    public TestCase {

public:
    void test_bounding_box_updates_correctly() {

        kglt::Polygon polygon;

        polygon.recalc_bounds();

        assert_true(kglt::Vec3(polygon.bounding_box.min) == kglt::Vec3());
        assert_true(kglt::Vec3(polygon.bounding_box.max) == kglt::Vec3());

        kglt::VertexData data;
        kglt::Vec3 new_p(10, 10, 10);
        data.position(new_p);

        polygon.source_data = &data;
        polygon.indices.push_back(0);

        polygon.recalc_bounds();

        assert_true(kglt::Vec3(polygon.bounding_box.min) == new_p);
        assert_true(kglt::Vec3(polygon.bounding_box.max) == new_p);
    }
};


#endif // TEST_STATIC_CHUNK_H
