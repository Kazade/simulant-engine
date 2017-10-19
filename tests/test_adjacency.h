#pragma once

#include "global.h"
#include "../simulant/meshes/adjacency_info.h"

namespace {

using namespace smlt;

class AdjacencyTests : public SimulantTestCase {
public:
    void test_basic_adjacency_build() {
        auto mesh = window->shared_assets->new_mesh_as_rectangle(1.0, 1.0f).fetch();

        auto adjacency = std::make_shared<AdjacencyInfo>(mesh.get());
        adjacency->rebuild();

        // 2 triangles, with a shared edge
        assert_equal(5u, adjacency->edge_count());

        auto unshared = 0;
        auto shared = 0;

        // Count the shared edges, make sure there is only one
        adjacency->each_edge([&](std::size_t i, const EdgeInfo& edge) {
            if(edge.triangle_count == 2) {
                shared++;
            } else if(edge.triangle_count == 1) {
                unshared++;
            }
        });

        assert_equal(4, unshared);
        assert_equal(1, shared);
    }

    void test_shared_positions_detected() {

    }

    void test_rebuild_on_index_data_change() {

    }
};

}
