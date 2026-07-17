#pragma once

#include <array>

#include <simulant/simulant.h>
#include <simulant/test.h>

namespace {

using namespace smlt;

class SubMeshTests: public smlt::test::SimulantTestCase {
public:
    // Regression test: SubMesh::each_triangle() must apply the same
    // odd/even winding-parity flip for indexed TRIANGLE_STRIP submeshes
    // that it already applied for ranged (non-indexed) ones. The indexed
    // path used to emit every triangle in the strip's raw (a, b, c) order
    // regardless of position, which reverses the winding of every other
    // triangle - breaking anything that derives normals/winding from
    // each_triangle() (adjacency info, shadow silhouettes, physics
    // collision meshes, the octree/quadtree geometry cullers).
    void test_indexed_triangle_strip_matches_ranged_winding() {
        auto material = application->shared_assets->create_material();
        auto mesh =
            application->shared_assets->create_mesh(VertexSpecification::DEFAULT);

        // Five vertices in strip order. An indexed submesh referencing them
        // as [0, 1, 2, 3, 4] has identical connectivity to a ranged submesh
        // covering the same range, so the two should produce identical
        // triangles if each_triangle() is consistent between the two paths.
        mesh->vertex_data->move_to_start();
        mesh->vertex_data->position(0.0f, 0.0f, 0.0f);
        mesh->vertex_data->move_next();
        mesh->vertex_data->position(1.0f, 0.0f, 0.0f);
        mesh->vertex_data->move_next();
        mesh->vertex_data->position(0.0f, 1.0f, 0.0f);
        mesh->vertex_data->move_next();
        mesh->vertex_data->position(1.0f, 1.0f, 0.0f);
        mesh->vertex_data->move_next();
        mesh->vertex_data->position(2.0f, 1.0f, 0.0f);
        mesh->vertex_data->move_next();
        mesh->vertex_data->done();

        auto indexed = mesh->create_submesh("indexed_strip", material,
                                            INDEX_TYPE_16_BIT,
                                            MESH_ARRANGEMENT_TRIANGLE_STRIP);
        for(uint32_t i = 0; i < 5; ++i) {
            indexed->index_data->index(i);
        }
        indexed->index_data->done();

        auto ranged = mesh->create_submesh("ranged_strip", material,
                                           MESH_ARRANGEMENT_TRIANGLE_STRIP);
        ranged->add_vertex_range(0, 5);

        std::vector<std::array<uint32_t, 3>> indexed_tris;
        indexed->each_triangle([&](uint32_t a, uint32_t b, uint32_t c) {
            indexed_tris.push_back({a, b, c});
        });

        std::vector<std::array<uint32_t, 3>> ranged_tris;
        ranged->each_triangle([&](uint32_t a, uint32_t b, uint32_t c) {
            ranged_tris.push_back({a, b, c});
        });

        assert_equal((std::size_t)3, indexed_tris.size());
        assert_equal(ranged_tris.size(), indexed_tris.size());

        for(std::size_t i = 0; i < indexed_tris.size(); ++i) {
            assert_equal(ranged_tris[i][0], indexed_tris[i][0]);
            assert_equal(ranged_tris[i][1], indexed_tris[i][1]);
            assert_equal(ranged_tris[i][2], indexed_tris[i][2]);
        }

        // Pin down the expected winding at each strip position too (direct,
        // flipped, direct), so a regression is still caught even if both
        // paths broke in the same way.
        assert_equal((uint32_t)0, indexed_tris[0][0]);
        assert_equal((uint32_t)1, indexed_tris[0][1]);
        assert_equal((uint32_t)2, indexed_tris[0][2]);

        assert_equal((uint32_t)2, indexed_tris[1][0]);
        assert_equal((uint32_t)1, indexed_tris[1][1]);
        assert_equal((uint32_t)3, indexed_tris[1][2]);

        assert_equal((uint32_t)2, indexed_tris[2][0]);
        assert_equal((uint32_t)3, indexed_tris[2][1]);
        assert_equal((uint32_t)4, indexed_tris[2][2]);
    }
};

} // namespace
