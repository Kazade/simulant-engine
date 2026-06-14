#pragma once

#include <vector>

#include "../simulant/shadows.h"
#include "../simulant/meshes/mesh.h"
#include "../simulant/meshes/submesh.h"
#include "../simulant/asset_manager.h"
#include "../simulant/macros.h"

namespace {

using namespace smlt;

class SilhouetteAdjacencyTests : public smlt::test::SimulantTestCase {
private:
    std::pair<std::size_t, std::size_t> count_shared_unshared(const std::vector<EdgeInfo>& edges) {
        std::size_t shared = 0, unshared = 0;
        for(const auto& e: edges) {
            if(e.triangle_count == 2) shared++;
            else if(e.triangle_count == 1) unshared++;
        }
        return {shared, unshared};
    }

public:
    void test_basic_build_from_rectangle() {
        // A rectangle is two triangles sharing one edge — after position welding
        // the canonical edge count is 5 (4 boundary + 1 interior).
        auto mesh = application->shared_assets->create_mesh(VertexSpecification::DEFAULT);
        auto sm = mesh->create_submesh_as_rectangle(
            "rect", application->shared_assets->create_material(), 1.0f, 1.0f);

        std::vector<EdgeInfo> edges;
        build_silhouette_adjacency(mesh->vertex_data.get(),
                                   sm->index_data.get(),
                                   sm->index_data->count(),
                                   sm->arrangement(), edges);

        assert_equal(5u, edges.size());
        auto counts = count_shared_unshared(edges);
        assert_equal(1u, counts.first);   // the shared diagonal
        assert_equal(4u, counts.second);  // the four boundary edges
    }

    void test_duplicate_positions_weld_into_one_canonical_vertex() {
        // create_submesh_as_rectangle emits 4 vertices in distinct positions,
        // but rebuilt rectangles often share buffer slots. To exercise welding
        // explicitly, append a duplicate-position vertex and a triangle that
        // uses it; the duplicate should merge with the original, creating
        // another shared edge.
        auto mesh = application->shared_assets->create_mesh(VertexSpecification::DEFAULT);
        auto sm = mesh->create_submesh_as_rectangle(
            "rect", application->shared_assets->create_material(), 1.0f, 1.0f);

        const auto i = mesh->vertex_data->count();
        mesh->vertex_data->move_to_end();
        // Duplicate of vertex 1.
        mesh->vertex_data->position(*mesh->vertex_data->position_at<Vec3>(1));
        mesh->vertex_data->move_next();
        // A genuinely new corner so the new triangle isn't degenerate.
        mesh->vertex_data->position(10.0f, 10.0f, 0.0f);
        mesh->vertex_data->move_next();
        mesh->vertex_data->done();

        sm->index_data->index(i + 1);
        sm->index_data->index(i);
        sm->index_data->index(0);
        sm->index_data->done();

        std::vector<EdgeInfo> edges;
        build_silhouette_adjacency(mesh->vertex_data.get(),
                                   sm->index_data.get(),
                                   sm->index_data->count(),
                                   sm->arrangement(), edges);

        // The duplicate gets welded to vertex 1's canonical index, so an extra
        // edge becomes shared.
        auto counts = count_shared_unshared(edges);
        assert_equal(2u, counts.first);
    }

    void test_non_triangle_arrangement_yields_no_edges() {
        // Lines / points have no triangle topology so adjacency is empty.
        auto mesh = application->shared_assets->create_mesh(VertexSpecification::DEFAULT);
        auto sm = mesh->create_submesh_as_rectangle(
            "rect", application->shared_assets->create_material(), 1.0f, 1.0f);

        std::vector<EdgeInfo> edges;
        // Note: the submesh's index buffer is triangle-shaped but we pass
        // MESH_ARRANGEMENT_LINES to verify the function rejects it.
        build_silhouette_adjacency(mesh->vertex_data.get(),
                                   sm->index_data.get(),
                                   sm->index_data->count(),
                                   MESH_ARRANGEMENT_LINES, edges);

        assert_equal(0u, edges.size());
    }

    void test_null_vertex_data_yields_no_edges() {
        std::vector<EdgeInfo> edges;
        edges.push_back(EdgeInfo{});  // pre-populate so we can verify clear()
        build_silhouette_adjacency(nullptr, nullptr, 0,
                                   MESH_ARRANGEMENT_TRIANGLES, edges);
        assert_equal(0u, edges.size());
    }

    void test_triangle_strip_produces_expected_count() {
        // Build a manual 4-vertex strip = 2 triangles, like a rectangle.
        auto mesh = application->shared_assets->create_mesh(VertexSpecification::DEFAULT);
        auto sm = mesh->create_submesh(
            "strip", application->shared_assets->create_material(),
            INDEX_TYPE_16_BIT, MESH_ARRANGEMENT_TRIANGLE_STRIP);

        mesh->vertex_data->move_to_end();
        mesh->vertex_data->position(0.0f, 0.0f, 0.0f); mesh->vertex_data->move_next();
        mesh->vertex_data->position(1.0f, 0.0f, 0.0f); mesh->vertex_data->move_next();
        mesh->vertex_data->position(0.0f, 1.0f, 0.0f); mesh->vertex_data->move_next();
        mesh->vertex_data->position(1.0f, 1.0f, 0.0f); mesh->vertex_data->move_next();
        mesh->vertex_data->done();

        sm->index_data->index(0u);
        sm->index_data->index(1u);
        sm->index_data->index(2u);
        sm->index_data->index(3u);
        sm->index_data->done();

        std::vector<EdgeInfo> edges;
        build_silhouette_adjacency(mesh->vertex_data.get(),
                                   sm->index_data.get(),
                                   sm->index_data->count(),
                                   MESH_ARRANGEMENT_TRIANGLE_STRIP, edges);

        // Same topology as a rectangle: 5 unique edges, 1 shared.
        assert_equal(5u, edges.size());
        auto counts = count_shared_unshared(edges);
        assert_equal(1u, counts.first);
        assert_equal(4u, counts.second);
    }

    void test_recompute_normals_refreshes_after_position_change() {
        auto mesh = application->shared_assets->create_mesh(VertexSpecification::DEFAULT);
        auto sm = mesh->create_submesh_as_rectangle(
            "rect", application->shared_assets->create_material(), 1.0f, 1.0f);

        std::vector<EdgeInfo> edges;
        build_silhouette_adjacency(mesh->vertex_data.get(),
                                   sm->index_data.get(),
                                   sm->index_data->count(),
                                   sm->arrangement(), edges);
        assert_true(edges.size() > 0u);

        // create_submesh_as_rectangle emits a quad in the XY plane with normal
        // (0,0,1). The built normals should reflect that.
        const Vec3 expected_initial(0.0f, 0.0f, 1.0f);
        for(const auto& e: edges) {
            assert_close(expected_initial.x, e.normals[0].x, 1e-4f);
            assert_close(expected_initial.y, e.normals[0].y, 1e-4f);
            assert_close(expected_initial.z, e.normals[0].z, 1e-4f);
        }

        // Flip the rectangle around the X axis (negate Y and Z).
        for(uint32_t v = 0; v < mesh->vertex_data->count(); ++v) {
            auto p = *mesh->vertex_data->position_at<Vec3>(v);
            mesh->vertex_data->move_to(v);
            mesh->vertex_data->position(p.x, -p.y, -p.z);
        }
        mesh->vertex_data->done();

        recompute_silhouette_normals(mesh->vertex_data.get(), edges);

        // After flipping Y and Z signs, the face normal flips too — now (0,0,-1).
        const Vec3 expected_after(0.0f, 0.0f, -1.0f);
        for(const auto& e: edges) {
            assert_close(expected_after.x, e.normals[0].x, 1e-4f);
            assert_close(expected_after.y, e.normals[0].y, 1e-4f);
            assert_close(expected_after.z, e.normals[0].z, 1e-4f);
        }
    }
};

} // namespace
