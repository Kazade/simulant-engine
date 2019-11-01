#pragma once


#include "../simulant/meshes/adjacency_info.h"
#include "../simulant/asset_manager.h"

namespace {

using namespace smlt;

class AdjacencyTests : public smlt::test::SimulantTestCase {
public:
    void test_basic_adjacency_build() {
        auto mesh = window->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_rectangle("rect", window->shared_assets->new_material(), 1.0, 1.0f);

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
        auto mesh = window->shared_assets->new_mesh(smlt::VertexSpecification::DEFAULT);
        mesh->new_submesh_as_rectangle("rect", window->shared_assets->new_material(), 1.0, 1.0f);

        // Add another vertex, using the same position as the first
        auto i = mesh->vertex_data->count();
        mesh->vertex_data->move_to_end();
        mesh->vertex_data->position(*mesh->vertex_data->position_at<smlt::Vec3>(1));
        mesh->vertex_data->move_next();

        // Add another vertex
        mesh->vertex_data->position(10, 10, 10);
        mesh->vertex_data->move_next();
        mesh->vertex_data->done();

        // Add another triangle
        mesh->first_submesh()->index_data->index(i + 1);
        mesh->first_submesh()->index_data->index(i);
        mesh->first_submesh()->index_data->index(0);
        mesh->first_submesh()->index_data->done();

        auto adjacency = std::make_shared<AdjacencyInfo>(mesh.get());
        adjacency->rebuild();

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

        // Should've detected another shared edges
        assert_equal(2, shared);
    }
};

}
