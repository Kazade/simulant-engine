#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class HeightmapTests : public test::SimulantTestCase {
public:
    void test_basic_usage() {
        auto stage = window->new_stage();
        auto path = "flare.tga";
        auto tex = stage->assets->new_texture_from_file(path);
        auto heightmap = stage->assets->new_mesh_from_heightmap(path, HeightmapSpecification());

        assert_equal(heightmap->data->get<TerrainData>("terrain_data").x_size, tex->width());
        assert_equal(heightmap->data->get<TerrainData>("terrain_data").z_size, tex->height());
    }

};

}
