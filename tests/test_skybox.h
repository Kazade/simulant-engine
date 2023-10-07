#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class SkyboxTest : public smlt::test::SimulantTestCase {
public:
    void test_skybox_from_folder() {
        auto sky = scene->create_child<Skybox>("skyboxes/TropicalSunnyDay");

        assert_equal(sky->child_count(), 1u); // Should have 1 child (the actor)
    }

    void test_skybox_from_files() {

    }
};


}
