#pragma once


#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/nodes/geoms/octree_culler.h"
#include "simulant/nodes/geom.h"
#include "simulant/renderers/batching/renderable_store.h"

namespace {

using namespace smlt;

class OctreeCullerTests : public smlt::test::SimulantTestCase {
public:
    void test_octree_bounds() {
        auto stage = window->new_stage();

        // Guarantee 2 renderables by using different materials
        auto mat1 = stage->assets->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY);
        auto mat2 = stage->assets->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY);

        auto mesh = stage->assets->new_mesh(VertexSpecification::DEFAULT);
        mesh->new_submesh_as_box("visible", mat1, 1.0, 1.0, 1.0, Vec3(-20, -20, -20.0));
        mesh->new_submesh_as_box("not visible", mat2, 1.0, 1.0, 1.0, Vec3(20, 20, 20.0));

        OctreeCuller culler(nullptr, mesh, 4);
        culler.compile();

        auto bounds = culler.octree_bounds();

        assert_equal(41.0f, bounds.max_dimension());
        assert_equal(bounds.width(), bounds.height());
        assert_equal(bounds.width(), bounds.depth());
        assert_equal(-20.5f, bounds.min().x);
        assert_equal(20.5f, bounds.max().x);
    }

    void test_basic_visibility() {
        auto stage = window->new_stage();
        auto camera = stage->new_camera(); // Looking down -Z

        // Guarantee 2 renderables by using different materials
        auto mat1 = stage->assets->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY);
        auto mat2 = stage->assets->new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY);

        auto mesh = stage->assets->new_mesh(VertexSpecification::DEFAULT);
        mesh->new_submesh_as_box("visible", mat1, 1.0, 1.0, 1.0, Vec3(0, 0, -20.0));
        mesh->new_submesh_as_box("not visible", mat2, 1.0, 1.0, 1.0, Vec3(0, 0, 20.0));

        camera->look_at(0, 0, -1); // Looking up -Z

        RenderableStore store;
        auto factory = store.new_factory();

        auto geom = stage->new_geom_with_mesh(mesh->id());
        geom->culler->renderables_visible(camera->frustum(), factory);

        std::vector<Renderable*> result;
        factory->each_pushed([&](Renderable* r) {
            result.push_back(r);
        });

        // FIXME: Make sure this is the right value!!!
        // things changed when we started returning a renderable
        // per node per material, rather than one per material.
        assert_equal(3u, result.size());

        auto ret1 = *result[0];

        camera->look_at(0, 0, 1); // Looking up +Z

        store.clear();
        factory = store.new_factory();
        result.clear();

        geom->culler->renderables_visible(camera->frustum(), factory);
        factory->each_pushed([&](Renderable* r) {
            result.push_back(r);
        });

        assert_equal(3u, result.size());

        auto ret2 = *result[0];

        // Should be different renderables that came back
        assert_not_equal(ret1.material_id, ret2.material_id);
    }
};

}
