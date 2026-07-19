#pragma once

#include "simulant/simulant.h"
#include "simulant/test.h"

namespace {

using namespace smlt;

class AssetSharingTests : public smlt::test::SimulantTestCase {
public:
    void test_load_texture_twice_returns_same_asset() {
        auto tex1 = scene->assets->load_texture("assets/samples/crate.png");
        auto tex2 = scene->assets->load_texture("assets/samples/crate.png");

        assert_true(tex1);
        assert_equal(tex1.get(), tex2.get());
    }

    void test_load_texture_use_asset_cache_false_returns_different_assets() {
        TextureFlags flags;
        flags.use_asset_cache = false;

        auto tex1 = scene->assets->load_texture("assets/samples/crate.png");
        auto tex2 = scene->assets->load_texture("assets/samples/crate.png", flags);

        assert_true(tex1);
        assert_true(tex2);
        assert_not_equal(tex1.get(), tex2.get());
    }

    void test_load_texture_sets_source() {
        auto tex = scene->assets->load_texture("assets/samples/crate.png");
        assert_false(tex->source().str().empty());
    }

    void test_load_mesh_twice_referencing_same_texture_shares_texture() {
        auto before = scene->assets->texture_count();

        /* Two different mesh files, both referencing
         * assets/samples/crate.png via their .mtl */
        auto mesh1 = scene->assets->load_mesh("assets/samples/share_test_a.obj");
        auto mesh2 = scene->assets->load_mesh("assets/samples/share_test_b.obj");

        assert_true(mesh1);
        assert_true(mesh2);
        assert_not_equal(mesh1.get(), mesh2.get());

        auto tex1 = mesh1->first_submesh()->material()->base_color_map();
        auto tex2 = mesh2->first_submesh()->material()->base_color_map();

        assert_true(tex1);
        assert_equal(tex1.get(), tex2.get());

        /* Only one new texture (crate.png) should have been loaded across
         * both mesh loads */
        assert_equal(scene->assets->texture_count(), before + 1);
    }

    void test_load_mesh_use_asset_cache_false_loads_texture_twice() {
        auto before = scene->assets->texture_count();

        MeshLoadOptions opts;
        opts.use_asset_cache = false;

        auto mesh1 = scene->assets->load_mesh("assets/samples/share_test_a.obj",
                                              VertexSpecification::DEFAULT, opts);
        auto mesh2 = scene->assets->load_mesh("assets/samples/share_test_b.obj",
                                              VertexSpecification::DEFAULT, opts);

        auto tex1 = mesh1->first_submesh()->material()->base_color_map();
        auto tex2 = mesh2->first_submesh()->material()->base_color_map();

        assert_true(tex1);
        assert_not_equal(tex1.get(), tex2.get());
        assert_equal(scene->assets->texture_count(), before + 2);
    }

    void test_load_mesh_twice_same_path_shares_mesh() {
        auto mesh1 = scene->assets->load_mesh("assets/samples/cube.obj");
        auto mesh2 = scene->assets->load_mesh("assets/samples/cube.obj");

        assert_true(mesh1);
        assert_equal(mesh1.get(), mesh2.get());
    }

    void test_load_material_defaults_to_independent_instances() {
        /* Materials default to use_asset_cache=false: loading the same builtin
         * material path twice must return two independent instances, since
         * that's an established idiom in this codebase for getting a fresh,
         * mutable material (see e.g. Debug::on_init) */
        auto mat1 = scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);
        auto mat2 = scene->assets->load_material(Material::BuiltIns::DIFFUSE_ONLY);

        assert_true(mat1);
        assert_not_equal(mat1.get(), mat2.get());
    }

    void test_load_material_use_asset_cache_true_shares_instances() {
        auto mat1 = scene->assets->load_material(
            Material::BuiltIns::DIFFUSE_ONLY, GARBAGE_COLLECT_PERIODIC, true);
        auto mat2 = scene->assets->load_material(
            Material::BuiltIns::DIFFUSE_ONLY, GARBAGE_COLLECT_PERIODIC, true);

        assert_true(mat1);
        assert_equal(mat1.get(), mat2.get());
    }

    void test_texture_reloads_after_being_garbage_collected() {
        Texture* first_ptr = nullptr;

        {
            auto tex = scene->assets->load_texture(
                "assets/samples/crate.png", TextureFlags(),
                GARBAGE_COLLECT_PERIODIC);
            first_ptr = tex.get();
        }

        /* Dropping the last reference above should allow GC to collect it */
        scene->assets->run_garbage_collection();

        auto tex2 = scene->assets->load_texture("assets/samples/crate.png");

        /* We can't guarantee the allocator won't reuse the same address,
         * so the meaningful assertion is that this doesn't return a stale/
         * broken texture - it should be usable and have data. */
        assert_true(tex2);
        assert_true(tex2->width() > 0);
        _S_UNUSED(first_ptr);
    }
};

}
