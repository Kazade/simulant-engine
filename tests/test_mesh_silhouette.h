#pragma once

#include <vector>

#include "../simulant/shadows.h"
#include "../simulant/stage.h"
#include "../simulant/meshes/mesh.h"
#include "../simulant/meshes/submesh.h"
#include "../simulant/nodes/light.h"

namespace {

using namespace smlt;

class MeshSilhouetteTests : public smlt::test::SimulantTestCase {
private:
    /* Builds the adjacency for a submesh and returns the silhouette as seen
     * from the given light. The edge buffer must outlive the returned
     * silhouette, so the caller passes it in. */
    MeshSilhouette silhouette_for(MeshPtr mesh, SubMesh* sm, LightPtr light,
                                  std::vector<EdgeInfo>& edges) {
        build_silhouette_adjacency(mesh->vertex_data.get(),
                                   sm->index_data.get(),
                                   sm->index_data->count(),
                                   sm->arrangement(), edges);
        return MeshSilhouette(mesh->vertex_data.get(), edges, Mat4(), light);
    }

public:
    void test_directional_silhouette_generation() {
        auto mesh = application->shared_assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        auto sm = mesh->create_submesh_as_rectangle("rect", application->shared_assets->create_material(), 1.0, 1.0f);

        auto light = scene->create_child<DirectionalLight>();
        light->transform->set_translation(Vec3(0, 0, -10));

        std::vector<EdgeInfo> edges;
        auto silhouette = silhouette_for(mesh, sm, light, edges);
        assert_equal(4u, silhouette.edge_list().size());
    }

    void test_point_silhouette_generation() {
        auto mesh = application->shared_assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        auto sm = mesh->create_submesh_as_rectangle("rect", application->shared_assets->create_material(), 1.0, 1.0f);

        auto light = scene->create_child<PointLight>();
        light->transform->set_translation(Vec3(0, 0, -10));

        std::vector<EdgeInfo> edges;
        auto silhouette = silhouette_for(mesh, sm, light, edges);
        assert_equal(4u, silhouette.edge_list().size());
    }

    void test_out_of_range_generates_none() {
        auto mesh = application->shared_assets->create_mesh(smlt::VertexSpecification::DEFAULT);
        auto sm = mesh->create_submesh_as_rectangle("rect", application->shared_assets->create_material(), 1.0, 1.0f);

        auto light = scene->create_child<PointLight>();
        light->transform->set_translation(Vec3(0, 0, -10));
        light->set_range(5.0);

        // Out of range of light, no silhouette edges
        std::vector<EdgeInfo> edges;
        auto silhouette = silhouette_for(mesh, sm, light, edges);
        assert_equal(0u, silhouette.edge_list().size());
    }
};

}
