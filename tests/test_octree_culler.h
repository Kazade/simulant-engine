#pragma once


#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/nodes/geoms/octree_culler.h"
#include "simulant/nodes/geom.h"

namespace {

using namespace smlt;

class OctreeCullerTests : public smlt::test::SimulantTestCase {
public:
    void test_octree_bounds() {
        // Guarantee 2 renderables by using different materials
        auto mat1 = scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);
        auto mat2 = scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);

        auto mesh = scene->assets->create_mesh(VertexSpecification::DEFAULT);
        mesh->create_submesh_as_box("visible", mat1, 1.0, 1.0, 1.0, Vec3(-20, -20, -20.0));
        mesh->create_submesh_as_box("not visible", mat2, 1.0, 1.0, 1.0, Vec3(20, 20, 20.0));

        OctreeCuller culler(nullptr, mesh, 4);
        culler.compile(Vec3(), Quaternion(), Vec3(1, 1, 1));

        auto bounds = culler.octree_bounds();

        assert_equal(41.0f, bounds.max_dimension());
        assert_equal(bounds.width(), bounds.height());
        assert_equal(bounds.width(), bounds.depth());
        assert_equal(-20.5f, bounds.min().x);
        assert_equal(20.5f, bounds.max().x);
    }

    void test_basic_visibility() {
        auto stage = scene->create_child<smlt::Stage>();
        auto camera = scene->create_child<smlt::Camera>(); // Looking down -Z

        // Guarantee 2 renderables by using different materials
        auto mat1 = scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);
        auto mat2 = scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);

        auto mesh = scene->assets->create_mesh(VertexSpecification::DEFAULT);
        mesh->create_submesh_as_box("visible", mat1, 1.0, 1.0, 1.0, Vec3(0, 0, -20.0));
        mesh->create_submesh_as_box("not visible", mat2, 1.0, 1.0, 1.0, Vec3(0, 0, 20.0));

        camera->transform->look_at(smlt::Vec3(0, 0, -1)); // Looking up -Z

        batcher::RenderQueue queue;
        queue.reset(stage, window->renderer.get(), camera);
        auto geom = scene->create_child<Geom>(mesh);
        geom->culler->renderables_visible(camera->frustum(), &queue);

        std::vector<Renderable*> result;        
        for(auto i = 0u; i < queue.renderable_count(); ++i) {
            result.push_back(queue.renderable(i));
        }

        // FIXME: Make sure this is the right value!!!
        // things changed when we started returning a renderable
        // per node per material, rather than one per material.
        assert_equal(3u, result.size());

        auto ret1 = *result[0];

        camera->transform->look_at(smlt::Vec3(0, 0, 1)); // Looking up +Z

        queue.clear();
        result.clear();

        geom->culler->renderables_visible(camera->frustum(), &queue);
        for(auto i = 0u; i < queue.renderable_count(); ++i) {
            result.push_back(queue.renderable(i));
        }

        assert_equal(3u, result.size());

        auto ret2 = *result[0];

        // Should be different renderables that came back
        assert_not_equal(ret1.material->id(), ret2.material->id());
    }
};

}
