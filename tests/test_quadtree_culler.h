#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"
#include "simulant/nodes/geoms/quadtree_culler.h"
#include "simulant/nodes/geom.h"

namespace {

using namespace smlt;

class QuadtreeCullerTests: public smlt::test::SimulantTestCase {
public:
    void test_adjacent_nodes_merged_into_one_draw_call() {
        auto stage = scene->create_child<smlt::Stage>();
        auto camera = scene->create_child<smlt::Camera3D>(); // Looking down -Z

        auto material =
            scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);

        auto mesh = scene->assets->create_mesh(VertexSpecification::DEFAULT);
        mesh->create_submesh_as_box("left", material, 1.0, 1.0, 1.0,
                                    Vec3(-8, 0, -20.0));
        mesh->create_submesh_as_box("right", material, 1.0, 1.0, 1.0,
                                    Vec3(8, 0, -20.0));

        GeomCullerOptions options;
        options.type = GEOM_CULLER_TYPE_QUADTREE;
        options.quadtree_max_depth = 4;

        auto geom = scene->create_child<Geom>(mesh, options);

        batcher::RenderQueue queue;
        queue.reset(stage, window->renderer.get(), camera);
        geom->culler->renderables_visible(camera->frustum(), &queue);

        assert_equal(1u, queue.renderable_count());

        // Both boxes are 12 triangles with 3 indices each
        assert_equal(72u, queue.renderable(0)->index_element_count);
    }
};

} // namespace
