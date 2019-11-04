#ifndef TEST_OBJ_LOADER_H
#define TEST_OBJ_LOADER_H

#include "simulant/deps/kfs/kfs.h"
#include "simulant/loaders/obj_loader.h"
#include "simulant/asset_manager.h"

class OBJLoaderTest : public smlt::test::SimulantTestCase {
public:
    void test_loading_without_texture_coords() {
        //Shouldn't throw
        smlt::MeshID mid = window->shared_assets->new_mesh_from_file("cube.obj");
    }

    void test_culling_method_applied() {
        smlt::MeshLoadOptions opts;
        opts.cull_mode = smlt::CULL_MODE_FRONT_FACE;

        smlt::MeshID mid = window->shared_assets->new_mesh_from_file("cube.obj", opts);
        smlt::MeshPtr m = mid.fetch();

        assert_equal(m->submesh_count(), 1u);
        assert_true(m->first_submesh()->material());

        smlt::MaterialPtr mat = m->first_submesh()->material();
        assert_equal(mat->pass(0)->cull_mode(), opts.cull_mode);
    }
};

#endif // TEST_OBJ_LOADER_H
