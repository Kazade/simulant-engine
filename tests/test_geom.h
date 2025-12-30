#pragma once

#include <simulant/simulant.h>
#include <simulant/test.h>
#include "../simulant/nodes/geom.h"

namespace {

using namespace smlt;

class GeomTests : public smlt::test::SimulantTestCase {
public:
    void test_can_set_render_priority() {
        auto mesh = scene->assets->create_mesh(VertexSpecification::DEFAULT);
        auto geom = scene->create_child<Geom>(mesh);

        assert_equal(geom->render_priority(), smlt::RENDER_PRIORITY_MAIN);
        geom->set_render_priority(smlt::RENDER_PRIORITY_NEAR);
        assert_equal(geom->render_priority(), smlt::RENDER_PRIORITY_NEAR);
    }
};

}
