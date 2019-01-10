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
};

#endif // TEST_OBJ_LOADER_H
