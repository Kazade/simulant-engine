#ifndef TEST_OBJ_LOADER_H
#define TEST_OBJ_LOADER_H

#include <kfs/kfs.h>
#include "kglt/loaders/obj_loader.h"

class OBJLoaderTest : public KGLTTestCase {
public:
    void test_face_parsing() {
        unicode line = "1//2";

        int32_t v, vn, vt;

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(-1, vt);
        assert_equal(1, vn);

        line = "1";

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(-1, vt);
        assert_equal(-1, vn);

        line = "1/2";

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(1, vt);
        assert_equal(-1, vn);

        line = "1/2/3";

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(1, vt);
        assert_equal(2, vn);

        line = "1//";

        kglt::loaders::parse_face(line, v, vt, vn);

        assert_equal(0, v);
        assert_equal(-1, vt);
        assert_equal(-1, vn);
    }

    void test_loading_without_texture_coords() {
        kfs::Path path = kfs::path::join(kfs::path::dir_name(__FILE__), "test-data");
        window->resource_locator->add_search_path(path);

        //Shouldn't throw
        kglt::MeshID mid = window->shared_assets->new_mesh_from_file("cube.obj");
    }
};

#endif // TEST_OBJ_LOADER_H
