#pragma once

#include "simulant/test.h"
#include "simulant/utils/kfs.h"
#include "simulant/loaders/obj_loader.h"
#include "simulant/asset_manager.h"

namespace {

using namespace smlt;

class OBJLoaderTest : public smlt::test::SimulantTestCase {
public:
    void test_loading_without_texture_coords() {
        //Shouldn't throw
        application->shared_assets->load_mesh("assets/samples/cube.obj");
    }

    void test_culling_method_applied() {
        smlt::MeshLoadOptions opts;
        opts.cull_mode = smlt::CULL_MODE_FRONT_FACE;

        auto m = application->shared_assets->load_mesh(
            "assets/samples/cube.obj", opts);

        assert_equal(m->submesh_count(), 1u);
        assert_true(m->first_submesh()->material());

        smlt::MaterialPtr mat = m->first_submesh()->material();
        assert_equal(mat->pass(0)->cull_mode(), opts.cull_mode);
    }

    void test_vertex_colors() {

        std::string obj_file(R"(
            v 0.0 0.0 0.0 1.0 0.0 0.0
            v 1.0 0.0 0.0 0.0 1.0 0.0
            v 0.0 1.0 0.0 0.0 0.0 1.0
            f 1 2 3
        )");

        loaders::OBJLoader loader(
            "test.obj",
            std::make_shared<std::istringstream>(obj_file)
        );

        auto spec = VertexFormat::standard();
        auto mesh = application->shared_assets->create_mesh(spec);
        loader.into(*mesh);

        assert_equal(mesh->vertex_data->count(), 3u);

        const uint8_t* bytes =
            mesh->vertex_data->attr_at(VERTEX_ATTR_NAME_COLOR, 0);

        assert_equal(bytes[0], 0);   // B
        assert_equal(bytes[1], 0);   // G
        assert_equal(bytes[2], 255); // R
        assert_equal(bytes[3], 255); // A

        bytes = mesh->vertex_data->attr_at(VERTEX_ATTR_NAME_COLOR, 1);
        assert_equal(bytes[0], 0);   // B
        assert_equal(bytes[1], 255); // G
        assert_equal(bytes[2], 0);   // R
        assert_equal(bytes[3], 255); // A
    }

    void test_vertex_colors_default_white() {

        std::string obj_file(R"(
            v 0.0 0.0 0.0
            v 1.0 0.0 0.0
            v 0.0 1.0 0.0
            f 1 2 3
        )");

        loaders::OBJLoader loader(
            "test.obj",
            std::make_shared<std::istringstream>(obj_file)
        );

        auto spec = VertexFormat::standard();
        auto mesh = application->shared_assets->create_mesh(spec);
        loader.into(*mesh);

        assert_equal(mesh->vertex_data->count(), 3u);

        const uint8_t* bytes =
            mesh->vertex_data->attr_at(VERTEX_ATTR_NAME_COLOR, 0);
        assert_equal(bytes[0], 255);  // B
        assert_equal(bytes[1], 255);  // G
        assert_equal(bytes[2], 255);  // R
        assert_equal(bytes[3], 255);  // A

        bytes = mesh->vertex_data->attr_at(VERTEX_ATTR_NAME_COLOR, 1);
        assert_equal(bytes[0], 255);  // B
        assert_equal(bytes[1], 255);  // G
        assert_equal(bytes[2], 255);  // R
        assert_equal(bytes[3], 255);  // A
    }

    void test_specification_override() {
        std::string obj_file(R"(
            v 0.0 0.0 0.0
            v 1.0 0.0 0.0
            v 0.0 1.0 0.0
            f 1 2 3
        )");

        loaders::OBJLoader loader("assets/samples/test.obj",
                                  std::make_shared<std::istringstream>(obj_file));

        auto mesh =
            application->shared_assets->load_mesh("assets/samples/cube.obj");
        loader.into(*mesh);

        assert_equal(mesh->vertex_data->count(), 3u);

        auto spec = mesh->vertex_data->vertex_specification();
        assert_false(spec.attr_count(VERTEX_ATTR_NAME_COLOR));
        assert_false(spec.attr_count(VERTEX_ATTR_NAME_NORMAL));
        assert_false(spec.attr_count(VERTEX_ATTR_NAME_TEXCOORD_0));
    }
};

}
