#pragma once

#include <vector>

#include "../simulant/shadows.h"
#include "../simulant/meshes/mesh.h"
#include "../simulant/meshes/submesh.h"
#include "../simulant/nodes/shadow_caster.h"
#include "../simulant/nodes/light.h"
#include "../simulant/stage.h"
#include "../simulant/asset_manager.h"
#include "../simulant/macros.h"

namespace {

using namespace smlt;

class ShadowVolumeTests : public smlt::test::SimulantTestCase {
private:
    // Build a 1x1 rectangle whose face normal points along +Z.
    struct RectGeometry {
        MeshPtr mesh;
        SubMeshPtr submesh;
        std::vector<EdgeInfo> edges;
        Mat4 world; // identity by default
        Renderable renderable;
    };

    RectGeometry make_rect() {
        RectGeometry g;
        g.mesh = application->shared_assets->create_mesh(VertexSpecification::DEFAULT);
        g.submesh = g.mesh->create_submesh_as_rectangle(
            "rect", application->shared_assets->create_material(), 1.0f, 1.0f);

        build_silhouette_adjacency(g.mesh->vertex_data.get(),
                                   g.submesh->index_data.get(),
                                   g.submesh->index_data->count(),
                                   g.submesh->arrangement(),
                                   g.edges);

        g.renderable.arrangement       = g.submesh->arrangement();
        g.renderable.vertex_data       = g.mesh->vertex_data.get();
        g.renderable.index_data        = g.submesh->index_data.get();
        g.renderable.index_element_count = g.submesh->index_data->count();
        g.renderable.first_index       = 0;
        g.renderable.final_transformation = &g.world;
        return g;
    }

public:
    // Light facing the rectangle's front (+Z normal). Both rectangle triangles
    // are lit, so we expect side quads + a 2-triangle front cap fan + a
    // 2-triangle back cap fan around the rectangle's 4-vertex silhouette loop.
    // Vertices are shared between sides and caps: 4 world + 4 extruded = 8.
    void test_directional_lit_produces_sides_and_caps() {
        auto caster = scene->create_child<ShadowCaster>();
        auto g = make_rect();

        // Light "behind" the rectangle (along -Z), so direction() returns +Z
        // (toward-light) which dots positively against the rectangle's +Z
        // normal → triangles are lit.
        auto light = scene->create_child<DirectionalLight>();
        light->set_direction(Vec3(0, 0, -1));

        const auto verts_before = caster->_shadow_volume_vertex_data()->count();
        const auto idx_before   = caster->_shadow_volume_index_data()->count();

        caster->generate_shadow_geometry(g.renderable, g.edges, light, Mat4());

        const auto verts_added = caster->_shadow_volume_vertex_data()->count() - verts_before;
        const auto idx_added   = caster->_shadow_volume_index_data()->count() - idx_before;

        // 1 silhouette loop of 4 verts → 8 shared verts (4 world + 4 extruded).
        // 4 side quads × 6 indices                                  = 24 indices.
        // Front cap fan (N-2 = 2 triangles × 3 indices)             =  6 indices.
        // Back  cap fan (N-2 = 2 triangles × 3 indices)             =  6 indices.
        // Totals                                                    =  8 verts, 36 indices.
        assert_equal(8u, verts_added);
        assert_equal(36u, idx_added);
    }

    // Light pointing away from the rectangle's front face. Boundary silhouette
    // edges are still forced (the missing-neighbour rule makes them always-
    // silhouette on open meshes), so side quads are emitted; but no triangles
    // satisfy the lit test, so neither cap is emitted.
    void test_directional_unlit_emits_sides_only() {
        auto caster = scene->create_child<ShadowCaster>();
        auto g = make_rect();

        // Light in front of the rectangle (along +Z), so direction() returns -Z
        // and the +Z normal is unlit.
        auto light = scene->create_child<DirectionalLight>();
        light->set_direction(Vec3(0, 0, 1));

        const auto verts_before = caster->_shadow_volume_vertex_data()->count();
        const auto idx_before   = caster->_shadow_volume_index_data()->count();

        caster->generate_shadow_geometry(g.renderable, g.edges, light, Mat4());

        const auto verts_added = caster->_shadow_volume_vertex_data()->count() - verts_before;
        const auto idx_added   = caster->_shadow_volume_index_data()->count() - idx_before;

        // 4 side quads only; no caps. Loop is 4 shared verts (world+extruded).
        assert_equal(8u, verts_added);
        assert_equal(24u, idx_added);
    }

    // Point light positioned in front of the rectangle so both triangles are
    // lit and within range. We expect the same side+cap structure as the lit
    // directional case.
    void test_point_in_range_produces_sides_and_caps() {
        auto caster = scene->create_child<ShadowCaster>();
        auto g = make_rect();

        auto light = scene->create_child<PointLight>();
        light->transform->set_translation(Vec3(0.0f, 0.0f, 10.0f));
        light->set_range(100.0f);

        const auto verts_before = caster->_shadow_volume_vertex_data()->count();
        const auto idx_before   = caster->_shadow_volume_index_data()->count();

        caster->generate_shadow_geometry(g.renderable, g.edges, light, Mat4());

        const auto verts_added = caster->_shadow_volume_vertex_data()->count() - verts_before;
        const auto idx_added   = caster->_shadow_volume_index_data()->count() - idx_before;

        assert_equal(8u, verts_added);
        assert_equal(36u, idx_added);
    }

    // Point light outside the caster's range — MeshSilhouette's AABB range
    // check rejects the whole caster and produces no silhouette edges, so the
    // shadow volume should be empty.
    void test_point_out_of_range_produces_no_volume() {
        auto caster = scene->create_child<ShadowCaster>();
        auto g = make_rect();

        auto light = scene->create_child<PointLight>();
        light->transform->set_translation(Vec3(0.0f, 0.0f, 10.0f));
        light->set_range(2.0f); // diameter 4; the rect at z=0 is 10 units away.

        const auto verts_before = caster->_shadow_volume_vertex_data()->count();
        const auto idx_before   = caster->_shadow_volume_index_data()->count();

        caster->generate_shadow_geometry(g.renderable, g.edges, light, Mat4());

        assert_equal(verts_before, caster->_shadow_volume_vertex_data()->count());
        assert_equal(idx_before,   caster->_shadow_volume_index_data()->count());
    }

    // Calling twice should append: the second call's added counts should match
    // the first, and the absolute totals should accumulate.
    void test_repeated_calls_append_to_buffer() {
        auto caster = scene->create_child<ShadowCaster>();
        auto g = make_rect();

        auto light = scene->create_child<DirectionalLight>();
        light->set_direction(Vec3(0, 0, -1));

        caster->generate_shadow_geometry(g.renderable, g.edges, light, Mat4());
        const auto verts_after_first = caster->_shadow_volume_vertex_data()->count();
        const auto idx_after_first   = caster->_shadow_volume_index_data()->count();

        caster->generate_shadow_geometry(g.renderable, g.edges, light, Mat4());

        assert_equal(verts_after_first * 2u, caster->_shadow_volume_vertex_data()->count());
        assert_equal(idx_after_first   * 2u, caster->_shadow_volume_index_data()->count());
    }
};

} // namespace
