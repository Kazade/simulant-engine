#ifndef TEST_OBJ_LOADER_H
#define TEST_OBJ_LOADER_H

#include "simulant/deps/kfs/kfs.h"
#include "simulant/loaders/obj_loader.h"
#include "simulant/asset_manager.h"

class OBJLoaderTest : public smlt::test::SimulantTestCase {
public:
    void test_face_parsing() {
        std::string line = "1//2";

        int32_t v, vn, vt;

        smlt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(-1, vt);
        assert_equal(1, vn);

        line = "1";

        smlt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(-1, vt);
        assert_equal(-1, vn);

        line = "1/2";

        smlt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(1, vt);
        assert_equal(-1, vn);

        line = "1/2/3";

        smlt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(1, vt);
        assert_equal(2, vn);

        line = "1//";

        smlt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(-1, vt);
        assert_equal(-1, vn);
    }

    void test_loading_without_texture_coords() {
        //Shouldn't throw
        smlt::MeshID mid = window->shared_assets->new_mesh_from_file("cube.obj");
    }
};

#endif // TEST_OBJ_LOADER_H
