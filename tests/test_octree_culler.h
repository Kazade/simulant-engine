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
        auto camera = scene->create_child<smlt::Camera3D>(); // Looking down -Z

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

        // Only the near box is visible and all of its triangles fit in a
        // single contiguous range of the shared per-material index buffer.
        assert_equal(1u, result.size());

        auto ret1 = *result[0];

        camera->transform->look_at(smlt::Vec3(0, 0, 1)); // Looking up +Z

        queue.clear();
        result.clear();

        geom->culler->renderables_visible(camera->frustum(), &queue);
        for(auto i = 0u; i < queue.renderable_count(); ++i) {
            result.push_back(queue.renderable(i));
        }

        assert_equal(1u, result.size());

        auto ret2 = *result[0];

        // Should be different renderables that came back
        assert_not_equal(ret1.material->id(), ret2.material->id());
    }
    void test_large_mesh_uses_32bit_indices() {
        auto stage = scene->create_child<smlt::Stage>();
        auto camera = scene->create_child<smlt::Camera3D>(); // Looking down -Z

        auto material =
            scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);

        auto mesh = scene->assets->create_mesh(VertexSpecification::DEFAULT);
        auto* vertex_data = mesh->vertex_data.get();
        vertex_data->move_to_start();

        // More vertices than a 16-bit index can address.
        const uint32_t vertex_count = 70000;
        const uint32_t grid_size = 300;

        for(uint32_t i = 0; i < vertex_count; ++i) {
            Vec3 position(float(i % grid_size) - 150.0f, 0.0f,
                          float(i / grid_size) - 150.0f);

            // Use the last three vertices for a triangle close to the camera.
            // Its indices are > 65535 so a 16-bit index buffer would wrap.
            if(i == vertex_count - 3) {
                position = Vec3(0.0f, 0.0f, -20.0f);
            } else if(i == vertex_count - 2) {
                position = Vec3(1.0f, 0.0f, -20.0f);
            } else if(i == vertex_count - 1) {
                position = Vec3(0.0f, 1.0f, -20.0f);
            }

            vertex_data->position(position);
            vertex_data->move_next();
        }

        vertex_data->done();

        auto submesh = mesh->create_submesh("large", material,
                                            INDEX_TYPE_32_BIT,
                                            MESH_ARRANGEMENT_TRIANGLES);
        submesh->index_data->index(vertex_count - 3);
        submesh->index_data->index(vertex_count - 2);
        submesh->index_data->index(vertex_count - 1);
        submesh->index_data->done();

        auto geom = scene->create_child<Geom>(mesh);

        batcher::RenderQueue queue;
        queue.reset(stage, window->renderer.get(), camera);
        geom->culler->renderables_visible(camera->frustum(), &queue);

        assert_equal(1u, queue.renderable_count());
        assert_equal(INDEX_TYPE_32_BIT,
                     queue.renderable(0)->index_data->index_type());
    }

    void test_adjacent_nodes_merged_into_one_draw_call() {
        auto stage = scene->create_child<smlt::Stage>();
        auto camera = scene->create_child<smlt::Camera3D>(); // Looking down -Z

        // Both boxes share a material, so they end up in the same per-material
        // index buffer. They are far enough apart to be in different octree
        // nodes, but there are no other triangles between them in traversal
        // order, so their ranges are adjacent and should be merged.
        auto material =
            scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);

        auto mesh = scene->assets->create_mesh(VertexSpecification::DEFAULT);
        mesh->create_submesh_as_box("left", material, 1.0, 1.0, 1.0,
                                    Vec3(-8, 0, -20.0));
        mesh->create_submesh_as_box("right", material, 1.0, 1.0, 1.0,
                                    Vec3(8, 0, -20.0));

        auto geom = scene->create_child<Geom>(mesh);

        batcher::RenderQueue queue;
        queue.reset(stage, window->renderer.get(), camera);
        geom->culler->renderables_visible(camera->frustum(), &queue);

        assert_equal(1u, queue.renderable_count());

        // Both boxes are 12 triangles with 3 indices each
        assert_equal(72u, queue.renderable(0)->index_element_count);
    }
};

}
